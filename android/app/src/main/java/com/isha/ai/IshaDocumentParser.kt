package com.isha.ai

import android.content.Context
import android.net.Uri
import java.io.BufferedReader
import java.io.InputStream
import java.io.InputStreamReader
import java.util.zip.ZipInputStream
import java.util.regex.Pattern

object IshaDocumentParser {

    interface ParserListener {
        fun onPageParsed(pageNum: Int, text: String)
    }

    fun parseUriStreamed(context: Context, uri: Uri, mimeType: String?, listener: ParserListener) {
        val contentResolver = context.contentResolver
        val inputStream = contentResolver.openInputStream(uri) ?: return
        
        try {
            when {
                mimeType == "text/plain" || mimeType == "text/markdown" || uri.path?.endsWith(".txt") == true || uri.path?.endsWith(".md") == true -> {
                    parseTxtStreamed(inputStream, listener)
                }
                mimeType == "application/vnd.openxmlformats-officedocument.wordprocessingml.document" || uri.path?.endsWith(".docx") == true -> {
                    parseDocxStreamed(inputStream, listener)
                }
                mimeType == "application/pdf" || uri.path?.endsWith(".pdf") == true -> {
                    parsePdfStreamed(inputStream, listener)
                }
                else -> {
                    // Fallback to text
                    parseTxtStreamed(inputStream, listener)
                }
            }
        } finally {
            inputStream.close()
        }
    }

    private fun parseTxtStreamed(inputStream: InputStream, listener: ParserListener) {
        val reader = BufferedReader(InputStreamReader(inputStream, "UTF-8"))
        var line: String?
        var pageNum = 1
        var charCount = 0
        val pageBuffer = StringBuilder()

        // Page size boundary proxy for plain text: every ~1500 chars is treated as a "page"
        while (reader.readLine().also { line = it } != null) {
            pageBuffer.append(line).append("\n")
            charCount += line!!.length + 1
            if (charCount >= 1500) {
                listener.onPageParsed(pageNum++, pageBuffer.toString())
                pageBuffer.setLength(0)
                charCount = 0
            }
        }
        if (pageBuffer.isNotEmpty()) {
            listener.onPageParsed(pageNum, pageBuffer.toString())
        }
    }

    private fun parseDocxStreamed(inputStream: InputStream, listener: ParserListener) {
        val zipInputStream = ZipInputStream(inputStream)
        var entry = zipInputStream.nextEntry
        val xmlContent = StringBuilder()

        while (entry != null) {
            if (entry.name == "word/document.xml") {
                val reader = BufferedReader(InputStreamReader(zipInputStream, "UTF-8"))
                var line: String?
                while (reader.readLine().also { line = it } != null) {
                    xmlContent.append(line)
                }
                break
            }
            entry = zipInputStream.nextEntry
        }
        zipInputStream.close()

        // Parse paragraphs using regex from word/document.xml
        val docXml = xmlContent.toString()
        val matcher = Pattern.compile("<w:t.*?>(.*?)</w:t>").matcher(docXml)
        val textBuffer = StringBuilder()
        var pageNum = 1
        var count = 0

        while (matcher.find()) {
            val text = matcher.group(1) ?: continue
            textBuffer.append(text).append(" ")
            count += text.length
            if (count >= 1500) {
                listener.onPageParsed(pageNum++, textBuffer.toString().trim())
                textBuffer.setLength(0)
                count = 0
            }
        }
        if (textBuffer.isNotEmpty()) {
            listener.onPageParsed(pageNum, textBuffer.toString().trim())
        }
    }

    private fun parsePdfStreamed(inputStream: InputStream, listener: ParserListener) {
        val reader = BufferedReader(InputStreamReader(inputStream, "ISO-8859-1"))
        val pdfContent = StringBuilder()
        var line: String?
        
        // Read in blocks to scan for text streams
        while (reader.readLine().also { line = it } != null) {
            pdfContent.append(line).append("\n")
        }
        
        val pdfText = pdfContent.toString()
        // Extract text streams between BT and ET
        val matcher = Pattern.compile("(?s)BT(.*?)ET").matcher(pdfText)
        var pageNum = 1
        val pageBuffer = StringBuilder()
        var charCount = 0

        while (matcher.find()) {
            val stream = matcher.group(1) ?: continue
            // Extract content inside Tj or TJ operators: (text)Tj or [(text1)...(text2)]TJ
            val textMatcher = Pattern.compile("\\((.*?)\\)\\s*T[jJ]").matcher(stream)
            while (textMatcher.find()) {
                val matchText = textMatcher.group(1) ?: continue
                // Filter out PDF control chars
                val cleanText = matchText.replace("\\\\", "").replace("\\(", "(").replace("\\)", ")")
                pageBuffer.append(cleanText).append(" ")
                charCount += cleanText.length
                
                // Stream Page when buffer size reached
                if (charCount >= 1200) {
                    listener.onPageParsed(pageNum++, pageBuffer.toString().trim())
                    pageBuffer.setLength(0)
                    charCount = 0
                }
            }
        }
        
        if (pageBuffer.isNotEmpty()) {
            listener.onPageParsed(pageNum, pageBuffer.toString().trim())
        }
        
        // Fallback for scanned/binary PDFs
        if (pageNum == 1 && pageBuffer.isEmpty()) {
            listener.onPageParsed(1, "[Scanned or Non-text PDF document content]")
        }
    }
}
