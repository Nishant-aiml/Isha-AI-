package com.isha.ai

import android.util.Log
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import org.json.JSONArray
import org.json.JSONObject
import java.io.BufferedReader
import java.io.InputStreamReader
import java.net.HttpURLConnection
import java.net.URL
import java.net.URLEncoder
import java.util.regex.Pattern

data class WebSource(
    val title: String,
    val snippet: String,
    val url: String,
    val confidence: Double = 0.88
)

object IshaOnlineRAG {
    private const val TAG = "IshaOnlineRAG"
    private const val TIMEOUT_MS = 4000

    // PII scrubber patterns to protect user privacy
    private val PII_PATTERNS = listOf(
        Pattern.compile("[a-zA-Z0-9_.+-]+@[a-zA-Z0-9-]+\\.[a-zA-Z0-9-.]+"), // Email
        Pattern.compile("\\b(?:\\+91|91)?[6-9]\\d{9}\\b"),                  // Indian Phone
        Pattern.compile("\\b\\d{4}\\s?\\d{4}\\s?\\d{4}\\b"),                // Aadhaar
        Pattern.compile("\\b[A-Z]{5}[0-9]{4}[A-Z]\\b")                     // PAN
    )

    fun scrubPII(text: String): String {
        var clean = text
        for (pattern in PII_PATTERNS) {
            clean = pattern.matcher(clean).replaceAll("[REDACTED]")
        }
        return clean
    }

    suspend fun searchLiveWeb(query: String, maxResults: Int = 3): List<WebSource> = withContext(Dispatchers.IO) {
        val cleanQuery = scrubPII(query).trim()
        if (cleanQuery.isEmpty()) return@withContext emptyList()

        val results = mutableListOf<WebSource>()

        // 1. DuckDuckGo Instant Answer API
        try {
            val ddgResults = queryDuckDuckGo(cleanQuery)
            results.addAll(ddgResults)
        } catch (e: Exception) {
            Log.w(TAG, "DuckDuckGo search error: ${e.message}")
        }

        // 2. Wikipedia Search API (fallback & supplement for facts/news/definitions)
        if (results.size < maxResults) {
            try {
                val wikiResults = queryWikipedia(cleanQuery, maxResults - results.size)
                results.addAll(wikiResults)
            } catch (e: Exception) {
                Log.w(TAG, "Wikipedia search error: ${e.message}")
            }
        }

        Log.i(TAG, "Live web search for '$cleanQuery' retrieved ${results.size} sources.")
        results.take(maxResults)
    }

    private fun queryDuckDuckGo(query: String): List<WebSource> {
        val encoded = URLEncoder.encode(query, "UTF-8")
        val endpoint = "https://api.duckduckgo.com/?q=$encoded&format=json&no_html=1&skip_disambig=1"
        val list = mutableListOf<WebSource>()

        val conn = (URL(endpoint).openConnection() as HttpURLConnection).apply {
            requestMethod = "GET"
            connectTimeout = TIMEOUT_MS
            readTimeout = TIMEOUT_MS
            setRequestProperty("User-Agent", "ISHA-AI/1.0 (Android Mobile Runtime)")
        }

        if (conn.responseCode == HttpURLConnection.HTTP_OK) {
            val reader = BufferedReader(InputStreamReader(conn.inputStream, "UTF-8"))
            val responseStr = reader.use { it.readText() }
            val json = JSONObject(responseStr)

            val abstractText = json.optString("AbstractText", "").trim()
            val abstractSource = json.optString("AbstractSource", "DuckDuckGo")
            val abstractUrl = json.optString("AbstractURL", "")

            if (abstractText.isNotEmpty()) {
                list.add(WebSource(abstractSource, abstractText, abstractUrl, 0.95))
            }

            // Related topics
            val relatedTopics = json.optJSONArray("RelatedTopics") ?: JSONArray()
            for (i in 0 until minOf(relatedTopics.length(), 2)) {
                val item = relatedTopics.optJSONObject(i) ?: continue
                val text = item.optString("Text", "").trim()
                val url = item.optString("FirstURL", "")
                if (text.isNotEmpty() && text != abstractText) {
                    val title = text.take(60).substringBefore(" - ")
                    list.add(WebSource(title, text, url, 0.85))
                }
            }
        }
        conn.disconnect()
        return list
    }

    private fun queryWikipedia(query: String, max: Int): List<WebSource> {
        val encoded = URLEncoder.encode(query, "UTF-8")
        val endpoint = "https://en.wikipedia.org/w/api.php?action=query&list=search&srsearch=$encoded&format=json&utf8=1"
        val list = mutableListOf<WebSource>()

        val conn = (URL(endpoint).openConnection() as HttpURLConnection).apply {
            requestMethod = "GET"
            connectTimeout = TIMEOUT_MS
            readTimeout = TIMEOUT_MS
            setRequestProperty("User-Agent", "ISHA-AI/1.0 (Android Mobile Runtime)")
        }

        if (conn.responseCode == HttpURLConnection.HTTP_OK) {
            val reader = BufferedReader(InputStreamReader(conn.inputStream, "UTF-8"))
            val responseStr = reader.use { it.readText() }
            val json = JSONObject(responseStr)
            val queryObj = json.optJSONObject("query")
            val searchArr = queryObj?.optJSONArray("search") ?: JSONArray()

            for (i in 0 until minOf(searchArr.length(), max)) {
                val item = searchArr.getJSONObject(i)
                val title = item.optString("title", "Wikipedia")
                val snippetRaw = item.optString("snippet", "")
                // Strip HTML tags from snippet
                val snippetClean = snippetRaw.replace(Regex("<[^>]*>"), "").replace("&quot;", "\"").replace("&#039;", "'").trim()
                val url = "https://en.wikipedia.org/wiki/${URLEncoder.encode(title.replace(" ", "_"), "UTF-8")}"

                if (snippetClean.isNotEmpty()) {
                    list.add(WebSource(title, snippetClean, url, 0.90))
                }
            }
        }
        conn.disconnect()
        return list
    }
}
