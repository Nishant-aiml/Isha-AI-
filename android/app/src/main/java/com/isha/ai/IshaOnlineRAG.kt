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
import java.net.URLDecoder
import java.net.URLEncoder
import java.util.regex.Pattern

data class WebSource(
    val title: String,
    val snippet: String,
    val url: String,
    val confidence: Double = 0.90
)

object IshaOnlineRAG {
    private const val TAG = "IshaOnlineRAG"
    private const val TIMEOUT_MS = 6000

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

    fun extractCoreQuery(rawQuery: String): String {
        var q = rawQuery.trim()
        val prefixes = listOf(
            "tell me who is ", "who is ", "tell me about ", "tell me ",
            "what is ", "what are ", "can you tell me about ", "can you tell me ",
            "explain to me ", "explain ", "please tell me ", "give details on ",
            "give me information about ", "who was ", "what was ", "show me "
        )
        for (p in prefixes) {
            if (q.startsWith(p, ignoreCase = true)) {
                q = q.substring(p.length).trim()
                break
            }
        }
        q = q.trimEnd('?', '.', '!', ',')
        return if (q.isNotEmpty()) q else rawQuery
    }

    suspend fun searchLiveWeb(query: String, maxResults: Int = 3): List<WebSource> = withContext(Dispatchers.IO) {
        val cleanQuery = scrubPII(query).trim()
        if (cleanQuery.isEmpty()) return@withContext emptyList()

        val coreQuery = extractCoreQuery(cleanQuery)
        val results = mutableListOf<WebSource>()

        // 1. DuckDuckGo Real Web Search (POST to html.duckduckgo.com/html/)
        try {
            val ddgResults = queryDuckDuckGoHtml(coreQuery, maxResults)
            results.addAll(ddgResults)
        } catch (e: Exception) {
            Log.w(TAG, "DuckDuckGo HTML search error: ${e.message}")
        }

        // 2. Wikipedia Search API (fallback or supplement)
        if (results.size < maxResults) {
            try {
                val wikiResults = queryWikipedia(coreQuery, maxResults - results.size)
                results.addAll(wikiResults)

                // Alternate spelling attempt (e.g. dipak -> deepak)
                if (results.isEmpty() && coreQuery.contains("dipak", ignoreCase = true)) {
                    val altQuery = coreQuery.replace("dipak", "deepak", ignoreCase = true)
                    results.addAll(queryWikipedia(altQuery, maxResults))
                }
            } catch (e: Exception) {
                Log.w(TAG, "Wikipedia search error: ${e.message}")
            }
        }

        Log.i(TAG, "Live web search for '$coreQuery' retrieved ${results.size} sources.")
        results.take(maxResults)
    }

    private fun queryDuckDuckGoHtml(query: String, max: Int): List<WebSource> {
        val list = mutableListOf<WebSource>()
        val url = URL("https://html.duckduckgo.com/html/")
        val postData = "q=" + URLEncoder.encode(query, "UTF-8")
        val postDataBytes = postData.toByteArray(Charsets.UTF_8)

        val conn = (url.openConnection() as HttpURLConnection).apply {
            requestMethod = "POST"
            connectTimeout = TIMEOUT_MS
            readTimeout = TIMEOUT_MS
            doOutput = true
            setRequestProperty("Content-Type", "application/x-www-form-urlencoded")
            setRequestProperty("Content-Length", postDataBytes.size.toString())
            setRequestProperty("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36")
        }

        conn.outputStream.use { it.write(postDataBytes) }

        if (conn.responseCode == HttpURLConnection.HTTP_OK || conn.responseCode == 202) {
            val html = conn.inputStream.bufferedReader(Charsets.UTF_8).use { it.readText() }

            val snippetPattern = Pattern.compile("<a class=\"result__snippet[^\"]*\"[^>]*>(.*?)</a>", Pattern.DOTALL)
            val titlePattern = Pattern.compile("<a class=\"result__title\"[^>]*href=\"([^\"]+)\"[^>]*>(.*?)</a>", Pattern.DOTALL)

            val snippetMatcher = snippetPattern.matcher(html)
            val titleMatcher = titlePattern.matcher(html)

            val snippets = mutableListOf<String>()
            val titles = mutableListOf<Pair<String, String>>()

            while (snippetMatcher.find()) {
                val s = snippetMatcher.group(1) ?: ""
                val clean = s.replace(Regex("<[^>]+>"), "")
                    .replace("&quot;", "\"")
                    .replace("&#x27;", "'")
                    .replace("&#039;", "'")
                    .replace("&amp;", "&")
                    .replace("&lt;", "<")
                    .replace("&gt;", ">")
                    .trim()
                if (clean.isNotEmpty()) snippets.add(clean)
            }

            while (titleMatcher.find()) {
                val href = titleMatcher.group(1) ?: ""
                val rawTitle = titleMatcher.group(2) ?: ""
                val cleanTitle = rawTitle.replace(Regex("<[^>]+>"), "")
                    .replace("&quot;", "\"")
                    .replace("&#x27;", "'")
                    .replace("&#039;", "'")
                    .replace("&amp;", "&")
                    .trim()

                var finalHref = href
                if (href.contains("uddg=")) {
                    val m = Pattern.compile("uddg=([^&]+)").matcher(href)
                    if (m.find()) {
                        try {
                            finalHref = URLDecoder.decode(m.group(1), "UTF-8")
                        } catch (e: Exception) {}
                    }
                }
                titles.add(Pair(cleanTitle, finalHref))
            }

            for (i in 0 until minOf(snippets.size, max)) {
                val title = if (i < titles.size && titles[i].first.isNotEmpty()) titles[i].first else "Web Information"
                val href = if (i < titles.size && titles[i].second.isNotEmpty()) titles[i].second else "https://duckduckgo.com"
                list.add(WebSource(title, snippets[i], href, 0.92))
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
            setRequestProperty("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36")
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
                val snippetClean = snippetRaw.replace(Regex("<[^>]+>"), "")
                    .replace("&quot;", "\"")
                    .replace("&#039;", "'")
                    .replace("&#x27;", "'")
                    .replace("&amp;", "&")
                    .trim()
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
