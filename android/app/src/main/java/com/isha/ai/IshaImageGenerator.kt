package com.isha.ai

import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.util.Log
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.net.HttpURLConnection
import java.net.URL
import java.net.URLEncoder

object IshaImageGenerator {
    private const val TAG = "IshaImageGenerator"

    /**
     * Checks whether the user's prompt is requesting AI image generation.
     */
    fun isImageGenerationIntent(prompt: String): Boolean {
        val p = prompt.lowercase().trim()
        val triggers = listOf(
            "generate image", "create image", "draw an image", "draw a",
            "draw ", "paint an image", "paint a", "paint ", "make an image",
            "make a picture", "picture of ", "generate visual", "ai image"
        )
        return triggers.any { p.startsWith(it) } || p.contains("generate an image of") || p.contains("draw an image of")
    }

    /**
     * Cleans prompt to extract core visual concepts for the image generator.
     */
    fun extractImagePrompt(rawPrompt: String): String {
        var p = rawPrompt.trim()
        val prefixes = listOf(
            "generate image of ", "generate an image of ", "generate image ",
            "create image of ", "create an image of ", "create image ",
            "draw an image of ", "draw a ", "draw ",
            "paint an image of ", "paint a ", "paint ",
            "make an image of ", "make a picture of ", "picture of ",
            "generate visual of ", "ai image of "
        )
        for (pref in prefixes) {
            if (p.startsWith(pref, ignoreCase = true)) {
                p = p.substring(pref.length).trim()
                break
            }
        }
        return if (p.isNotEmpty()) p else rawPrompt
    }

    /**
     * Generates a high-resolution 1024x1024 visual using FLUX.1 [schnell] / Pollinations engine.
     * Completely free, zero API key required, fast serverless edge rendering.
     */
    suspend fun generateImage(promptText: String): Bitmap? = withContext(Dispatchers.IO) {
        val corePrompt = extractImagePrompt(promptText)
        if (corePrompt.isEmpty()) return@withContext null

        val seed = (10000..999999).random()
        val encoded = URLEncoder.encode(corePrompt, "UTF-8")
        // Use FLUX.1 model via high-speed serverless endpoint
        val endpoint = "https://image.pollinations.ai/prompt/$encoded?width=1024&height=1024&nologo=true&seed=$seed&model=flux"

        try {
            val url = URL(endpoint)
            val conn = url.openConnection() as HttpURLConnection
            conn.connectTimeout = 20000
            conn.readTimeout = 30000
            conn.setRequestProperty("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) IshaAI/1.0")
            conn.connect()

            if (conn.responseCode == HttpURLConnection.HTTP_OK) {
                val bitmap = BitmapFactory.decodeStream(conn.inputStream)
                conn.disconnect()
                return@withContext bitmap
            }
            conn.disconnect()
        } catch (e: Exception) {
            Log.e(TAG, "Image generation failed: ${e.message}", e)
        }
        return@withContext null
    }
}
