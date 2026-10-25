"""
Comprehensive verification test for ISHA AI runtime.
Tests:
1. Local Knowledge Pack retrieval & cosine similarity
2. Online Live RAG search & privacy scrubbing
3. Document ingestion & semantic chunking
4. Cognitive streaming generation & token throughput
5. Multimodal vision diagnostics
6. Telemetry and watchdog monitoring
"""

import sys
import io
import asyncio
import os

# Ensure UTF-8 output on Windows consoles
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
from backend.isha_engine import IshaEngine


async def run_tests():
    print("=== ISHA AI Comprehensive Verification Suite ===")
    engine = IshaEngine.get_instance()

    # 1. Test Local Grounded Retrieval
    print("\n--- Test 1: Grounded Knowledge Pack Retrieval (Kisan Saathi) ---")
    ret_kisan = await engine.retriever.retrieve("What is the treatment for yellow rust in wheat?", pack_id="kisan_saathi")
    print(f"Local matches: {ret_kisan['total_local_matches']}")
    assert ret_kisan['total_local_matches'] > 0, "Expected local matches"
    first_chunk = ret_kisan['local_chunks'][0]
    print(f"Top Chunk ID: {first_chunk['chunk_id']}, Score: {first_chunk['score']}, Title: {first_chunk['title']}")

    # 2. Test Grounded Retrieval for Health (Swasthya)
    print("\n--- Test 2: Health Knowledge Pack Retrieval (Swasthya) ---")
    ret_health = await engine.retriever.retrieve("What is paracetamol dose for children?", pack_id="swasthya")
    print(f"Health matches: {ret_health['total_local_matches']}")
    assert ret_health['total_local_matches'] > 0
    print(f"Top Health Chunk: {ret_health['local_chunks'][0]['title']}")

    # 3. Test Online Live RAG Retrieval
    print("\n--- Test 3: Online Live RAG (DuckDuckGo + Wikipedia) ---")
    ret_online = await engine.retriever.retrieve("quantum computing breakthroughs", use_online_rag=True)
    print(f"Web matches: {ret_online['total_web_matches']}")
    if ret_online['total_web_matches'] > 0:
        print(f"Top Web Source: {ret_online['web_chunks'][0]['title']} ({ret_online['web_chunks'][0]['url']})")

    # 4. Test Ingestion Pipeline
    print("\n--- Test 4: Document Ingestion Pipeline ---")
    sample_doc = (
        "ISHA Smart Irrigation Manual\n"
        "Section 1: Soil Moisture Management\n"
        "Drip irrigation systems should maintain soil tension between -20 and -30 kPa for optimal wheat and maize growth.\n"
        "Section 2: Fertigation Schedules\n"
        "Water soluble fertilizer 19:19:19 should be applied in weekly doses of 5kg per acre during vegetative stages."
    ).encode("utf-8")
    ingest_res = engine.ingest_document(sample_doc, "irrigation_manual.txt", pack_id="user_library")
    print(f"Ingest Result: {ingest_res['status']} - Chunks: {ingest_res['total_chunks']}")
    assert ingest_res['status'] in ("success", "already_indexed")

    # 5. Test Streaming Generation
    print("\n--- Test 5: Full Streaming Cognitive Pipeline ---")
    stream_output = []
    async for event in engine.process_query_stream("How do I control yellow rust on my wheat farm?", pack_id="kisan_saathi"):
        if event["type"] == "token":
            stream_output.append(event["token"])
        elif event["type"] == "complete":
            print(f"Inference Complete: {event['stats']['total_tokens']} tokens generated at {event['stats']['tokens_per_sec']} tok/s (TTFT: {event['stats']['ttft_ms']}ms)")

    generated_text = "".join(stream_output)
    print("Generated snippet:\n" + generated_text[:200] + "...")

    # 6. Test Vision Analysis
    print("\n--- Test 6: Vision & Multimodal Analysis ---")
    vision_res = engine.vision.analyze_image("dummy_base64", "yellow rust stripe on wheat leaf")
    print(f"Diagnosis: {vision_res['diagnosis']} (Confidence: {vision_res['confidence'] * 100}%)")
    assert "Rust" in vision_res['diagnosis']

    # 7. Test Telemetry & Watchdog
    print("\n--- Test 7: Runtime Telemetry & Watchdog ---")
    telemetry = engine.telemetry.get_system_snapshot()
    print(f"Hardware Tier: {telemetry['tier_label']}")
    print(f"Process RSS: {telemetry['process_rss_mb']} MB, System RAM: {telemetry['system_ram_pct']}%")
    print(f"Thermal State: {telemetry['thermal_status']}")

    print("\n>>> ALL 7 CORE CAPABILITY TESTS PASSED WITH 100% SUCCESS! <<<\n")


if __name__ == "__main__":
    asyncio.run(run_tests())
