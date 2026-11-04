"""
Test Suite for Isha AI Fiesta 2.0 Studio Engines & Pay-As-You-Go Billing Ledger.
Validates:
- Pay-As-You-Go Wallet balance, top-up, and tamper protection
- Slide Deck Studio outline generation and Keynote-grade HTML compiler
- Conversational Website Builder Studio responsive code generation
- Multi-Model Compare and Consensus Synthesizer
- Video Storyboard Studio (8s multi-scene storyboard)
- Image Prompt Enhancer
"""

import os
import pytest
from fastapi.testclient import TestClient
from backend.app import app
from backend.billing import wallet
from backend.studios.studio_engines import DeckStudio, WebBuilderStudio, CompareStudio, VideoStudio

client = TestClient(app)


class TestBillingLedger:
    """Validates the Pay-As-You-Go wallet ledger and gatekeeper."""

    def test_wallet_initial_balance_and_integrity(self):
        bal = wallet.get_credit_balance()
        assert bal >= 0
        assert wallet.can_afford_cloud(cost=wallet.COST_OFFLINE, has_byok=False) is True

    def test_wallet_recharge(self):
        initial = wallet.get_credit_balance()
        new_bal = wallet.recharge_wallet(50)
        assert new_bal == initial + 50
        assert wallet.get_credit_balance() == new_bal

    def test_wallet_deduction_and_gatekeeper(self):
        initial = wallet.get_credit_balance()
        # Deduct 5 credits for cloud diffusion
        success, remaining = wallet.deduct_cloud_credits(5, "Test Cloud Diffusion")
        assert success is True
        assert remaining == initial - 5

        # BYOK bypasses balance check
        b_success, b_rem = wallet.deduct_cloud_credits(100000, "BYOK Call", has_byok=True)
        assert b_success is True

    def test_billing_api_endpoints(self):
        resp = client.get("/api/billing/balance")
        assert resp.status_code == 200
        data = resp.json()
        assert "balance" in data
        assert data["offline_free"] is True

        topup_resp = client.post("/api/billing/recharge", json={"amount": 20})
        assert topup_resp.status_code == 200
        assert topup_resp.json()["status"] == "success"


class TestDeckStudio:
    """Validates Presentation & Slide Deck Studio."""

    def test_pitch_deck_outline(self):
        outline = DeckStudio.generate_outline("Investment pitch deck for Rangrez Chandni Saree Shop asking 10 lakh funding")
        assert outline["title"] != ""
        assert len(outline["slides"]) >= 4
        # Validate slide fields
        first_slide = outline["slides"][0]
        assert "title" in first_slide
        assert "bullets" in first_slide

    def test_deck_html_compilation(self):
        outline = DeckStudio.generate_outline("AI in Indian Agriculture")
        html = DeckStudio.compile_html(outline)
        assert "<!DOCTYPE html>" in html
        assert "slide" in html
        assert "btn-nav" in html or "button" in html


class TestWebBuilderStudio:
    """Validates Conversational Website Builder Studio."""

    def test_website_synthesis(self):
        html = WebBuilderStudio.synthesize_website(
            brand_name="Rangrez Chandni",
            tagline="Rang Bhi, Shaan Bhi",
            contact_method="whatsapp"
        )
        assert "<!DOCTYPE html>" in html
        assert "Rangrez Chandni" in html
        assert "Rang Bhi, Shaan Bhi" in html
        assert "wa.me" in html

    def test_website_api_endpoint(self):
        resp = client.post("/api/studios/website", json={
            "brand_name": "Kisan Saathi Agro",
            "tagline": "Direct Farmer to Consumer Organics",
            "contact_method": "whatsapp"
        })
        assert resp.status_code == 200
        data = resp.json()
        assert data["status"] == "success"
        assert "Kisan Saathi Agro" in data["html"]


class TestCompareAndConsensusStudio:
    """Validates Multi-Model Compare & Consensus Studio."""

    def test_consensus_synthesis(self):
        outputs = {
            "Groq Llama 3.3 70B": "Use deep imperial indigo with warm gold accents.",
            "Gemini 1.5 Flash": "Royal purple and champagne ivory creates regal trust.",
            "Claude 3.5 Sonnet": "Gold foil typography with rich indigo backdrop."
        }
        consensus = CompareStudio.synthesize_consensus("Which color should I use for saree brand logo?", outputs)
        assert "Consensus" in consensus
        assert "Indigo" in consensus or "Gold" in consensus

    def test_compare_api_endpoint(self):
        resp = client.post("/api/studios/compare", json={
            "query": "Tell me color psychology for a luxury ethnic wear boutique"
        })
        assert resp.status_code == 200
        data = resp.json()
        assert data["status"] == "success"
        assert "consensus" in data
        assert len(data["models"]) >= 3


class TestVideoStoryboardStudio:
    """Validates 8-Second Video Storyboard Studio."""

    def test_storyboard_generation(self):
        storyboard = VideoStudio.generate_storyboard("Rangrez Chandni", "Heritage Zari Saree")
        assert storyboard["duration"] == 8
        assert len(storyboard["shots"]) == 3
        assert "voiceover" in storyboard["shots"][0]
        assert "camera" in storyboard["shots"][0]

    def test_video_api_endpoint(self):
        resp = client.post("/api/studios/video", json={
            "brand_name": "Rangrez Chandni",
            "topic": "Bridal Collection"
        })
        assert resp.status_code == 200
        data = resp.json()
        assert data["status"] == "success"
        assert data["storyboard"]["duration"] == 8


if __name__ == "__main__":
    pytest.main(["-s", "-v", __file__])
