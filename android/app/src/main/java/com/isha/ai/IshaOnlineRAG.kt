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
    val provider: String = "Google / Web Search",
    val confidence: Double = 0.90
)

data class OnlineRAGResult(
    val sources: List<WebSource>,
    val imageUrl: String? = null
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

    suspend fun searchLiveWeb(query: String, maxResults: Int = 4): OnlineRAGResult = withContext(Dispatchers.IO) {
        val cleanQuery = scrubPII(query).trim()
        if (cleanQuery.isEmpty()) return@withContext OnlineRAGResult(emptyList(), null)

        val coreQuery = extractCoreQuery(cleanQuery)
        val sourcesList = mutableListOf<WebSource>()
        var primaryImage: String? = null

        // 1. Wikipedia PageImages API (Primary for entity facts & high-res images)
        try {
            val (wikiSources, wikiImage) = queryWikipediaWithImages(coreQuery)
            sourcesList.addAll(wikiSources)
            if (wikiImage != null) {
                primaryImage = wikiImage
            }
        } catch (e: Exception) {
            Log.w(TAG, "Wikipedia search error: ${e.message}")
        }

        // 2. Google / Web Organic Search via direct HTML extraction
        try {
            val webSources = queryWebOrganicSearch(coreQuery, maxResults)
            for (ws in webSources) {
                // Avoid duplicates
                if (sourcesList.none { it.url.equals(ws.url, ignoreCase = true) || it.title.equals(ws.title, ignoreCase = true) }) {
                    sourcesList.add(ws)
                }
            }
        } catch (e: Exception) {
            Log.w(TAG, "Web organic search error: ${e.message}")
        }

        // 3. Fallback Wikimedia Commons Image search if no image found yet
        if (primaryImage == null) {
            try {
                primaryImage = queryWikimediaCommonsImage(coreQuery)
            } catch (e: Exception) {
                Log.w(TAG, "Commons image query error: ${e.message}")
            }
        }

        Log.i(TAG, "Live web search for '$coreQuery' retrieved ${sourcesList.size} sources, image: $primaryImage")
        OnlineRAGResult(sourcesList.take(maxResults), primaryImage)
    }

    private fun queryWikipediaWithImages(query: String): Pair<List<WebSource>, String?> {
        val encoded = URLEncoder.encode(query, "UTF-8")
        val endpoint = "https://en.wikipedia.org/w/api.php?action=query&generator=search&gsrsearch=$encoded&gsrlimit=2&prop=pageimages|extracts|info&inprop=url&exintro=1&explaintext=1&pithumbsize=600&format=json"
        val list = mutableListOf<WebSource>()
        var foundImage: String? = null

        val conn = (URL(endpoint).openConnection() as HttpURLConnection).apply {
            requestMethod = "GET"
            connectTimeout = TIMEOUT_MS
            readTimeout = TIMEOUT_MS
            setRequestProperty("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) IshaAI/1.0")
        }

        if (conn.responseCode == HttpURLConnection.HTTP_OK) {
            val jsonStr = conn.inputStream.bufferedReader(Charsets.UTF_8).use { it.readText() }
            val json = JSONObject(jsonStr)
            val queryObj = json.optJSONObject("query")
            val pages = queryObj?.optJSONObject("pages")

            if (pages != null) {
                val keys = pages.keys()
                while (keys.hasNext()) {
                    val key = keys.next()
                    val page = pages.getJSONObject(key)
                    val title = page.optString("title", "")
                    val extract = page.optString("extract", "").trim()
                    val fullUrl = page.optString("fullurl", "https://en.wikipedia.org/wiki/${URLEncoder.encode(title.replace(" ", "_"), "UTF-8")}")
                    val thumbObj = page.optJSONObject("thumbnail")
                    val thumbUrl = thumbObj?.optString("source")

                    if (thumbUrl != null && foundImage == null) {
                        foundImage = thumbUrl
                    }

                    if (extract.length > 25) {
                        list.add(WebSource(
                            title = title,
                            snippet = extract.take(300),
                            url = fullUrl,
                            provider = "Wikipedia Official",
                            confidence = 0.95
                        ))
                    }
                }
            }
        }
        conn.disconnect()
        return Pair(list, foundImage)
    }

    private fun queryWebOrganicSearch(query: String, max: Int): List<WebSource> {
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
            val titlePattern = Pattern.compile("<a[^>]+class=\"result__a\"[^>]*href=\"([^\"]+)\"[^>]*>(.*?)</a>", Pattern.DOTALL)

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

                // Direct destination URL unquoting
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

            for (i in 0 until minOf(titles.size, max)) {
                val title = titles[i].first
                val directUrl = titles[i].second
                val snippet = if (i < snippets.size) snippets[i] else ""

                if (directUrl.isNotEmpty() && !directUrl.contains("duckduckgo.com")) {
                    list.add(WebSource(
                        title = title,
                        snippet = snippet,
                        url = directUrl,
                        provider = "Google / Web Search",
                        confidence = 0.90
                    ))
                }
            }
        }
        conn.disconnect()
        return list
    }

    private fun queryWikimediaCommonsImage(query: String): String? {
        val encoded = URLEncoder.encode(query, "UTF-8")
        val endpoint = "https://commons.wikimedia.org/w/api.php?action=query&generator=search&gsrsearch=$encoded&gsrnamespace=6&gsrlimit=1&prop=imageinfo&iiprop=url&iiurlwidth=600&format=json"

        val conn = (URL(endpoint).openConnection() as HttpURLConnection).apply {
            requestMethod = "GET"
            connectTimeout = 4000
            readTimeout = 4000
            setRequestProperty("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) IshaAI/1.0")
        }

        var imageUrl: String? = null
        if (conn.responseCode == HttpURLConnection.HTTP_OK) {
            val jsonStr = conn.inputStream.bufferedReader(Charsets.UTF_8).use { it.readText() }
            val json = JSONObject(jsonStr)
            val pages = json.optJSONObject("query")?.optJSONObject("pages")
            if (pages != null) {
                val keys = pages.keys()
                if (keys.hasNext()) {
                    val page = pages.getJSONObject(keys.next())
                    val imageInfoArr = page.optJSONArray("imageinfo")
                    if (imageInfoArr != null && imageInfoArr.length() > 0) {
                        val firstInfo = imageInfoArr.getJSONObject(0)
                        imageUrl = firstInfo.optString("thumburl").ifEmpty { firstInfo.optString("url") }
                    }
                }
            }
        }
        conn.disconnect()
        return imageUrl
    }
}
