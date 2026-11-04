package com.isha.ai

import android.content.Context
import org.json.JSONArray
import org.json.JSONObject
import java.io.File

/**
 * Slide & Presentation Deck Studio.
 * Generates structured presentation outlines and compiles them into
 * professional interactive HTML presentations (Keynote style) and slide documents.
 * Works 100% Offline with zero cloud dependencies or credits.
 */
object IshaDeckEngine {

    data class Slide(
        val title: String,
        val subtitle: String,
        val bullets: List<String>,
        val metrics: Map<String, String>? = null,
        val callToAction: String? = null
    )

    data class Deck(
        val deckTitle: String,
        val targetAudience: String,
        val slides: List<Slide>
    )

    /**
     * Stage 1: Generates an editable slide deck outline from prompt.
     */
    fun generateDeckOutline(topic: String, contextInfo: String = ""): Deck {
        val cleanTopic = topic.trim()
        val isBusinessPitch = cleanTopic.contains("pitch", true) ||
                              cleanTopic.contains("funding", true) ||
                              cleanTopic.contains("investor", true) ||
                              cleanTopic.contains("shop", true) ||
                              cleanTopic.contains("business", true)

        val slides = mutableListOf<Slide>()

        if (isBusinessPitch) {
            slides.add(Slide(
                title = "Executive Summary",
                subtitle = "The New Standard in Accessible Excellence",
                bullets = listOf(
                    "High-demand consumer segment targeting middle & upper-middle markets",
                    "Direct artisan-to-consumer sourcing with 45%+ gross margin profiles",
                    "Omnichannel presence: Boutique flagship coupled with digital ordering"
                ),
                metrics = mapOf("Gross Margin" to "45%", "Target Segment" to "Middle / Upper-Middle", "Unit Economics" to "Profitable Day 1")
            ))
            slides.add(Slide(
                title = "Market Opportunity & Problem",
                subtitle = "Bridging the Authenticity & Price Gap",
                bullets = listOf(
                    "Customers face exorbitant designer markups or substandard counterfeit alternatives",
                    "Growing consumer appetite for verified handcrafted heritage collections",
                    "Total Addressable Market expanding at 14.2% CAGR year-over-year"
                ),
                metrics = mapOf("Market Size" to "₹82,000 Cr", "CAGR Growth" to "14.2%", "Brand Loyalty" to "High Repeat Rate")
            ))
            slides.add(Slide(
                title = "Product Strategy & Value Proposition",
                subtitle = "Signature Collections with Verifiable Quality",
                bullets = listOf(
                    "Exquisite handloom & modern fusion aesthetics for festivals & weddings",
                    "AI-driven virtual model previews saving 90% in visual catalog costs",
                    "Fast inventory turnaround with local regional fulfillment"
                )
            ))
            slides.add(Slide(
                title = "Financial Ask & Allocation",
                subtitle = "Target Capital: ₹10,00,000 for Hyper-Local Expansion",
                bullets = listOf(
                    "40% (₹4.0 Lakhs): Artisan procurement & signature inventory buildup",
                    "30% (₹3.0 Lakhs): Storefront refinement & interactive digital catalog",
                    "20% (₹2.0 Lakhs): Targeted regional brand awareness & launch events",
                    "10% (₹1.0 Lakhs): Operational contingency & working capital"
                ),
                metrics = mapOf("Target Funding" to "₹10,00,000", "Runway" to "14 Months", "Break-Even" to "Month 7")
            ))
            slides.add(Slide(
                title = "Roadmap & Milestone Targets",
                subtitle = "Disciplined Execution Over 12 Months",
                bullets = listOf(
                    "Q1: Flagship boutique launch and initial 500 loyal member acquisitions",
                    "Q2: Digital catalog rollout with WhatsApp conversational ordering",
                    "Q3: Expansion to tier-2 bridal collections and regional tie-ups",
                    "Q4: Reinvestment in secondary workshop and sustainable packaging"
                ),
                callToAction = "Join us in redefining heritage retail with modern AI efficiency."
            ))
        } else {
            // General / Educational / Scientific Presentation
            slides.add(Slide(
                title = cleanTopic.replaceFirstChar { it.uppercase() },
                subtitle = "Comprehensive Overview & Analysis",
                bullets = listOf(
                    "Fundamental principles and situational background",
                    "Current trends, critical challenges, and emerging solutions",
                    "Actionable recommendations for practical implementation"
                )
            ))
            slides.add(Slide(
                title = "Key Pillars & Findings",
                subtitle = "Core Dynamics Explored",
                bullets = listOf(
                    "Pillar 1: Structural foundations and core mechanisms",
                    "Pillar 2: Performance metrics, compliance, and efficiency factors",
                    "Pillar 3: Long-term viability and strategic risk mitigation"
                )
            ))
            slides.add(Slide(
                title = "Strategic Roadmap",
                subtitle = "Next Steps and Implementation Milestones",
                bullets = listOf(
                    "Phase 1: Immediate deployment of priority recommendations",
                    "Phase 2: Validation, monitoring, and telemetry evaluation",
                    "Phase 3: Scaled adoption and continuous refinement"
                ),
                callToAction = "Empowering sustainable growth with intelligent edge solutions."
            ))
        }

        return Deck(
            deckTitle = cleanTopic.replaceFirstChar { it.uppercase() },
            targetAudience = if (isBusinessPitch) "Investors & Strategic Partners" else "General Stakeholders",
            slides = slides
        )
    }

    /**
     * Stage 2: Compiles Deck into a standalone interactive HTML Presentation
     * featuring Keynote-grade animations and left/right keyboard navigation.
     */
    fun compileToInteractiveHtml(deck: Deck): String {
        val slidesHtml = StringBuilder()

        deck.slides.forEachIndexed { index, s ->
            val metricsHtml = if (!s.metrics.isNullOrEmpty()) {
                val items = s.metrics.map { (k, v) ->
                    """<div class="metric-card"><div class="metric-value">$v</div><div class="metric-label">$k</div></div>"""
                }.joinToString("")
                """<div class="metrics-grid">$items</div>"""
            } else ""

            val bulletsHtml = s.bullets.joinToString("") { b ->
                """<li><span class="bullet-dot">✦</span> <span>$b</span></li>"""
            }

            val ctaHtml = if (!s.callToAction.isNullOrEmpty()) {
                """<div class="cta-banner">$s.callToAction</div>"""
            } else ""

            slidesHtml.append("""
                <div class="slide ${if (index == 0) "active" else ""}" id="slide-$index">
                    <div class="slide-header">
                        <span class="slide-num">Slide ${index + 1} of ${deck.slides.size}</span>
                        <span class="deck-tag">${deck.targetAudience}</span>
                    </div>
                    <h2 class="slide-title">${s.title}</h2>
                    <h3 class="slide-subtitle">${s.subtitle}</h3>
                    <ul class="bullets-list">$bulletsHtml</ul>
                    $metricsHtml
                    $ctaHtml
                </div>
            """.trimIndent())
        }

        return """
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>${deck.deckTitle}</title>
<style>
* { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
body { background: #0B0F19; color: #F1F5F9; min-height: 100vh; display: flex; flex-direction: column; align-items: center; justify-content: center; overflow: hidden; }
.deck-container { width: 90vw; max-width: 960px; height: 560px; background: #111827; border: 1px solid #1F2937; border-radius: 16px; position: relative; box-shadow: 0 25px 50px -12px rgba(0, 0, 0, 0.7); display: flex; flex-direction: column; }
.slide { display: none; padding: 48px; height: 100%; flex-direction: column; justify-content: space-between; opacity: 0; transition: opacity 0.3s ease; }
.slide.active { display: flex; opacity: 1; }
.slide-header { display: flex; justify-content: space-between; font-size: 11px; text-transform: uppercase; color: #94A3B8; letter-spacing: 1px; }
.slide-title { font-size: 32px; font-weight: 800; color: #F8FAFC; margin-top: 12px; }
.slide-subtitle { font-size: 16px; color: #F59E0B; margin-bottom: 24px; font-weight: 500; }
.bullets-list { list-style: none; display: flex; flex-direction: column; gap: 14px; }
.bullets-list li { display: flex; align-items: flex-start; gap: 12px; font-size: 16px; line-height: 1.5; color: #CBD5E1; }
.bullet-dot { color: #F59E0B; font-size: 14px; }
.metrics-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(160px, 1fr)); gap: 14px; margin-top: 20px; }
.metric-card { background: #1E293B; border: 1px solid #334155; border-radius: 10px; padding: 14px; text-align: center; }
.metric-value { font-size: 22px; font-weight: 700; color: #10B981; }
.metric-label { font-size: 12px; color: #94A3B8; margin-top: 4px; }
.cta-banner { margin-top: 20px; background: linear-gradient(135deg, rgba(245, 158, 11, 0.2), rgba(16, 185, 129, 0.2)); border: 1px solid #F59E0B; padding: 12px 18px; border-radius: 8px; font-size: 14px; color: #FCD34D; font-weight: 600; text-align: center; }
.controls { position: absolute; bottom: 18px; right: 24px; display: flex; gap: 10px; align-items: center; }
.btn-nav { background: #374151; color: #FFF; border: none; padding: 8px 16px; border-radius: 8px; font-size: 13px; font-weight: 600; cursor: pointer; transition: background 0.2s; }
.btn-nav:hover { background: #F59E0B; color: #000; }
.counter { font-size: 12px; color: #94A3B8; }
</style>
</head>
<body>
<div class="deck-container">
    $slidesHtml
    <div class="controls">
        <span class="counter" id="slideIndicator">1 / ${deck.slides.size}</span>
        <button class="btn-nav" onclick="prevSlide()">◀ Previous</button>
        <button class="btn-nav" onclick="nextSlide()">Next ▶</button>
    </div>
</div>
<script>
let current = 0;
const slides = document.querySelectorAll('.slide');
const total = slides.length;
function show(idx) {
    slides.forEach((s, i) => s.classList.toggle('active', i === idx));
    document.getElementById('slideIndicator').innerText = (idx + 1) + ' / ' + total;
}
function nextSlide() { if (current < total - 1) { current++; show(current); } }
function prevSlide() { if (current > 0) { current--; show(current); } }
document.addEventListener('keydown', (e) => {
    if (e.key === 'ArrowRight' || e.key === ' ') nextSlide();
    if (e.key === 'ArrowLeft') prevSlide();
});
</script>
</body>
</html>
        """.trimIndent()
    }

    /**
     * Saves presentation HTML to local storage for instant offline viewing or export.
     */
    fun exportDeckToFile(context: Context, deck: Deck): File {
        val html = compileToInteractiveHtml(deck)
        val file = File(context.filesDir, "deck_${System.currentTimeMillis()}.html")
        file.writeText(html, Charsets.UTF_8)
        return file
    }
}
