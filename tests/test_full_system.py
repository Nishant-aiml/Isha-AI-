"""
ISHA AI - Comprehensive End-to-End System Test Suite.
Verifies:
1. REST API endpoints
2. WebSocket real-time token streaming
3. Multi-format Document Ingestion & Deduplication
4. Hybrid Vector + BM25-lite Retrieval Accuracy
5. Online Live RAG & PII Sanitization
6. Vision & Multimodal Edge Diagnostics
7. Telemetry & Hardware Degradation Ladder
8. Android APK Artifact Verification
"""

import sys
import os
import io
import json
import zipfile
import pytest
from fastapi.testclient import TestClient

# UTF-8 stdout protection
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if BASE_DIR not in sys.path:
    sys.path.insert(0, BASE_DIR)

from backend.app import app
from backend.isha_engine import IshaEngine
from backend.rag.online_rag import PrivacyScrubber

client = TestClient(app)
engine = IshaEngine.get_instance()


class TestSystemEndpoints:
    def test_health_check(self):
        resp = client.get("/api/health")
        assert resp.status_code == 200
        data = resp.json()
        assert data["status"] == "online"
        assert "ISHA AI" in data["runtime"]

    def test_get_packs(self):
        resp = client.get("/api/packs")
        assert resp.status_code == 200
        packs = resp.json()
        pack_ids = [p["id"] for p in packs]
        assert "kisan_saathi" in pack_ids
        assert "swasthya" in pack_ids
        assert "bharat_core" in pack_ids
        assert "shiksha" in pack_ids
        assert "user_library" in pack_ids

    def test_get_pack_chunks(self):
        resp = client.get("/api/packs/kisan_saathi/chunks")
        assert resp.status_code == 200
        chunks = resp.json()
        assert len(chunks) > 0
        assert any("rust" in c["text"].lower() or "wheat" in c["text"].lower() for c in chunks)

    def test_telemetry_and_tier_update(self):
        # Update tier to 3
        resp = client.post("/api/telemetry/tier", json={"tier": 3})
        assert resp.status_code == 200
        data = resp.json()
        assert data["tier"] == 3
        assert "Flagship" in data["tier_label"]

        # Reset to Tier 2
        resp2 = client.post("/api/telemetry/tier", json={"tier": 2})
        assert resp2.status_code == 200
        assert resp2.json()["tier"] == 2

    def test_thermal_zone_simulation(self):
        resp = client.post("/api/telemetry/thermal", json={"zone": 3})
        assert resp.status_code == 200
        data = resp.json()
        assert data["thermal_zone"] == 3
        assert "Paused" in data["thermal_throttle"]

        # Reset to Normal (Zone 1)
        resp2 = client.post("/api/telemetry/thermal", json={"zone": 1})
        assert resp2.status_code == 200
        assert resp2.json()["thermal_zone"] == 1


class TestDocumentIngestion:
    def test_text_document_ingestion(self):
        sample_text = (
            "Soil Salinity Management in Arid Zones\n"
            "Apply gypsum at 5 tons per hectare to displace sodium ions with calcium.\n"
            "Leach the root zone with good quality canal water before sowing."
        ).encode("utf-8")

        response = client.post(
            "/api/packs/ingest",
            files={"file": ("salinity_guide.txt", sample_text, "text/plain")},
            data={"pack_id": "user_library"}
        )
        assert response.status_code == 200
        res = response.json()
        assert res["status"] in ("success", "already_indexed")
        assert res["total_chunks"] >= 1

    def test_csv_ingestion(self):
        csv_data = (
            "crop,disease,remedy\n"
            "Wheat,Yellow Rust,Propiconazole 25% EC at 1ml/L\n"
            "Rice,Blast,Tricyclazole 75% WP at 0.6g/L\n"
            "Cotton,Bollworm,Profenofos 50% EC at 2ml/L\n"
        ).encode("utf-8")

        response = client.post(
            "/api/packs/ingest",
            files={"file": ("crop_diseases.csv", csv_data, "text/csv")},
            data={"pack_id": "user_library"}
        )
        assert response.status_code == 200
        assert response.json()["status"] in ("success", "already_indexed")

    def test_duplicate_avoidance(self):
        content = b"Deterministic test content for duplicate hash avoidance test 12345."
        # First upload
        res1 = client.post(
            "/api/packs/ingest",
            files={"file": ("hash_test.txt", content, "text/plain")},
            data={"pack_id": "user_library"}
        )
        assert res1.status_code == 200
        # Second upload with identical content
        res2 = client.post(
            "/api/packs/ingest",
            files={"file": ("hash_test.txt", content, "text/plain")},
            data={"pack_id": "user_library"}
        )
        assert res2.status_code == 200
        assert res2.json()["status"] == "already_indexed"


class TestPrivacyAndOnlineRag:
    def test_pii_scrubbing(self):
        dirty_query = "My phone is 9876543210 and my email is farmer_ramesh@gmail.com. How to treat rice blast?"
        clean = PrivacyScrubber.sanitize_query(dirty_query)
        assert "9876543210" not in clean
        assert "farmer_ramesh@gmail.com" not in clean
        assert "rice blast" in clean.lower()

    def test_online_search_endpoint(self):
        resp = client.post(
            "/api/rag/online-search",
            json={"query": "photosynthesis", "max_results": 2}
        )
        assert resp.status_code == 200
        data = resp.json()
        assert "query" in data
        assert len(data["sources"]) > 0
        assert any("wikipedia" in s.get("url", "").lower() or "duckduckgo" in s.get("url", "").lower() for s in data["sources"])


class TestVisionDiagnostics:
    def test_crop_disease_diagnostic(self):
        resp = client.post(
            "/api/vision/analyze",
            json={
                "image_base64": "data:image/jpeg;base64,dGVzdA==",
                "context_hint": "yellow powder stripe lesions on wheat leaf foliage"
            }
        )
        assert resp.status_code == 200
        data = resp.json()
        assert "Yellow Rust" in data["diagnosis"]
        assert data["confidence"] > 0.85
        assert "Propiconazole" in data["remedy"]


class TestWebSocketStreaming:
    def test_websocket_chat_flow(self):
        with client.websocket_connect("/ws/chat") as ws:
            ws.send_json({
                "query": "What is the pediatric dose of paracetamol?",
                "pack_id": "swasthya",
                "use_online_rag": False,
                "history": []
            })

            received_retrieval = False
            token_chunks = []
            received_complete = False

            while True:
                raw_msg = ws.receive_text()
                msg = json.loads(raw_msg)
                if msg["type"] == "retrieval_meta":
                    received_retrieval = True
                    assert msg["data"]["local_matches"] > 0
                elif msg["type"] == "token":
                    token_chunks.append(msg["token"])
                elif msg["type"] == "complete":
                    received_complete = True
                    assert msg["stats"]["total_tokens"] > 0
                    break

            assert received_retrieval, "Did not receive retrieval metadata"
            assert len(token_chunks) > 0, "Did not receive any streamed tokens"
            assert received_complete, "Stream did not complete normally"


class TestAndroidPackageArtifacts:
    def test_apk_exists_and_valid(self):
        candidate_paths = [
            os.path.join(BASE_DIR, "releases", "isha-ai-v1.0.0.apk"),
            os.path.join(BASE_DIR, "android", "app", "build", "outputs", "apk", "debug", "app-debug.apk")
        ]
        apk_path = next((p for p in candidate_paths if os.path.exists(p)), None)
        assert apk_path is not None, "APK not found in releases/ or build outputs"
        assert os.path.getsize(apk_path) > 5 * 1024 * 1024  # > 5MB

        with zipfile.ZipFile(apk_path, "r") as z:
            names = z.namelist()
            assert "AndroidManifest.xml" in names
            assert any(n.endswith(".dex") for n in names)
            assert "lib/arm64-v8a/libisha_jni.so" in names
            assert "lib/arm64-v8a/libc++_shared.so" in names


if __name__ == "__main__":
    pytest.main(["-v", __file__])
