package com.isha.ai

import android.content.Context
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.util.Log
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.withContext
import org.json.JSONObject
import java.net.HttpURLConnection
import java.net.URL
import java.net.URLEncoder

object IshaImageGenerator {
    private const val TAG = "IshaImageGenerator"

    data class VisualResult(
        val bitmap: Bitmap,
        val engineName: String,
        val isOffline: Boolean,
        val prompt: String
    )

    @Volatile
    private var lastPrompt: String = "joyful people laughing in sunset"

    /**
     * Checks whether the user's prompt is requesting AI image generation.
     * Supports broad English and Hindi / Hinglish phrasing.
     */
    fun isImageGenerationIntent(prompt: String): Boolean {
        val p = prompt.lowercase().trim()
        val triggers = listOf(
            "generate image", "create image", "draw an image", "draw a",
            "draw ", "paint an image", "paint a", "paint ", "make an image",
            "make a picture", "picture of", "photo of", "image of",
            "generate visual", "ai image", "artwork of", "sketch a",
            "sketch of", "illustration of", "render a", "render image",
            "visualize", "chitra banao", "tasveer banao", "photo banao",
            "image banao", "chitra dikhao", "tasveer dikhao",
            "generate another", "generate again", "one more image", "make another",
            "draw another", "create another", "another image", "another picture",
            "ek aur photo", "ek aur chitra", "phir se banao", "fir se banao",
            "generate one more", "generate visual of"
        )

        return triggers.any { p.startsWith(it) || p.contains(it) } ||
               p.endsWith("ka chitra") ||
               p.endsWith("ki photo") ||
               p.endsWith("banao") ||
               p.endsWith("dikhao")
    }

    /**
     * Cleans prompt to extract core visual concepts for the image generator.
     * Handles follow-ups ("another", "again", "one more") by maintaining session memory.
     */
    fun extractImagePrompt(rawPrompt: String): String {
        val pTrim = rawPrompt.trim()

        // 1. Direct follow-up variation check
        val followUpPatterns = listOf(
            Regex("^(please\\s+)?(generate|create|draw|paint|make)?\\s*(another|again|one\\s+more|one\\s+more\\s+time|next)?\\s*(image|picture|photo|visual)?(\\s+again)?$", RegexOption.IGNORE_CASE),
            Regex("^(ek\\s+aur|phir\\s+se|fir\\s+se)\\s*(banao|dikhao)?$", RegexOption.IGNORE_CASE)
        )
        for (fp in followUpPatterns) {
            if (fp.matches(pTrim)) {
                return lastPrompt
            }
        }

        // 2. Iteratively strip leading directives
        var p = pTrim
        val prefixPatterns = listOf(
            Regex("^(please\\s+|can\\s+you\\s+|kindly\\s+)?(generate|create|draw|paint|make|show|render|sketch|visualize)(\\s+(me|us))?(\\s+\\b(an|a|the)\\b)?(\\s+(image|picture|photo|visual|artwork|drawing|portrait|illustration))?(\\s+of)?\\s*[:,-]?\\s*", RegexOption.IGNORE_CASE),
            Regex("^(ai\\s+)?(image|picture|photo|visual|artwork|portrait|illustration)\\s+(of)?\\s*[:,-]?\\s*", RegexOption.IGNORE_CASE),
            Regex("^(chitra|tasveer|photo|image)\\s+(banao|dikhao)\\s*[:,-]?\\s*", RegexOption.IGNORE_CASE),
            Regex("^(ek\\s+)?(chitra|tasveer|photo|image)\\s*[:,-]?\\s*", RegexOption.IGNORE_CASE)
        )

        var changed = true
        while (changed) {
            changed = false
            for (pat in prefixPatterns) {
                val newP = pat.replace(p, "").trim().replace(Regex("^[:,-]+\\s*"), "")
                if (newP != p && newP.isNotEmpty()) {
                    p = newP
                    changed = true
                }
            }
        }

        // 3. Suffixes (Hindi / Hinglish / English)
        val suffixPatterns = listOf(
            Regex("\\s+(ka\\s+chitra|ki\\s+photo|ki\\s+tasveer|chitra|photo|tasveer)\\s+(banao|dikhao)?$", RegexOption.IGNORE_CASE),
            Regex("\\s+(ka|ki)\\s+(chitra|photo|tasveer)$", RegexOption.IGNORE_CASE),
            Regex("\\s+(banao|dikhao)$", RegexOption.IGNORE_CASE),
            Regex("\\s+(ka|ki)$", RegexOption.IGNORE_CASE)
        )
        for (spat in suffixPatterns) {
            p = spat.replace(p, "").trim()
        }

        if (p.lowercase() in listOf("another", "again", "one more", "another one", "next", "more")) {
            p = lastPrompt
        } else if (p.isNotEmpty()) {
            lastPrompt = p
        } else {
            p = lastPrompt
        }

        return p
    }

    /**
     * Multi-Tier Continuous AI Image Generation Architecture.
     * Tier 1: Hugging Face Serverless Diffusion (FLUX.1 [schnell] / SDXL) if HF token configured.
     * Tier 2: AI Horde Distributed Neural Cluster (Stable Diffusion 512x512, free & continuous).
     * Tier 3: Pollinations Cloud Diffusion with dynamic seed.
     * Tier 4: Wikimedia Commons High-Resolution Visual Concept Grounding.
     * Tier 5: On-Device Neural Canvas Engine (100% Offline, zero-crash fallback).
     */
    suspend fun generateVisual(
        promptText: String,
        forceOffline: Boolean = false,
        context: Context? = null
    ): VisualResult = withContext(Dispatchers.IO) {
        val corePrompt = extractImagePrompt(promptText)

        // If forceOffline is requested, immediately use On-Device Offline Neural Canvas
        if (forceOffline) {
            val offlineBitmap = IshaOfflineImageSynthesizer.synthesizeVisual(corePrompt)
            return@withContext VisualResult(
                bitmap = offlineBitmap,
                engineName = "On-Device Neural Canvas Engine (100% Offline)",
                isOffline = true,
                prompt = corePrompt
            )
        }

        // Tier 1: Hugging Face Serverless Diffusion (FLUX.1 [schnell] or SDXL) if token is present
        if (context != null) {
            try {
                val hfToken = IshaCloudAI.getHfToken(context)
                if (hfToken.isNotEmpty()) {
                    val hfBitmap = fetchHuggingFaceDiffusion(corePrompt, hfToken)
                    if (hfBitmap != null) {
                        return@withContext VisualResult(
                            bitmap = hfBitmap,
                            engineName = "FLUX.1 [schnell] Neural Diffusion (Hugging Face)",
                            isOffline = false,
                            prompt = corePrompt
                        )
                    }
                }
            } catch (e: Exception) {
                Log.w(TAG, "Hugging Face Diffusion error: ${e.message}")
            }
        }

        // Tier 2: AI Horde Distributed Neural Cluster (Free Stable Diffusion, Continuous No-Limit)
        try {
            val hordeBitmap = fetchAiHordeDiffusion(corePrompt)
            if (hordeBitmap != null) {
                return@withContext VisualResult(
                    bitmap = hordeBitmap,
                    engineName = "Stable Diffusion AI Engine (Distributed Neural Cluster)",
                    isOffline = false,
                    prompt = corePrompt
                )
            }
        } catch (e: Exception) {
            Log.w(TAG, "AI Horde diffusion skipped: ${e.message}")
        }

        // Tier 3: Pollinations Cloud Diffusion Engine (with dynamic random seed)
        try {
            val pollinationsBitmap = fetchPollinationsVisual(corePrompt)
            if (pollinationsBitmap != null) {
                return@withContext VisualResult(
                    bitmap = pollinationsBitmap,
                    engineName = "Cloud Diffusion Engine (Pollinations)",
                    isOffline = false,
                    prompt = corePrompt
                )
            }
        } catch (e: Exception) {
            Log.w(TAG, "Pollinations diffusion skipped: ${e.message}")
        }

        // Tier 4: High-Resolution Visual Concept Grounding via Wikimedia Commons
        try {
            val wikiCommonsBitmap = fetchWikimediaCommonsVisual(corePrompt)
            if (wikiCommonsBitmap != null) {
                return@withContext VisualResult(
                    bitmap = wikiCommonsBitmap,
                    engineName = "Wikimedia High-Resolution Visual Grounding",
                    isOffline = false,
                    prompt = corePrompt
                )
            }
        } catch (e: Exception) {
            Log.w(TAG, "Wikimedia Commons fetch skipped: ${e.message}")
        }

        // Tier 5: Resilient Fallback - On-Device Offline Neural Canvas Engine
        // Runs 100% locally with zero internet, zero lag, and zero failure.
        val onDeviceBitmap = IshaOfflineImageSynthesizer.synthesizeVisual(corePrompt)
        VisualResult(
            bitmap = onDeviceBitmap,
            engineName = "On-Device Neural Canvas Engine (100% Offline)",
            isOffline = true,
            prompt = corePrompt
        )
    }

    /**
     * Queries Hugging Face Serverless Inference Router for FLUX.1 [schnell] or SDXL.
     */
    private suspend fun fetchHuggingFaceDiffusion(prompt: String, hfToken: String): Bitmap? = withContext(Dispatchers.IO) {
        val endpoints = listOf(
            "https://router.huggingface.co/hf-inference/models/black-forest-labs/FLUX.1-schnell",
            "https://router.huggingface.co/hf-inference/models/stabilityai/stable-diffusion-xl-base-1.0"
        )
        for (ep in endpoints) {
            try {
                val conn = URL(ep).openConnection() as HttpURLConnection
                conn.requestMethod = "POST"
                conn.connectTimeout = 8000
                conn.readTimeout = 15000
                conn.doOutput = true
                conn.setRequestProperty("Authorization", "Bearer $hfToken")
                conn.setRequestProperty("Content-Type", "application/json")
                conn.setRequestProperty("User-Agent", "IshaAI/1.0")

                val payload = JSONObject().apply {
                    put("inputs", prompt)
                }
                conn.outputStream.use { os ->
                    os.write(payload.toString().toByteArray(Charsets.UTF_8))
                }

                if (conn.responseCode == HttpURLConnection.HTTP_OK) {
                    val bytes = conn.inputStream.use { it.readBytes() }
                    conn.disconnect()
                    val bmp = BitmapFactory.decodeByteArray(bytes, 0, bytes.size)
                    if (bmp != null) return@withContext bmp
                } else {
                    conn.disconnect()
                }
            } catch (e: Exception) {
                Log.w(TAG, "Hugging Face router error for $ep: ${e.message}")
            }
        }
        null
    }

    /**
     * Submits an asynchronous generation job to AI Horde (Stable Diffusion cluster),
     * polls for completion, and downloads the resulting image.
     */
    private suspend fun fetchAiHordeDiffusion(prompt: String): Bitmap? = withContext(Dispatchers.IO) {
        try {
            val url = URL("https://aihorde.net/api/v2/generate/async")
            val conn = url.openConnection() as HttpURLConnection
            conn.requestMethod = "POST"
            conn.connectTimeout = 6000
            conn.readTimeout = 8000
            conn.doOutput = true
            conn.setRequestProperty("Content-Type", "application/json; charset=UTF-8")
            conn.setRequestProperty("apikey", "0000000000")
            conn.setRequestProperty("Client-Agent", "IshaAI:1.0.0:contact@isha.ai")
            conn.setRequestProperty("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) IshaAI/1.0")

            val payload = JSONObject().apply {
                put("prompt", prompt)
                put("params", JSONObject().apply {
                    put("steps", 20)
                    put("width", 512)
                    put("height", 512)
                })
                put("nsfw", false)
                put("censor_nsfw", true)
            }

            conn.outputStream.use { os ->
                os.write(payload.toString().toByteArray(Charsets.UTF_8))
            }

            if (conn.responseCode != HttpURLConnection.HTTP_OK) {
                conn.disconnect()
                return@withContext null
            }

            val respText = conn.inputStream.bufferedReader().use { it.readText() }
            conn.disconnect()

            val jobId = JSONObject(respText).optString("id", "")
            if (jobId.isEmpty()) return@withContext null

            // Poll for completion (up to 12 checks ~ 15-18s)
            var imgUrl: String? = null
            for (i in 0 until 12) {
                delay(1400)
                val checkConn = URL("https://aihorde.net/api/v2/generate/check/$jobId").openConnection() as HttpURLConnection
                checkConn.connectTimeout = 5000
                checkConn.readTimeout = 6000
                checkConn.setRequestProperty("Client-Agent", "IshaAI:1.0.0:contact@isha.ai")
                checkConn.setRequestProperty("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) IshaAI/1.0")

                if (checkConn.responseCode == HttpURLConnection.HTTP_OK) {
                    val checkJson = checkConn.inputStream.bufferedReader().use { it.readText() }
                    checkConn.disconnect()
                    val checkObj = JSONObject(checkJson)
                    if (checkObj.optBoolean("done", false)) {
                        val statusConn = URL("https://aihorde.net/api/v2/generate/status/$jobId").openConnection() as HttpURLConnection
                        statusConn.connectTimeout = 6000
                        statusConn.readTimeout = 8000
                        statusConn.setRequestProperty("Client-Agent", "IshaAI:1.0.0:contact@isha.ai")
                        statusConn.setRequestProperty("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) IshaAI/1.0")

                        if (statusConn.responseCode == HttpURLConnection.HTTP_OK) {
                            val statusJson = statusConn.inputStream.bufferedReader().use { it.readText() }
                            statusConn.disconnect()
                            val gens = JSONObject(statusJson).optJSONArray("generations")
                            if (gens != null && gens.length() > 0) {
                                imgUrl = gens.getJSONObject(0).optString("img", "")
                            }
                        } else {
                            statusConn.disconnect()
                        }
                        break
                    }
                } else {
                    checkConn.disconnect()
                }
            }

            if (!imgUrl.isNullOrEmpty()) {
                val imgConn = URL(imgUrl).openConnection() as HttpURLConnection
                imgConn.connectTimeout = 8000
                imgConn.readTimeout = 10000
                imgConn.setRequestProperty("User-Agent", "Mozilla/5.0")
                if (imgConn.responseCode == HttpURLConnection.HTTP_OK) {
                    val bytes = imgConn.inputStream.use { it.readBytes() }
                    imgConn.disconnect()
                    return@withContext BitmapFactory.decodeByteArray(bytes, 0, bytes.size)
                }
                imgConn.disconnect()
            }
        } catch (e: Exception) {
            Log.w(TAG, "AI Horde generation error: ${e.message}")
        }
        null
    }

    private fun fetchPollinationsVisual(prompt: String): Bitmap? {
        val enc = URLEncoder.encode(prompt, "UTF-8")
        val seed = (System.currentTimeMillis() % 100000).toInt()
        val urlString = "https://image.pollinations.ai/prompt/$enc?seed=$seed&nologo=true"

        val conn = URL(urlString).openConnection() as HttpURLConnection
        conn.connectTimeout = 5000
        conn.readTimeout = 8000
        conn.setRequestProperty("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) IshaAI/1.0")
        conn.connect()

        if (conn.responseCode == HttpURLConnection.HTTP_OK) {
            val bytes = conn.inputStream.use { it.readBytes() }
            conn.disconnect()
            return BitmapFactory.decodeByteArray(bytes, 0, bytes.size)
        }
        conn.disconnect()
        return null
    }

    private fun fetchWikimediaCommonsVisual(prompt: String): Bitmap? {
        val clean = prompt.replace(Regex("[^a-zA-Z0-9\\s]"), " ").trim()
        val enc = URLEncoder.encode(clean, "UTF-8")
        val urlString = "https://commons.wikimedia.org/w/api.php?action=query&generator=search&gsrnamespace=6&gsrsearch=$enc&gsrlimit=8&prop=imageinfo&iiprop=url&iiurlwidth=1024&format=json"

        val conn = URL(urlString).openConnection() as HttpURLConnection
        conn.connectTimeout = 6000
        conn.readTimeout = 8000
        conn.setRequestProperty("User-Agent", "IshaAI/1.0 (contact@isha.ai)")
        conn.connect()

        if (conn.responseCode != HttpURLConnection.HTTP_OK) {
            conn.disconnect()
            return null
        }

        val jsonText = conn.inputStream.bufferedReader().use { it.readText() }
        conn.disconnect()

        val root = JSONObject(jsonText)
        val query = root.optJSONObject("query") ?: return null
        val pages = query.optJSONObject("pages") ?: return null

        val keys = pages.keys()
        while (keys.hasNext()) {
            val page = pages.getJSONObject(keys.next())
            val title = page.optString("title", "")
            val isImageFile = title.endsWith(".jpg", true) ||
                             title.endsWith(".jpeg", true) ||
                             title.endsWith(".png", true)
            if (!isImageFile) continue

            val imageInfoArray = page.optJSONArray("imageinfo") ?: continue
            if (imageInfoArray.length() == 0) continue
            val info = imageInfoArray.getJSONObject(0)
            val imgUrl = info.optString("thumburl").ifEmpty { info.optString("url") }

            if (imgUrl.isNotEmpty() && !imgUrl.endsWith(".pdf", true) && !imgUrl.endsWith(".djvu", true)) {
                try {
                    val imgConn = URL(imgUrl).openConnection() as HttpURLConnection
                    imgConn.connectTimeout = 7000
                    imgConn.readTimeout = 9000
                    imgConn.setRequestProperty("User-Agent", "IshaAI/1.0 (contact@isha.ai)")
                    imgConn.connect()

                    if (imgConn.responseCode == HttpURLConnection.HTTP_OK) {
                        val bytes = imgConn.inputStream.use { it.readBytes() }
                        imgConn.disconnect()
                        val bmp = BitmapFactory.decodeByteArray(bytes, 0, bytes.size)
                        if (bmp != null) return bmp
                    } else {
                        imgConn.disconnect()
                    }
                } catch (e: Exception) {
                    Log.w(TAG, "Failed downloading candidate image from Wikimedia: ${e.message}")
                }
            }
        }
        return null
    }
}
