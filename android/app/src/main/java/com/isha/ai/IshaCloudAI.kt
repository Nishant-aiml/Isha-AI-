package com.isha.ai

import android.content.Context
import android.util.Log
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.withContext
import org.json.JSONArray
import org.json.JSONObject
import java.io.BufferedReader
import java.io.InputStreamReader
import java.io.OutputStreamWriter
import java.net.HttpURLConnection
import java.net.URL

object IshaCloudAI {
    private const val TAG = "IshaCloudAI"
    private const val PREFS_NAME = "isha_ai_cloud_prefs"
    private const val KEY_PROVIDER = "cloud_provider"
    private const val KEY_API_KEY = "cloud_api_key"
    private const val KEY_SELECTED_MODEL = "cloud_selected_model"
    private const val KEY_BACKEND_URL = "cloud_backend_url"

    const val PROVIDER_HYBRID = "hybrid"   // Auto: Cloud if key available, else Web RAG + Cognitive
    const val PROVIDER_GROQ = "groq"       // Free Groq Cloud (Llama 3.3 70B / 3.2 3B)
    const val PROVIDER_GEMINI = "gemini"   // Free Google AI Studio (Gemini 1.5/2.5 Flash)
    const val PROVIDER_BACKEND = "backend" // Custom Isha Server (/api/chat)
    const val PROVIDER_LOCAL = "local"     // Force local on-device only

    // Available Cloud Models
    const val MODEL_GROQ_LLAMA_70B = "llama-3.3-70b-versatile"
    const val MODEL_GROQ_LLAMA_3B = "llama-3.2-3b-preview"
    const val MODEL_GROQ_MISTRAL_7B = "mixtral-8x7b-32768"
    const val MODEL_GEMINI_FLASH = "gemini-1.5-flash"

    fun getProvider(context: Context): String {
        val prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
        return prefs.getString(KEY_PROVIDER, PROVIDER_HYBRID) ?: PROVIDER_HYBRID
    }

    fun getApiKey(context: Context): String {
        val prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
        return prefs.getString(KEY_API_KEY, "") ?: ""
    }

    fun getSelectedModel(context: Context): String {
        val prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
        val prov = getProvider(context)
        val defaultModel = if (prov == PROVIDER_GEMINI) MODEL_GEMINI_FLASH else MODEL_GROQ_LLAMA_70B
        return prefs.getString(KEY_SELECTED_MODEL, defaultModel) ?: defaultModel
    }

    fun getBackendUrl(context: Context): String {
        val prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
        return prefs.getString(KEY_BACKEND_URL, "http://10.0.2.2:8000") ?: "http://10.0.2.2:8000"
    }

    fun saveConfig(context: Context, provider: String, apiKey: String, selectedModel: String = "", backendUrl: String = "") {
        val prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
        prefs.edit().apply {
            putString(KEY_PROVIDER, provider)
            putString(KEY_API_KEY, apiKey.trim())
            if (selectedModel.isNotEmpty()) putString(KEY_SELECTED_MODEL, selectedModel.trim())
            if (backendUrl.isNotEmpty()) putString(KEY_BACKEND_URL, backendUrl.trim())
            apply()
        }
    }

    fun clearConfig(context: Context) {
        val prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
        prefs.edit().clear().apply()
    }

    /**
     * Determines whether cloud offload can be used for the current request.
     */
    fun isCloudAvailable(context: Context): Boolean {
        val provider = getProvider(context)
        if (provider == PROVIDER_LOCAL) return false
        val key = getApiKey(context)
        return key.isNotEmpty() || provider == PROVIDER_BACKEND
    }

    /**
     * Streams an answer from the selected free cloud provider with grounded context and optional image Base64.
     */
    suspend fun streamCloudInference(
        context: Context,
        query: String,
        groundedContext: String,
        imageBase64: String? = null,
        onToken: (String) -> Unit
    ): Boolean = withContext(Dispatchers.IO) {
        val provider = getProvider(context)
        val apiKey = getApiKey(context)
        val backendUrl = getBackendUrl(context)
        val modelName = getSelectedModel(context)

        // Determine actual active cloud target
        val activeTarget = when {
            provider == PROVIDER_GROQ -> PROVIDER_GROQ
            provider == PROVIDER_GEMINI -> PROVIDER_GEMINI
            provider == PROVIDER_BACKEND -> PROVIDER_BACKEND
            provider == PROVIDER_HYBRID && apiKey.startsWith("gsk_") -> PROVIDER_GROQ
            provider == PROVIDER_HYBRID && apiKey.startsWith("AIza") -> PROVIDER_GEMINI
            provider == PROVIDER_HYBRID && apiKey.isNotEmpty() -> PROVIDER_GEMINI // Default to Gemini
            else -> return@withContext false
        }

        val promptWithContext = if (groundedContext.trim().isNotEmpty()) {
            "Verified Reference Context:\n$groundedContext\n\nUser Question: $query\n\nPlease answer the user's question accurately, concisely, and helpfully using the verified context above when applicable."
        } else {
            query
        }

        try {
            when (activeTarget) {
                PROVIDER_GROQ -> streamGroq(apiKey, promptWithContext, modelName, onToken)
                PROVIDER_GEMINI -> streamGemini(apiKey, promptWithContext, imageBase64, onToken)
                PROVIDER_BACKEND -> streamBackend(backendUrl, query, groundedContext, onToken)
                else -> false
            }
        } catch (e: Exception) {
            Log.e(TAG, "Cloud inference failed: ${e.message}", e)
            false
        }
    }

    /**
     * Groq Cloud Inference (Free Tier, Llama 3.3 70B / 3.2 3B, Streaming SSE)
     */
    private suspend fun streamGroq(
        apiKey: String,
        prompt: String,
        modelName: String = MODEL_GROQ_LLAMA_70B,
        onToken: (String) -> Unit
    ): Boolean {
        if (apiKey.isEmpty()) return false
        val url = URL("https://api.groq.com/openai/v1/chat/completions")
        val conn = (url.openConnection() as HttpURLConnection).apply {
            requestMethod = "POST"
            connectTimeout = 12000
            readTimeout = 25000
            doOutput = true
            setRequestProperty("Authorization", "Bearer $apiKey")
            setRequestProperty("Content-Type", "application/json")
        }

        val body = JSONObject().apply {
            put("model", if (modelName.isNotEmpty()) modelName else MODEL_GROQ_LLAMA_70B)
            put("messages", JSONArray().apply {
                put(JSONObject().apply {
                    put("role", "system")
                    put("content", "You are ISHA AI, an expert, accurate, and grounded AI assistant. You answer questions factually and concisely without hallucination.")
                })
                put(JSONObject().apply {
                    put("role", "user")
                    put("content", prompt)
                })
            })
            put("stream", true)
            put("temperature", 0.3)
            put("max_tokens", 2048)
        }

        OutputStreamWriter(conn.outputStream, Charsets.UTF_8).use { it.write(body.toString()) }

        if (conn.responseCode != HttpURLConnection.HTTP_OK) {
            val err = conn.errorStream?.bufferedReader(Charsets.UTF_8)?.use { it.readText() } ?: ""
            Log.w(TAG, "Groq HTTP ${conn.responseCode}: $err")
            conn.disconnect()
            return false
        }

        val reader = BufferedReader(InputStreamReader(conn.inputStream, Charsets.UTF_8))
        var line: String?
        var tokenCount = 0

        while (reader.readLine().also { line = it } != null) {
            val l = line?.trim() ?: continue
            if (l.startsWith("data: ")) {
                val dataStr = l.substring(6).trim()
                if (dataStr == "[DONE]") break
                try {
                    val obj = JSONObject(dataStr)
                    val choices = obj.optJSONArray("choices")
                    if (choices != null && choices.length() > 0) {
                        val delta = choices.getJSONObject(0).optJSONObject("delta")
                        val content = delta?.optString("content", "") ?: ""
                        if (content.isNotEmpty()) {
                            onToken(content)
                            tokenCount++
                        }
                    }
                } catch (e: Exception) {
                    // Ignore transient chunk parse errors
                }
            }
        }
        conn.disconnect()
        return tokenCount > 0
    }

    /**
     * Google AI Studio Gemini API (Free Tier, Gemini 1.5 Flash, Multimodal Vision QA)
     */
    private suspend fun streamGemini(
        apiKey: String,
        prompt: String,
        imageBase64: String? = null,
        onToken: (String) -> Unit
    ): Boolean {
        if (apiKey.isEmpty()) return false
        val endpoint = "https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent?key=$apiKey"
        val url = URL(endpoint)
        val conn = (url.openConnection() as HttpURLConnection).apply {
            requestMethod = "POST"
            connectTimeout = 15000
            readTimeout = 30000
            doOutput = true
            setRequestProperty("Content-Type", "application/json")
        }

        val partsArray = JSONArray().apply {
            put(JSONObject().apply {
                put("text", prompt)
            })
            if (imageBase64 != null && imageBase64.isNotEmpty()) {
                put(JSONObject().apply {
                    put("inlineData", JSONObject().apply {
                        put("mimeType", "image/jpeg")
                        put("data", imageBase64)
                    })
                })
            }
        }

        val body = JSONObject().apply {
            put("contents", JSONArray().apply {
                put(JSONObject().apply {
                    put("parts", partsArray)
                })
            })
            put("systemInstruction", JSONObject().apply {
                put("parts", JSONArray().apply {
                    put(JSONObject().apply {
                        put("text", "You are ISHA AI, an expert, accurate, and grounded AI assistant. Answer truthfully, concisely, and helpfully. For images, inspect the visual features in detail.")
                    })
                })
            })
            put("generationConfig", JSONObject().apply {
                put("temperature", 0.3)
                put("maxOutputTokens", 2048)
            })
        }

        OutputStreamWriter(conn.outputStream, Charsets.UTF_8).use { it.write(body.toString()) }

        if (conn.responseCode != HttpURLConnection.HTTP_OK) {
            val err = conn.errorStream?.bufferedReader(Charsets.UTF_8)?.use { it.readText() } ?: ""
            Log.w(TAG, "Gemini HTTP ${conn.responseCode}: $err")
            conn.disconnect()
            return false
        }

        val jsonStr = conn.inputStream.bufferedReader(Charsets.UTF_8).use { it.readText() }
        conn.disconnect()

        val json = JSONObject(jsonStr)
        val candidates = json.optJSONArray("candidates")
        if (candidates != null && candidates.length() > 0) {
            val content = candidates.getJSONObject(0).optJSONObject("content")
            val parts = content?.optJSONArray("parts")
            if (parts != null && parts.length() > 0) {
                val text = parts.getJSONObject(0).optString("text", "")
                if (text.isNotEmpty()) {
                    val tokens = text.split(Regex("(?<=\\s)|(?=\\s)"))
                    for (t in tokens) {
                        onToken(t)
                        delay(12)
                    }
                    return true
                }
            }
        }
        return false
    }

    /**
     * Custom Local / Cloud Isha Backend Server (/api/chat)
     */
    private suspend fun streamBackend(
        backendUrl: String,
        query: String,
        groundedContext: String,
        onToken: (String) -> Unit
    ): Boolean {
        val endpoint = "${backendUrl.trimEnd('/')}/api/chat"
        val url = URL(endpoint)
        val conn = (url.openConnection() as HttpURLConnection).apply {
            requestMethod = "POST"
            connectTimeout = 8000
            readTimeout = 20000
            doOutput = true
            setRequestProperty("Content-Type", "application/json")
        }

        val body = JSONObject().apply {
            put("prompt", query)
            put("context", groundedContext)
        }

        OutputStreamWriter(conn.outputStream, Charsets.UTF_8).use { it.write(body.toString()) }

        if (conn.responseCode != HttpURLConnection.HTTP_OK) {
            conn.disconnect()
            return false
        }

        val respStr = conn.inputStream.bufferedReader(Charsets.UTF_8).use { it.readText() }
        conn.disconnect()

        val json = JSONObject(respStr)
        val response = json.optString("response", "")
        if (response.isNotEmpty()) {
            val tokens = response.split(Regex("(?<=\\s)|(?=\\s)"))
            for (t in tokens) {
                onToken(t)
                delay(12)
            }
            return true
        }
        return false
    }
}
