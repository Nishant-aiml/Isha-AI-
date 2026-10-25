# ISHA AI — Intelligent Speech & Haptic Assistant
> **"Minimum Compute. Maximum Intelligence. Structure does the thinking. Model does the speaking."**

ISHA AI is a privacy-first, on-device AI cognitive runtime designed to run natively on smartphones and personal computers — without requiring cloud subscriptions or permanent internet connectivity, while supporting **Online Live RAG** when internet is accessible.

---

## 🌟 Key Features

### 1. 🎨 Google Stitch UI Design System
- **Ultra-Modern Canvas Layout**: Frosted glass surfaces (`backdrop-filter: blur`), glowing saffron (`#f59e0b`) & electric cyan (`#06b6d4`) accents, and clean micro-interactions.
- **Dark & Light Mode**: Seamless theme switching with persistent client state.
- **Live Mode Beacon**: Real-time indicator displaying **Device Offline Mode** vs. **Online Live Mode (Live RAG)**.
- **Hardware Tier Badges**: Real-time status for simulated Tier 1 (4GB Budget), Tier 2 (8GB Standard), and Tier 3 (16GB+ Flagship).

### 2. 🌐 Online Live RAG & MCP Architecture
- **Real-Time Web Search & Scraping**: Connects directly to DuckDuckGo and Wikipedia REST APIs to fetch live web information.
- **Privacy Sanitization Layer**: Automatically scrubs phone numbers, emails, location identifiers, and personal data locally *before* transmitting search queries.
- **Grounded Citations**: Synthesizes responses with structured citations `[W1]`, `[W2]` linked to live source articles and domains.

### 3. 📚 Knowledge Base Studio & Domain Packs
- **Pre-Bundled Domain Packs**:
  - 🌾 **Kisan Saathi**: Crop disease diagnostics (wheat rust, blast), fertilizer dosages (NPK, DAP), drip irrigation, PM-Kisan.
  - 🩺 **Swasthya Rakshak**: Pediatric paracetamol dosing, ORS preparation, burn first aid, dengue danger signs, CPR.
  - ⚖️ **Bharat Vidhi**: Fundamental Rights (Articles 14-32), police FIR filing guidelines, Consumer Protection Act, emergency helplines (112, 108, 1091, 1930).
  - 🎓 **Shiksha Prep**: STEM formulas, Newton's laws, thermodynamics, calculus, and biology cycles.
- **Document Ingestion Pipeline**:
  - Drag-and-drop support for `.pdf`, `.docx`, `.txt`, `.md`, `.csv`, `.json`.
  - **Boundary-Aware Semantic Chunker**: Preserves tables, bullet points, and numbered lists (250 words target, 32 overlap).
  - **SHA-256 Deduplication**: Prevents duplicate chunk indexing.
  - **EdgeFlatVectorIndex**: Fast character 3-gram TF-IDF normalized 64-D vector index with BM25-lite keyword fallback.

### 4. 👁️ Vision & Multimodal Studio
- **Crop Disease Diagnostics**: Identifies Wheat Yellow Rust, Rice Blast, and Nitrogen Chlorosis with confidence metrics and treatment remedies.
- **Prescription & Document OCR**: Identifies medical dosages and validates safety baselines.
- **Frame Governor**: 5 FPS cap and thermal throttling prevention.

### 5. 🎙️ Voice STT & Waveform Audio
- **Speech-to-Text (STT)**: Web Speech API microphone dictation.
- **Waveform Pulse Visualizer**: Real-time HTML5 Canvas waveform animation during speech input and output.
- **Text-to-Speech (TTS)**: High-clarity audio speech synthesis.

### 6. ⚡ Runtime Watchdog & Telemetry
- **Dynamic Degradation Ladder**: Simulates thermal transitions across Zone 1 Normal (<38°C), Zone 2 Warm (38-42°C), Zone 3 Hot (42-46°C), and Zone 4 Critical (>46°C).
- **Resource Profiler**: Tracks Process RSS memory, system RAM %, CPU %, and KV Cache occupancy in real time.

---

## 🚀 Quick Start (1-Click Launch)

### Option A: Windows 1-Click Batch File
Double-click `run_isha.bat` in the repository root.

### Option B: Python CLI
```bash
python run_isha.py
```

### Option C: PowerShell
```powershell
.\run_isha.ps1
```

Once launched, ISHA AI automatically opens your default browser at:
```
http://127.0.0.1:8000
```

---

## 🧪 Running the Verification Test Suite

Verify all 7 core capability subsystems (Knowledge Packs, Live RAG, Ingestion, Streaming, Vision, Watchdog) with:
```bash
python test_isha_runtime.py
```

Expected output:
```
=== ISHA AI Comprehensive Verification Suite ===
--- Test 1: Grounded Knowledge Pack Retrieval (Kisan Saathi) --- [PASSED]
--- Test 2: Health Knowledge Pack Retrieval (Swasthya) --- [PASSED]
--- Test 3: Online Live RAG (DuckDuckGo + Wikipedia) --- [PASSED]
--- Test 4: Document Ingestion Pipeline --- [PASSED]
--- Test 5: Full Streaming Cognitive Pipeline --- [PASSED]
--- Test 6: Vision & Multimodal Analysis --- [PASSED]
--- Test 7: Runtime Telemetry & Watchdog --- [PASSED]
>>> ALL 7 CORE CAPABILITY TESTS PASSED WITH 100% SUCCESS! <<<
```

---

## 🛠️ Project Directory Structure

```
├── backend/
│   ├── app.py                     # FastAPI REST API & WebSocket server
│   ├── isha_engine.py             # Central cognitive runtime coordinator
│   ├── rag/
│   │   ├── index.py               # EdgeFlatVectorIndex (Cosine + BM25-lite)
│   │   ├── embeddings.py          # Character 3-gram TF-IDF embedding engine
│   │   ├── chunker.py             # Semantic boundary-aware text chunker
│   │   ├── document_parser.py     # PDF, DOCX, TXT, MD, CSV, JSON parser + SHA-256
│   │   ├── knowledge_packs.py     # Kisan Saathi, Swasthya, Bharat Core, Shiksha
│   │   ├── online_rag.py          # Live web search (DDG, Wikipedia) & PII scrubber
│   │   └── hybrid_retriever.py    # Local pack + Live web hybrid retriever
│   ├── cognition/
│   │   ├── context_governor.py    # Hardware token budget controller
│   │   └── grounded_pipeline.py   # Grounded prompt synthesis & citation manager
│   ├── inference/
│   │   └── model_provider.py      # On-device cognitive engine + Gemini/OpenAI fallback
│   ├── vision/
│   │   └── vision_engine.py       # Crop pathology & OCR vision engine
│   ├── voice/
│   │   └── voice_service.py       # Audio transcription & TTS speech synthesis
│   └── telemetry/
│       └── monitor.py             # Hardware tiers, thermal zones, memory guard
│
├── frontend/
│   ├── index.html                 # Google Stitch single-page application
│   ├── css/stitch.css             # Stitch glassmorphic design system
│   └── js/stitch-app.js           # Reactive UI, WebSockets, STT/TTS & audio visualizer
│
├── core/                          # Native C++ edge runtime & survival layers
├── android/                       # Native Android JNI app & thermal service
├── benchmarks/                    # C++ benchmarks
├── run_isha.py                    # Production launcher
├── run_isha.bat                   # 1-click Windows launcher
├── run_isha.ps1                   # PowerShell launcher
└── test_isha_runtime.py           # Verification test suite
```

---

## 🔧 Optional LLM Providers

ISHA AI works **100% offline out-of-the-box** using its native grounded cognitive generator. If you wish to connect cloud models or local LLMs:
- **Google Gemini**: Set environment variable `GEMINI_API_KEY="your_api_key"`
- **OpenAI**: Set environment variable `OPENAI_API_KEY="your_api_key"`
- **Ollama**: Run Ollama locally on `http://localhost:11434`
