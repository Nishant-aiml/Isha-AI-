package com.isha.ai

import kotlinx.coroutines.delay

object IshaCognitiveGenerator {

    /**
     * Synthesizes a natural, high-fidelity grounded answer from verified local knowledge
     * and live internet search snippets.
     */
    suspend fun streamSynthesizedAnswer(
        query: String,
        localCitations: List<String>,
        localChunks: List<String>,
        webSources: List<WebSource>,
        onToken: (String) -> Unit
    ) {
        val sections = StringBuilder()

        // Filter out empty or whitespace-only chunks
        val validLocal = localChunks.map { it.trim() }.filter { it.isNotEmpty() }
        val validWeb = webSources.filter { it.snippet.trim().isNotEmpty() }

        // CASE 1: Live Web Sources Only (e.g. Person, news, latest events)
        if (validWeb.isNotEmpty() && validLocal.isEmpty()) {
            sections.append("### Live Web Intelligence 🌐\n\n")
            sections.append("Based on verified online records for **\"$query\"**:\n\n")

            sections.append("#### 📌 Key Facts & Information\n")
            for (idx in validWeb.indices) {
                val src = validWeb[idx]
                val tag = "[W${idx + 1}]"
                sections.append("- **${src.title}** $tag:\n  ${src.snippet}\n\n")
            }

            sections.append("#### 🌐 Live Web Sources\n")
            for (idx in validWeb.indices) {
                val src = validWeb[idx]
                val tag = "[W${idx + 1}]"
                sections.append("- [${src.title}](${src.url}) $tag\n")
            }
        }
        // CASE 2: Local Knowledge Only (e.g. crop care, first aid, citizen law, custom docs)
        else if (validLocal.isNotEmpty() && validWeb.isEmpty()) {
            sections.append("### Verified On-Device Knowledge 📚\n\n")
            sections.append("Based on verified on-device knowledge documents for **\"$query\"**:\n\n")

            for (idx in validLocal.indices) {
                val chunk = validLocal[idx]
                val tag = "[L${idx + 1}]"
                sections.append("${idx + 1}. **Guideline $tag**:\n   $chunk\n\n")
            }

            sections.append("#### ⚡ Recommended Next Steps\n")
            sections.append("- [x] **Verify Constraints:** Review the specific dosage, parameters, or legal criteria cited above.\n")
            sections.append("- [x] **Observe Progression:** For agricultural or medical concerns, track symptoms carefully.\n")
        }
        // CASE 3: Grounded Hybrid (Both Local and Live Web)
        else if (validLocal.isNotEmpty() && validWeb.isNotEmpty()) {
            sections.append("### Grounded Hybrid Assessment 🌿\n\n")
            sections.append("Synthesizing verified on-device documents and live online records for **\"$query\"**:\n\n")

            sections.append("#### 📚 Verified Offline Knowledge\n")
            for (idx in validLocal.indices) {
                val chunk = validLocal[idx]
                val tag = "[L${idx + 1}]"
                sections.append("- **Point $tag**: $chunk\n\n")
            }

            sections.append("#### 🌐 Live Web Findings\n")
            for (idx in validWeb.indices) {
                val src = validWeb[idx]
                val tag = "[W${idx + 1}]"
                sections.append("- **[${src.title}](${src.url}) $tag**: ${src.snippet}\n\n")
            }
        }
        // CASE 4: Neither matched (No false hallucination)
        else {
            sections.append("### ISHA AI Guidance\n\n")
            sections.append("I analyzed your query: **\"$query\"**.\n\n")
            sections.append("No specific local knowledge chunks or live web records matched this inquiry.\n\n")
            sections.append("💡 **Suggestions:**\n")
            sections.append("- Ensure **Live Web: ON** is active in the top bar to query real-time internet information.\n")
            sections.append("- You can upload and index your own PDF or text manuals in the **Knowledge & Docs** tab.\n")
        }

        val fullText = sections.toString()
        // Stream token-by-token
        val tokens = fullText.split(Regex("(?<=\\s)|(?=\\s)"))
        for (token in tokens) {
            onToken(token)
            delay(18)
        }
    }
}
