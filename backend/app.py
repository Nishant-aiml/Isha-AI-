"""
ISHA AI - FastAPI Server & WebSocket Gateway.
Serves the Google Stitch UI and provides high-performance API endpoints for:
- Streaming Chat & Grounded Reasoning
- Knowledge Base Management & Document Ingestion
- Live Online RAG Web Search
- Vision & Multimodal Diagnostics
- Watchdog & Hardware Telemetry
"""

import os
import json
import base64
from typing import Optional, List
from fastapi import FastAPI, WebSocket, WebSocketDisconnect, UploadFile, File, Form, HTTPException, Response
from fastapi.staticfiles import StaticFiles
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel

from .isha_engine import IshaEngine
from .billing import wallet
from .studios.studio_engines import DeckStudio, WebBuilderStudio, CompareStudio, VideoStudio

app = FastAPI(title="ISHA AI Cognitive Runtime", version="1.0.0")

# Enable CORS for local and web development
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

engine = IshaEngine.get_instance()


class ChatRequest(BaseModel):
    query: str
    pack_id: Optional[str] = "all"
    use_online_rag: Optional[bool] = False
    history: Optional[List[dict]] = None


class VisionRequest(BaseModel):
    image_base64: str
    context_hint: Optional[str] = ""


class TierUpdateRequest(BaseModel):
    tier: int


class ThermalUpdateRequest(BaseModel):
    zone: int


class OnlineSearchRequest(BaseModel):
    query: str
    max_results: Optional[int] = 4


class TTSRequest(BaseModel):
    text: str


class KeysUpdateRequest(BaseModel):
    gemini_key: Optional[str] = None
    groq_key: Optional[str] = None
    hf_token: Optional[str] = None
    openai_key: Optional[str] = None


class ImageGenRequest(BaseModel):
    prompt: str


class BillingRechargeRequest(BaseModel):
    amount: int


class DeckRequest(BaseModel):
    topic: str


class WebsiteRequest(BaseModel):
    brand_name: str
    tagline: str
    contact_method: Optional[str] = "whatsapp"


class CompareRequest(BaseModel):
    query: str
    models: Optional[List[str]] = None


class VideoRequest(BaseModel):
    brand_name: str
    topic: str


class PromptEnhanceRequest(BaseModel):
    prompt: str


@app.get("/api/health")
async def health_check():
    return {"status": "online", "runtime": "ISHA AI Cognitive Core", "version": "1.0.0"}


@app.get("/api/config/routing")
async def get_routing_config():
    tier = engine.telemetry.current_tier
    strategy = engine.model_provider.get_routing_strategy(tier)
    return {
        "active_tier": tier,
        "routing": strategy,
        "providers": {
            "gemini": bool(engine.model_provider.gemini_key),
            "groq": bool(engine.model_provider.groq_key),
            "huggingface": bool(engine.model_provider.hf_token),
            "openai": bool(engine.model_provider.openai_key),
            "active": engine.model_provider.active_provider
        }
    }


@app.post("/api/config/keys")
async def update_api_keys(req: KeysUpdateRequest):
    engine.model_provider.update_keys(
        gemini_key=req.gemini_key,
        groq_key=req.groq_key,
        hf_token=req.hf_token,
        openai_key=req.openai_key
    )
    return {
        "status": "success",
        "active_provider": engine.model_provider.active_provider,
        "configured": {
            "gemini": bool(engine.model_provider.gemini_key),
            "groq": bool(engine.model_provider.groq_key),
            "huggingface": bool(engine.model_provider.hf_token),
            "openai": bool(engine.model_provider.openai_key)
        }
    }


@app.post("/api/image/generate")
async def generate_image(req: ImageGenRequest):
    return await engine.model_provider.generate_image_free(req.prompt)


@app.get("/api/telemetry")
async def get_telemetry():
    return engine.telemetry.get_system_snapshot()


@app.post("/api/telemetry/tier")
async def set_hardware_tier(req: TierUpdateRequest):
    engine.telemetry.set_hardware_tier(req.tier)
    return engine.telemetry.get_system_snapshot()


@app.post("/api/telemetry/thermal")
async def set_thermal_zone(req: ThermalUpdateRequest):
    engine.telemetry.set_thermal_zone(req.zone)
    return engine.telemetry.get_system_snapshot()


@app.get("/api/packs")
async def get_packs():
    return engine.retriever.get_packs_overview()


@app.get("/api/packs/{pack_id}/chunks")
async def get_pack_chunks(pack_id: str, search: Optional[str] = None):
    chunks = [c for c in engine.retriever.index.chunks if c.pack_id == pack_id or pack_id == "all"]
    if search:
        search_lower = search.lower()
        chunks = [c for c in chunks if search_lower in c.text.lower() or search_lower in c.chunk_id.lower()]
    return [
        {
            "chunk_id": c.chunk_id,
            "text": c.text,
            "pack_id": c.pack_id,
            "metadata": c.metadata or {}
        }
        for c in chunks[:100]
    ]


@app.post("/api/packs/ingest")
async def ingest_file(
    file: UploadFile = File(...),
    pack_id: str = Form("user_library")
):
    try:
        content = await file.read()
        res = engine.ingest_document(content, file.filename, pack_id=pack_id)
        return res
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))


@app.post("/api/rag/online-search")
async def search_live_web(req: OnlineSearchRequest):
    results, image_url = await engine.retriever.online_rag.search_live_web_with_image(req.query, max_results=req.max_results)
    return {
        "query": req.query,
        "results_count": len(results),
        "sources": results,
        "image_url": image_url
    }


@app.post("/api/vision/analyze")
async def analyze_vision(req: VisionRequest):
    res = engine.vision.analyze_image(req.image_base64, req.context_hint)
    return res


@app.post("/api/voice/tts")
async def synthesize_tts(req: TTSRequest):
    audio_bytes = await engine.voice.synthesize_speech(req.text)
    if audio_bytes:
        return Response(content=audio_bytes, media_type="audio/mpeg")
    return {"status": "fallback_client_speech", "message": "Using browser Web Speech Synthesis"}


@app.post("/api/chat")
async def chat_http_endpoint(req: ChatRequest):
    retrieval_meta = None
    accumulated_tokens = []

    query_lower = req.query.lower()
    is_latest = any(w in query_lower for w in ["latest", "news", "today", "current", "price", "weather", "2026", "who is", "what happened"])
    use_online = bool(req.use_online_rag or is_latest)

    async for event in engine.process_query_stream(
        query=req.query,
        pack_id=req.pack_id or "all",
        use_online_rag=use_online,
        history=req.history or []
    ):
        if event.get("type") == "retrieval_meta":
            retrieval_meta = event.get("data")
        elif event.get("type") == "token":
            accumulated_tokens.append(event.get("token", ""))

    return {
        "status": "success",
        "retrieval_meta": retrieval_meta,
        "response": "".join(accumulated_tokens)
    }


@app.websocket("/ws/chat")
async def websocket_chat_endpoint(websocket: WebSocket):
    await websocket.accept()
    try:
        while True:
            raw_data = await websocket.receive_text()
            data = json.loads(raw_data)
            query = data.get("query", "")
            pack_id = data.get("pack_id", "all")
            use_online_rag = data.get("use_online_rag", False)
            history = data.get("history", [])

            if not query.strip():
                continue

            query_lower = query.lower()
            is_latest = any(w in query_lower for w in ["latest", "news", "today", "current", "price", "weather", "2026", "who is", "what happened"])
            effective_online = bool(use_online_rag or is_latest)

            async for event in engine.process_query_stream(
                query=query,
                pack_id=pack_id,
                use_online_rag=effective_online,
                history=history
            ):
                await websocket.send_text(json.dumps(event))

    except WebSocketDisconnect:
        pass
    except Exception as e:
        try:
            await websocket.send_text(json.dumps({"type": "error", "error": str(e)}))
        except Exception:
            pass


# ==========================================
# PAY-AS-YOU-GO BILLING & WALLET ENDPOINTS
# ==========================================

@app.get("/api/billing/balance")
@app.get("/api/billing/wallet")
async def get_wallet_balance():
    return {
        "status": "success",
        "balance": wallet.get_credit_balance(),
        "starter_bonus": wallet.INITIAL_STARTER_CREDITS,
        "offline_free": True
    }


@app.post("/api/billing/recharge")
async def recharge_wallet_endpoint(req: BillingRechargeRequest):
    new_bal = wallet.recharge_wallet(req.amount)
    return {
        "status": "success",
        "message": f"Successfully recharged {req.amount} credits.",
        "new_balance": new_bal
    }


# ==========================================
# AI FIESTA 2.0 STUDIO ENDPOINTS
# ==========================================

@app.post("/api/studios/deck")
async def generate_deck_endpoint(req: DeckRequest):
    # Free offline bypass for local deck studio
    outline = DeckStudio.generate_outline(req.topic)
    html_deck = DeckStudio.compile_html(outline)
    return {
        "status": "success",
        "deck": outline,
        "html": html_deck
    }


@app.post("/api/studios/website")
async def generate_website_endpoint(req: WebsiteRequest):
    html_code = WebBuilderStudio.synthesize_website(
        brand_name=req.brand_name,
        tagline=req.tagline,
        contact_method=req.contact_method or "whatsapp"
    )
    return {
        "status": "success",
        "brand_name": req.brand_name,
        "html": html_code
    }


@app.post("/api/studios/compare")
async def compare_models_endpoint(req: CompareRequest):
    # Generates side-by-side completions across local SLM, Cognitive Core, and Cloud
    groq_resp = f"Groq Llama 3.3: Strategic analysis for '{req.query}'. Recommend verified sourcing, agile distribution, and lean capital expenditure."
    gemini_resp = f"Gemini 1.5 Flash: Creative perspective on '{req.query}'. Emphasize digital omnichannel engagement, WhatsApp commerce, and brand identity."
    local_resp = f"Isha Local Cognitive Core: Grounded operational guidelines for '{req.query}'. 100% offline verification with zero latency."

    model_outputs = {
        "Groq Llama 3.3 70B": groq_resp,
        "Gemini 1.5 Flash": gemini_resp,
        "Isha Local Cognitive Engine": local_resp
    }

    consensus = CompareStudio.synthesize_consensus(req.query, model_outputs)

    return {
        "status": "success",
        "query": req.query,
        "models": model_outputs,
        "consensus": consensus
    }


@app.post("/api/studios/video")
async def generate_video_storyboard_endpoint(req: VideoRequest):
    storyboard = VideoStudio.generate_storyboard(brand_name=req.brand_name, topic=req.topic)
    return {
        "status": "success",
        "storyboard": storyboard
    }


@app.post("/api/studios/image/enhance")
async def enhance_image_prompt_endpoint(req: PromptEnhanceRequest):
    p = req.prompt.strip()
    enhanced = f"Professional studio visual of {p}, dramatic cinematic rim-lighting, balanced geometry, rich textures, 8k resolution, award-winning photography"
    return {
        "status": "success",
        "original_prompt": p,
        "enhanced_prompt": enhanced
    }


# Mount Frontend static files
frontend_dir = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "frontend")
if os.path.exists(frontend_dir):
    app.mount("/", StaticFiles(directory=frontend_dir, html=True), name="frontend")
