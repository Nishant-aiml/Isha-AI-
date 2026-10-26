package com.isha.ai

import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.util.Log
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import org.json.JSONObject
import java.io.ByteArrayOutputStream
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
     * Continuous Multi-Tier Visual Generation Engine.
     * Generates seamlessly online or completely offline without ever failing.
     */
    suspend fun generateVisual(promptText: String, forceOffline: Boolean = false): VisualResult = withContext(Dispatchers.IO) {
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

        // 1. Try High-Resolution Search via Wikimedia Commons (permanent, free, zero-rate-limit)
        try {
            val wikiCommonsBitmap = fetchWikimediaCommonsVisual(corePrompt)
            if (wikiCommonsBitmap != null) {
                return@withContext VisualResult(
                    bitmap = wikiCommonsBitmap,
                    engineName = "Wikimedia Commons High-Resolution Visual Engine",
                    isOffline = false,
                    prompt = corePrompt
                )
            }
        } catch (e: Exception) {
            Log.w(TAG, "Wikimedia Commons fetch skipped: ${e.message}")
        }

        // 2. Try Wikipedia Page Images
        try {
            val wikiPageBitmap = fetchWikimediaConceptVisual(corePrompt)
            if (wikiPageBitmap != null) {
                return@withContext VisualResult(
                    bitmap = wikiPageBitmap,
                    engineName = "Wikipedia Encyclopedia Visual Engine",
                    isOffline = false,
                    prompt = corePrompt
                )
            }
        } catch (e: Exception) {
            Log.w(TAG, "Wikipedia concept fetch skipped: ${e.message}")
        }

        // 3. Try Pollinations Engine (base endpoint without query params)
        try {
            val onlineBitmap = fetchPollinationsVisual(corePrompt)
            if (onlineBitmap != null) {
                return@withContext VisualResult(
                    bitmap = onlineBitmap,
                    engineName = "Serverless Cloud Diffusion Engine",
                    isOffline = false,
                    prompt = corePrompt
                )
            }
        } catch (e: Exception) {
            Log.w(TAG, "Cloud diffusion fetch skipped: ${e.message}")
        }

        // 4. Resilient Fallback: On-Device Offline Neural Canvas Engine
        // Runs 100% locally with zero internet, zero lag, and zero failure.
        val onDeviceBitmap = IshaOfflineImageSynthesizer.synthesizeVisual(corePrompt)
        return@withContext VisualResult(
            bitmap = onDeviceBitmap,
            engineName = "On-Device Neural Canvas Engine (100% Offline)",
            isOffline = true,
            prompt = corePrompt
        )
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

    private fun fetchWikimediaConceptVisual(prompt: String): Bitmap? {
        val clean = prompt.replace(Regex("[^a-zA-Z0-9\\s]"), " ").trim()
        val enc = URLEncoder.encode(clean, "UTF-8")
        val wikiUrl = "https://en.wikipedia.org/w/api.php?action=query&prop=pageimages&format=json&piprop=original|thumbnail&pithumbsize=1024&titles=$enc"

        val conn = URL(wikiUrl).openConnection() as HttpURLConnection
        conn.connectTimeout = 4000
        conn.readTimeout = 5000
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
            val thumb = page.optJSONObject("thumbnail")
            val original = page.optJSONObject("original")
            val imgUrl = thumb?.optString("source") ?: original?.optString("source")

            if (!imgUrl.isNullOrEmpty() && (imgUrl.endsWith(".jpg", true) || imgUrl.endsWith(".png", true) || imgUrl.contains(".jpg?") || imgUrl.contains(".png?"))) {
                try {
                    val imgConn = URL(imgUrl).openConnection() as HttpURLConnection
                    imgConn.connectTimeout = 6000
                    imgConn.readTimeout = 8000
                    imgConn.setRequestProperty("User-Agent", "IshaAI/1.0 (contact@isha.ai)")
                    imgConn.connect()

                    if (imgConn.responseCode == HttpURLConnection.HTTP_OK) {
                        val bytes = imgConn.inputStream.use { it.readBytes() }
                        imgConn.disconnect()
                        val bmp = BitmapFactory.decodeByteArray(bytes, 0, bytes.size)
                        if (bmp != null) return bmp
                    }
                    imgConn.disconnect()
                } catch (e: Exception) {
                    Log.w(TAG, "Error downloading Wikipedia image: ${e.message}")
                }
            }
        }
        return null
    }

    private fun fetchPollinationsVisual(prompt: String): Bitmap? {
        val enc = URLEncoder.encode(prompt, "UTF-8")
        val urlString = "https://image.pollinations.ai/prompt/$enc"

        val conn = URL(urlString).openConnection() as HttpURLConnection
        conn.connectTimeout = 6000
        conn.readTimeout = 9000
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
}
