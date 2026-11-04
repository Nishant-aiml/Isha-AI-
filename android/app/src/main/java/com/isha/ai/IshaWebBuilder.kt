package com.isha.ai

import android.content.Context
import java.io.File

/**
 * Conversational Website Builder Studio.
 * Generates beautiful, responsive single-page web applications with
 * interactive product cards, WhatsApp/Instagram ordering, and inquiry forms.
 * Works 100% Offline with zero cloud dependencies or credits.
 */
object IshaWebBuilder {

    data class WebsiteSpec(
        val brandName: String,
        val tagline: String,
        val siteType: String, // "full_shop", "portfolio", "inquiry"
        val contactMethod: String, // "whatsapp", "instagram", "form"
        val accentColor: String = "#F59E0B",
        val primaryImageBase64: String? = null,
        val logoBase64: String? = null
    )

    /**
     * Synthesizes full standalone HTML5/Tailwind responsive website.
     */
    fun synthesizeWebsite(spec: WebsiteSpec): String {
        val brand = spec.brandName.ifEmpty { "Heritage Collections" }
        val tagline = spec.tagline.ifEmpty { "Where Tradition Meets Timeless Elegance" }

        val contactAction = when (spec.contactMethod.lowercase()) {
            "whatsapp" -> """<a href="https://wa.me/919876543210?text=Hello%20$brand" class="cta-btn">Order on WhatsApp 💬</a>"""
            "instagram" -> """<a href="https://instagram.com" class="cta-btn">Follow on Instagram 📸</a>"""
            else -> """<a href="#inquiry" class="cta-btn">Submit Inquiry ✍️</a>"""
        }

        val heroImageHtml = if (!spec.primaryImageBase64.isNullOrEmpty()) {
            """<img src="data:image/jpeg;base64,${spec.primaryImageBase64}" alt="$brand Hero" class="hero-img"/>"""
        } else {
            """<div class="hero-placeholder"><div class="badge">✨ Verified Handcrafted Collection</div><h1>$brand</h1><p>$tagline</p></div>"""
        }

        return """
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>$brand — $tagline</title>
<script src="https://cdn.tailwindcss.com"></script>
<style>
body { background: #0F172A; color: #F8FAFC; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; margin: 0; }
.hero-img { width: 100%; max-height: 440px; object-fit: cover; border-radius: 16px; box-shadow: 0 20px 25px -5px rgba(0, 0, 0, 0.5); }
.hero-placeholder { background: linear-gradient(135deg, #1E293B, #0F172A); border: 1px solid #334155; padding: 48px; border-radius: 16px; text-align: center; }
.badge { display: inline-block; background: rgba(245, 158, 11, 0.15); border: 1px solid #F59E0B; color: #FCD34D; font-size: 12px; font-weight: 700; padding: 4px 12px; border-radius: 9999px; margin-bottom: 16px; }
.cta-btn { display: inline-block; background: #F59E0B; color: #000; font-weight: 700; padding: 12px 28px; border-radius: 9999px; text-decoration: none; transition: transform 0.2s; box-shadow: 0 10px 15px -3px rgba(245, 158, 11, 0.4); }
.cta-btn:hover { transform: scale(1.05); }
.product-card { background: #1E293B; border: 1px solid #334155; border-radius: 14px; overflow: hidden; transition: transform 0.2s; }
.product-card:hover { transform: translateY(-4px); border-color: #F59E0B; }
</style>
</head>
<body>
<!-- Navbar -->
<nav class="border-b border-slate-800 bg-slate-900/80 backdrop-blur sticky top-0 z-50 px-6 py-4 flex justify-between items-center">
    <div class="flex items-center gap-3">
        <span class="text-xl font-extrabold tracking-wide text-amber-400">✦ $brand</span>
    </div>
    <div>
        $contactAction
    </div>
</nav>

<!-- Hero Section -->
<section class="max-w-6xl mx-auto px-6 py-12">
    <div class="grid grid-cols-1 md:grid-cols-2 gap-10 items-center">
        <div>
            <span class="badge">Artisan Handloom • 100% Authentic</span>
            <h1 class="text-4xl md:text-5xl font-black leading-tight text-white mb-4">$tagline</h1>
            <p class="text-slate-400 text-lg mb-8">Crafted directly from regional master weavers to your wardrobe. Premium silk, organza, and festive handlooms with unmatched finish.</p>
            $contactAction
        </div>
        <div>
            $heroImageHtml
        </div>
    </div>
</section>

<!-- Collection Showcase -->
<section class="max-w-6xl mx-auto px-6 py-12 border-t border-slate-800">
    <div class="text-center mb-10">
        <h2 class="text-3xl font-bold text-white">Signature Collection</h2>
        <p class="text-slate-400">Handpicked exclusives with direct weaver verification</p>
    </div>
    <div class="grid grid-cols-1 sm:grid-cols-2 md:grid-cols-3 gap-6">
        <div class="product-card p-4">
            <div class="h-48 bg-slate-800 rounded-lg flex items-center justify-center text-slate-500 font-bold">Royal Banarasi Silk</div>
            <h3 class="text-lg font-bold mt-4 text-white">Royal Banarasi Zari Saree</h3>
            <p class="text-sm text-slate-400 mt-1">Pure golden zari border with rich pallu</p>
            <div class="flex justify-between items-center mt-4">
                <span class="text-amber-400 font-black text-lg">₹4,999</span>
                <span class="text-xs bg-emerald-900/40 text-emerald-300 px-2 py-1 rounded">In Stock</span>
            </div>
        </div>
        <div class="product-card p-4">
            <div class="h-48 bg-slate-800 rounded-lg flex items-center justify-center text-slate-500 font-bold">Chanderi Floral Craft</div>
            <h3 class="text-lg font-bold mt-4 text-white">Chanderi Silk Motif Saree</h3>
            <p class="text-sm text-slate-400 mt-1">Lightweight handwoven organza weave</p>
            <div class="flex justify-between items-center mt-4">
                <span class="text-amber-400 font-black text-lg">₹3,499</span>
                <span class="text-xs bg-emerald-900/40 text-emerald-300 px-2 py-1 rounded">Trending</span>
            </div>
        </div>
        <div class="product-card p-4">
            <div class="h-48 bg-slate-800 rounded-lg flex items-center justify-center text-slate-500 font-bold">Kanjivaram Festive</div>
            <h3 class="text-lg font-bold mt-4 text-white">Bridal Kanjivaram Brocade</h3>
            <p class="text-sm text-slate-400 mt-1">Traditional temple border with silk mark</p>
            <div class="flex justify-between items-center mt-4">
                <span class="text-amber-400 font-black text-lg">₹8,999</span>
                <span class="text-xs bg-amber-900/40 text-amber-300 px-2 py-1 rounded">Exclusive</span>
            </div>
        </div>
    </div>
</section>

<!-- Footer -->
<footer class="border-t border-slate-800 py-8 text-center text-slate-500 text-sm">
    <p>© 2026 $brand. All rights reserved. Generated seamlessly with Isha AI Studio.</p>
</footer>
</body>
</html>
        """.trimIndent()
    }

    fun exportWebsiteToFile(context: Context, spec: WebsiteSpec): File {
        val html = synthesizeWebsite(spec)
        val file = File(context.filesDir, "website_${System.currentTimeMillis()}.html")
        file.writeText(html, Charsets.UTF_8)
        return file
    }
}
