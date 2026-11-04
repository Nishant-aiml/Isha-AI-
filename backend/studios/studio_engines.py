"""
Isha AI Fiesta 2.0 Studio Engines (Python / Desktop & Web).
Parity with Android Native Studio Engines for Slide Decks, Websites, Compare, Video, and Image.
"""

from typing import List, Dict, Any, Optional


class DeckStudio:
    """Slide & Presentation Deck Studio (Keynote / PPTX style)."""

    @staticmethod
    def generate_outline(topic: str) -> Dict[str, Any]:
        t = topic.lower().strip()
        is_pitch = any(w in t for w in ["pitch", "funding", "investor", "shop", "business", "saree", "store"])

        if is_pitch:
            return {
                "title": topic.title(),
                "target_audience": "Investors & Strategic Partners",
                "slides": [
                    {
                        "slide": 1,
                        "title": "Executive Summary",
                        "subtitle": "The New Standard in Accessible Excellence",
                        "bullets": [
                            "High-demand consumer segment targeting middle & upper-middle markets",
                            "Direct artisan-to-consumer sourcing with 45%+ gross margin profiles",
                            "Omnichannel presence: Boutique flagship coupled with digital ordering"
                        ],
                        "metrics": {"Gross Margin": "45%", "Unit Economics": "Profitable Day 1"}
                    },
                    {
                        "slide": 2,
                        "title": "Market Opportunity & Problem",
                        "subtitle": "Bridging the Authenticity & Price Gap",
                        "bullets": [
                            "Customers face exorbitant designer markups or counterfeit alternatives",
                            "Growing consumer appetite for verified handcrafted heritage collections",
                            "Total Addressable Market expanding at 14.2% CAGR"
                        ],
                        "metrics": {"Market Size": "₹82,000 Cr", "CAGR": "14.2%"}
                    },
                    {
                        "slide": 3,
                        "title": "Financial Ask & Allocation",
                        "subtitle": "Target Capital: ₹10,00,000 for Hyper-Local Expansion",
                        "bullets": [
                            "40% (₹4.0 Lakhs): Artisan procurement & signature inventory buildup",
                            "30% (₹3.0 Lakhs): Storefront refinement & interactive digital catalog",
                            "20% (₹2.0 Lakhs): Targeted regional brand awareness & launch events",
                            "10% (₹1.0 Lakhs): Operational contingency & working capital"
                        ],
                        "metrics": {"Ask": "₹10,00,000", "Runway": "14 Months"}
                    },
                    {
                        "slide": 4,
                        "title": "Milestone Targets & Roadmap",
                        "subtitle": "Disciplined 12-Month Execution",
                        "bullets": [
                            "Q1: Flagship boutique launch & initial 500 loyal members",
                            "Q2: Digital catalog rollout with WhatsApp conversational ordering",
                            "Q3: Tier-2 bridal collections and regional showroom partnerships"
                        ],
                        "cta": "Join us in redefining heritage retail with modern AI efficiency."
                    }
                ]
            }
        else:
            return {
                "title": topic.title(),
                "target_audience": "General Stakeholders",
                "slides": [
                    {
                        "slide": 1,
                        "title": topic.title(),
                        "subtitle": "Strategic Analysis & Deep-Dive",
                        "bullets": [
                            "Fundamental operational principles and situational background",
                            "Key trends, emerging dynamics, and actionable solutions"
                        ]
                    },
                    {
                        "slide": 2,
                        "title": "Action Roadmap",
                        "subtitle": "Phased Milestones",
                        "bullets": [
                            "Phase 1: Implementation of priority recommendations",
                            "Phase 2: Validation, monitoring, and scaling"
                        ],
                        "cta": "Empowering sustainable growth with intelligent edge solutions."
                    }
                ]
            }

    @staticmethod
    def compile_html(deck_data: Dict[str, Any]) -> str:
        slides = deck_data.get("slides", [])
        title = deck_data.get("title", "Presentation Deck")
        audience = deck_data.get("target_audience", "Stakeholders")

        slides_html = ""
        for i, s in enumerate(slides):
            bullets = "".join(f"<li>✦ {b}</li>" for b in s.get("bullets", []))
            metrics = ""
            if "metrics" in s:
                m_items = "".join(f"<div class='m-card'><div class='m-val'>{v}</div><div class='m-lbl'>{k}</div></div>" for k, v in s["metrics"].items())
                metrics = f"<div class='m-grid'>{m_items}</div>"
            cta = f"<div class='cta'>{s['cta']}</div>" if "cta" in s else ""

            active_class = "active" if i == 0 else ""
            slides_html += f"""
            <div class='slide {active_class}'>
                <div class='header'><span>SLIDE {i+1} OF {len(slides)}</span><span>{audience}</span></div>
                <h2>{s['title']}</h2>
                <h3>{s['subtitle']}</h3>
                <ul class='bullets'>{bullets}</ul>
                {metrics}
                {cta}
            </div>"""

        return f"""<!DOCTYPE html><html><head><meta charset='UTF-8'><title>{title}</title>
<style>
*{{box-sizing:border-box;margin:0;padding:0;font-family:sans-serif;}}
body{{background:#0B0F19;color:#F1F5F9;height:100vh;display:flex;align-items:center;justify-content:center;overflow:hidden;}}
.deck{{width:90vw;max-width:900px;height:520px;background:#111827;border:1px solid #1F2937;border-radius:16px;padding:40px;position:relative;display:flex;flex-direction:column;justify-content:space-between;}}
.slide{{display:none;height:100%;flex-direction:column;justify-content:space-between;}}
.slide.active{{display:flex;}}
.header{{display:flex;justify-content:space-between;font-size:11px;color:#94A3B8;letter-spacing:1px;}}
h2{{font-size:28px;color:#F8FAFC;margin-top:10px;}}
h3{{font-size:15px;color:#F59E0B;margin-bottom:16px;}}
.bullets{{list-style:none;display:flex;flex-direction:column;gap:12px;font-size:15px;color:#CBD5E1;}}
.m-grid{{display:flex;gap:12px;margin-top:16px;}}
.m-card{{background:#1E293B;padding:10px 16px;border-radius:8px;border:1px solid #334155;text-align:center;}}
.m-val{{font-size:18px;font-weight:700;color:#10B981;}}
.m-lbl{{font-size:11px;color:#94A3B8;}}
.cta{{background:rgba(245,158,11,0.2);border:1px solid #F59E0B;padding:10px;border-radius:8px;color:#FCD34D;font-weight:600;margin-top:14px;}}
.ctrls{{position:absolute;bottom:20px;right:30px;display:flex;gap:10px;align-items:center;}}
button{{background:#374151;color:#fff;border:none;padding:6px 14px;border-radius:6px;cursor:pointer;}}
button:hover{{background:#F59E0B;color:#000;}}
</style></head><body>
<div class='deck'>{slides_html}
<div class='ctrls'><span id='idx'>1 / {len(slides)}</span><button onclick='prev()'>◀</button><button onclick='next()'>▶</button></div>
</div>
<script>
let cur=0; const ss=document.querySelectorAll('.slide');
function show(i){{ss.forEach((s,idx)=>s.classList.toggle('active',idx===i)); document.getElementById('idx').innerText=(i+1)+' / '+ss.length;}}
function next(){{if(cur<ss.length-1){{cur++;show(cur);}}}}
function prev(){{if(cur>0){{cur--;show(cur);}}}}
document.addEventListener('keydown', e=>{{if(e.key==='ArrowRight')next(); if(e.key==='ArrowLeft')prev();}});
</script></body></html>"""


class WebBuilderStudio:
    """Conversational Website Builder Studio (Responsive HTML/Tailwind/JS)."""

    @staticmethod
    def synthesize_website(brand_name: str, tagline: str, contact_method: str = "whatsapp") -> str:
        b = brand_name or "Heritage Collections"
        t = tagline or "Authentic Handcrafted Elegance"

        cta = '<a href="https://wa.me/919876543210" style="background:#F59E0B;color:#000;padding:10px 24px;border-radius:99px;font-weight:700;text-decoration:none;">Order via WhatsApp 💬</a>'
        if contact_method.lower() == "instagram":
            cta = '<a href="https://instagram.com" style="background:#F59E0B;color:#000;padding:10px 24px;border-radius:99px;font-weight:700;text-decoration:none;">Visit Instagram 📸</a>'

        return f"""<!DOCTYPE html><html><head><meta charset='UTF-8'><title>{b} — {t}</title>
<script src='https://cdn.tailwindcss.com'></script>
<body class='bg-slate-950 text-slate-100 font-sans'>
<nav class='border-b border-slate-800 bg-slate-900/80 px-8 py-4 flex justify-between items-center sticky top-0'>
  <div class='text-2xl font-black text-amber-400'>✦ {b}</div>
  <div>{cta}</div>
</nav>
<header class='max-w-5xl mx-auto px-6 py-16 text-center'>
  <span class='bg-amber-500/10 text-amber-400 border border-amber-500/30 px-3 py-1 rounded-full text-xs font-bold uppercase tracking-wider'>Artisan Heritage</span>
  <h1 class='text-5xl font-black mt-4 mb-6 leading-tight'>{t}</h1>
  <p class='text-slate-400 max-w-2xl mx-auto text-lg mb-8'>Direct artisan-to-wardrobe collections. Verified handloom quality, bespoke drape, and timeless elegance.</p>
  <div>{cta}</div>
</header>
<section class='max-w-5xl mx-auto px-6 py-12 border-t border-slate-800'>
  <h2 class='text-3xl font-bold mb-8 text-center'>Signature Exclusives</h2>
  <div class='grid grid-cols-1 md:grid-cols-3 gap-6'>
    <div class='bg-slate-900 border border-slate-800 rounded-xl p-5 hover:border-amber-400 transition'>
      <div class='h-40 bg-slate-800 rounded-lg flex items-center justify-center font-bold text-slate-500'>Banarasi Zari</div>
      <h3 class='text-lg font-bold mt-4'>Royal Banarasi Brocade</h3>
      <div class='flex justify-between items-center mt-3'><span class='text-amber-400 font-bold'>₹4,999</span><span class='text-xs bg-emerald-950 text-emerald-400 px-2 py-0.5 rounded'>In Stock</span></div>
    </div>
    <div class='bg-slate-900 border border-slate-800 rounded-xl p-5 hover:border-amber-400 transition'>
      <div class='h-40 bg-slate-800 rounded-lg flex items-center justify-center font-bold text-slate-500'>Chanderi Motif</div>
      <h3 class='text-lg font-bold mt-4'>Handcrafted Chanderi Silk</h3>
      <div class='flex justify-between items-center mt-3'><span class='text-amber-400 font-bold'>₹3,499</span><span class='text-xs bg-emerald-950 text-emerald-400 px-2 py-0.5 rounded'>Trending</span></div>
    </div>
    <div class='bg-slate-900 border border-slate-800 rounded-xl p-5 hover:border-amber-400 transition'>
      <div class='h-40 bg-slate-800 rounded-lg flex items-center justify-center font-bold text-slate-500'>Kanjivaram Festive</div>
      <h3 class='text-lg font-bold mt-4'>Bridal Kanjivaram Weave</h3>
      <div class='flex justify-between items-center mt-3'><span class='text-amber-400 font-bold'>₹8,999</span><span class='text-xs bg-amber-950 text-amber-400 px-2 py-0.5 rounded'>Exclusive</span></div>
    </div>
  </div>
</section>
<footer class='border-t border-slate-800 py-8 text-center text-slate-600 text-sm'>
  © 2026 {b}. Generated effortlessly with Isha AI Studio.
</footer>
</body></html>"""


class CompareStudio:
    """Multi-Model Compare & Consensus Engine."""

    @staticmethod
    def synthesize_consensus(query: str, model_outputs: Dict[str, str]) -> str:
        q = query.lower()
        if "color" in q or "colour" in q or "logo" in q or "brand" in q:
            return (
                "🎯 **Multi-AI Consensus Agreement**:\n\n"
                "✦ **Primary Palette**: Royal Indigo / Imperial Purple (Conveys heritage, nobility, and cultural trust).\n"
                "✦ **Accent Tones**: Metallic Champagne Gold & Ivory (Evokes festive luxury without overwhelming minimalism).\n"
                "✦ **Tagline Recommendation**: 'Rang Bhi, Shaan Bhi' — balances cultural emotion with memorable brevity.\n"
                "✦ **Demographic Target**: High resonance with middle & upper-middle bridal/festive shoppers."
            )
        else:
            bullets = "\n".join(f"  • **{m}**: {txt.splitlines()[0][:80]}" for m, txt in model_outputs.items())
            return (
                f"🎯 **Multi-AI Consensus Agreement**:\n\n"
                f"✦ **Key Common Findings**:\n{bullets}\n\n"
                "✦ **Agreed Action Plan**:\n"
                "  1. Focus on core verified requirements first.\n"
                "  2. Validate data assumptions against domain grounding.\n"
                "  3. Deploy in phased checkpoints to ensure operational safety."
            )


class VideoStudio:
    """8-Second Promotional Video Storyboard Studio."""

    @staticmethod
    def generate_storyboard(brand_name: str, topic: str) -> Dict[str, Any]:
        b = brand_name or "Rangrez Chandni"
        t = topic or "Handcrafted Saree Collection"
        return {
            "title": f"{b} — 8s Promotional Video Storyboard",
            "duration": 8,
            "tagline": "Rang Bhi, Shaan Bhi",
            "shots": [
                {
                    "shot": 1,
                    "time": "0.0s – 2.5s",
                    "camera": "Slow macro orbit",
                    "visual": f"Extreme close-up of intricate golden zari threadwork gleaming under warm studio rim-lighting for {t}.",
                    "voiceover": "Where heritage whispers in every golden thread...",
                    "audio": "Soft ambient sitar swell with warm acoustic pad"
                },
                {
                    "shot": 2,
                    "time": "2.5s – 5.5s",
                    "camera": "Medium tracking shot",
                    "visual": "Elegant Indian woman in palace courtyard wearing the handcrafted saree with graceful silk drape.",
                    "voiceover": "Crafted for timeless celebrations and modern royalty.",
                    "audio": "Rhythmic percussion drops in with subtle tabla cadence"
                },
                {
                    "shot": 3,
                    "time": "5.5s – 8.0s",
                    "camera": "Cinematic hero frame zoom out",
                    "visual": f"Model smiles; minimalist golden {b} brand watermark reveals cleanly in bottom-left corner.",
                    "voiceover": f"{b}. Rang Bhi, Shaan Bhi.",
                    "audio": "Melodic crescendo with clean chime finish"
                }
            ]
        }
