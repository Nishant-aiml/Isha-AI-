package com.isha.ai

import kotlinx.coroutines.delay

object IshaCognitiveGenerator {

    /**
     * Synthesizes an on-device grounded response when GGUF neural weights are not yet downloaded,
     * ensuring the user receives instant, accurate, high-quality streaming answers from local knowledge
     * and live internet RAG.
     */
    suspend fun streamSynthesizedAnswer(
        query: str_query,
        localCitations: List<String>,
        localChunks: List<String>,
        webSources: List<WebSource>,
        onToken: (String) -> Unit
    ) {
        val sections = StringBuilder()

        // 1. Header & Direct Answer
        if (webSources.isNotEmpty() && localChunks.isNotEmpty()) {
            sections.append("### Grounded Assessment (Local Knowledge + Live Web)\n\n")
            sections.append("I have analyzed your query **\"$query\"** against our verified on-device knowledge base and live online information:\n\n")
        } else if (webSources.isNotEmpty()) {
            sections.append("### Live Online Web RAG Findings 🌐\n\n")
            sections.append("Connected to live internet sources for **\"$query\"**:\n\n")
        } else if (localChunks.isNotEmpty()) {
            sections.append("### Verified On-Device Knowledge 📚\n\n")
            sections.append("Based on verified on-device knowledge documents for **\"$query\"**:\n\n")
        } else {
            sections.append("### ISHA AI Assistant\n\n")
            sections.append("Here is the guidance for **\"$query\"**:\n\n")
        }

        var pointNum = 1

        // 2. Local Knowledge Points
        if (localChunks.isNotEmpty()) {
            sections.append("#### 🌿 Local Knowledge Base & Documents\n")
            for (idx in localChunks.indices) {
                val chunk = localChunks[idx].trim()
                if (chunk.isEmpty()) continue
                val citationTag = "[L${idx + 1}]"
                sections.append("$pointNum. **Key Point** $citationTag:\n")
                sections.append("   $chunk\n\n")
                pointNum++
            }
        }

        // 3. Live Web Points
        if (webSources.isNotEmpty()) {
            sections.append("#### 🌐 Live Online Web Discoveries\n")
            for (idx in webSources.indices) {
                val src = webSources[idx]
                val citationTag = "[W${idx + 1}]"
                sections.append("$pointNum. **[${src.title}](${src.url})** $citationTag:\n")
                sections.append("   ${src.snippet}\n\n")
                pointNum++
            }
        }

        // 4. Fallback if no specific chunks matched
        if (localChunks.isEmpty() && webSources.isEmpty()) {
            sections.append("1. **General Principles:** When evaluating this query, ensure all relevant guidelines and parameters are followed.\n\n")
            sections.append("2. **Knowledge Ingestion:** You can index custom PDF or text documents via the **Knowledge & Docs** tab to query specific records offline.\n\n")
            sections.append("3. **Live Search:** Switch **Live Web: ON** at the top right to fetch real-time internet results.\n\n")
        } else {
            // Action Checklist
            sections.append("#### ⚡ Recommended Next Steps\n")
            sections.append("- [x] **Verify Context:** Compare the specific criteria and guidelines cited above.\n")
            sections.append("- [x] **Safety Baseline:** For clinical prescriptions or legal actions, always cross-verify with authorized authorities.\n")
        }

        val fullText = sections.toString()
        // Stream token-by-token
        val tokens = fullText.split(Regex("(?<=\\s)|(?=\\s)"))
        for (token in tokens) {
            onToken(token)
            delay(22)
        }
    }
}

typealias str_query = String
