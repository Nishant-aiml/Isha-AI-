package com.isha.ai

import android.content.Context
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.pdf.PdfRenderer
import android.net.Uri
import android.os.ParcelFileDescriptor
import android.util.Log
import com.google.mlkit.vision.common.InputImage
import com.google.mlkit.vision.text.TextRecognition
import com.google.mlkit.vision.text.latin.TextRecognizerOptions
import kotlinx.coroutines.suspendCancellableCoroutine
import java.io.BufferedReader
import java.io.ByteArrayOutputStream
import java.io.InputStream
import java.io.InputStreamReader
import java.util.zip.Inflater
import java.util.zip.ZipInputStream
import java.util.regex.Pattern
import kotlin.coroutines.resume
import kotlin.coroutines.resumeWithException

object IshaDocumentParser {
    private const val TAG = "IshaDocumentParser"

    interface ParserListener {
        fun onPageParsed(pageNum: Int, text: String)
    }

    suspend fun parseUriStreamed(context: Context, uri: Uri, mimeType: String?, listener: ParserListener) {
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
                    // Try parsing as normal text streams first.
                    // If stream text returns empty, fallback to OCR!
                    var textExtracted = false
                    parsePdfStreamed(inputStream, object : ParserListener {
                        override fun onPageParsed(pageNum: Int, text: String) {
                            if (text.isNotEmpty() && !text.contains("[Scanned or Non-text PDF document content]")) {
                                textExtracted = true
                            }
                            listener.onPageParsed(pageNum, text)
                        }
                    })

                    // Fallback to OCR if no selectable text was found (Identity-H / scanned images)
                    if (!textExtracted) {
                        Log.i(TAG, "No selectable text in PDF streams. Activating on-device OCR fallback.")
                        parsePdfOcrStreamed(context, uri, listener)
                    }
                }
                mimeType?.startsWith("image/") == true || uri.path?.endsWith(".png") == true || uri.path?.endsWith(".jpg") == true || uri.path?.endsWith(".jpeg") == true -> {
                    parseImageOcrStreamed(context, uri, listener)
                }
                else -> {
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
        try {
            val pdfBytes = inputStream.readBytes()
            var pageNum = 1
            val pageBuffer = StringBuilder()
            var charCount = 0

            var index = 0
            while (index < pdfBytes.size) {
                val streamStart = findSequence(pdfBytes, "stream".toByteArray(), index)
                if (streamStart == -1) break

                val streamEnd = findSequence(pdfBytes, "endstream".toByteArray(), streamStart + 6)
                if (streamEnd == -1) break

                val headerStart = maxOf(0, streamStart - 128)
                val headerText = String(pdfBytes, headerStart, streamStart - headerStart, Charsets.US_ASCII)

                val isFlate = headerText.contains("/FlateDecode")

                var dataOffset = streamStart + 6
                if (pdfBytes[dataOffset] == '\r'.toByte()) dataOffset++
                if (pdfBytes[dataOffset] == '\n'.toByte()) dataOffset++

                val dataLength = streamEnd - dataOffset
                if (dataLength > 0) {
                    val streamData = ByteArray(dataLength)
                    System.arraycopy(pdfBytes, dataOffset, streamData, 0, dataLength)

                    val textBytes = if (isFlate) {
                        decompressFlate(streamData)
                    } else {
                        streamData
                    }

                    if (textBytes != null) {
                        val textStr = String(textBytes, Charsets.UTF_8)
                        val cleanText = extractPdfTextFromStream(textStr)
                        if (cleanText.isNotEmpty()) {
                            pageBuffer.append(cleanText).append(" ")
                            charCount += cleanText.length

                            if (charCount >= 1500) {
                                listener.onPageParsed(pageNum++, pageBuffer.toString().trim())
                                pageBuffer.setLength(0)
                                charCount = 0
                            }
                        }
                    }
                }

                index = streamEnd + 9
            }

            if (pageBuffer.isNotEmpty()) {
                listener.onPageParsed(pageNum, pageBuffer.toString().trim())
            }

            if (pageNum == 1 && pageBuffer.isEmpty()) {
                Log.w(TAG, "Zero text parsed from PDF objects.")
                listener.onPageParsed(1, "[Scanned or Non-text PDF document content]")
            }
        } catch (e: Exception) {
            Log.e(TAG, "PDF parsing error: ${e.message}", e)
            listener.onPageParsed(1, "[Error parsing PDF document content]")
        }
    }

    private suspend fun parsePdfOcrStreamed(context: Context, uri: Uri, listener: ParserListener) {
        var pfd: ParcelFileDescriptor? = null
        var renderer: PdfRenderer? = null
        try {
            pfd = context.contentResolver.openFileDescriptor(uri, "r") ?: return
            renderer = PdfRenderer(pfd)
            val pageCount = renderer.pageCount

            for (i in 0 until pageCount) {
                val page = renderer.openPage(i)
                
                // Render page to a high-resolution Bitmap (scale up 2x for optimal OCR accuracy)
                val width = page.width * 2
                val height = page.height * 2
                val bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888)
                page.render(bitmap, null, null, PdfRenderer.Page.RENDER_MODE_FOR_DISPLAY)
                page.close()

                // Run on-device OCR
                val pageText = processOcrBitmap(bitmap)
                bitmap.recycle()

                if (pageText.trim().isNotEmpty()) {
                    listener.onPageParsed(i + 1, pageText.trim())
                }
            }
        } catch (e: Exception) {
            Log.e(TAG, "Failed executing PDF OCR: ${e.message}", e)
        } finally {
            renderer?.close()
            pfd?.close()
        }
    }

    private suspend fun parseImageOcrStreamed(context: Context, uri: Uri, listener: ParserListener) {
        val contentResolver = context.contentResolver
        val inputStream = contentResolver.openInputStream(uri) ?: return
        try {
            val bitmap = BitmapFactory.decodeStream(inputStream)
            if (bitmap != null) {
                val text = processOcrBitmap(bitmap)
                bitmap.recycle()
                listener.onPageParsed(1, text)
            }
        } catch (e: Exception) {
            Log.e(TAG, "Failed executing Image OCR: ${e.message}", e)
        } finally {
            inputStream.close()
        }
    }

    private suspend fun processOcrBitmap(bitmap: Bitmap): String = suspendCancellableCoroutine { cont ->
        val recognizer = TextRecognition.getClient(TextRecognizerOptions.DEFAULT_OPTIONS)
        val image = InputImage.fromBitmap(bitmap, 0)
        
        recognizer.process(image)
            .addOnSuccessListener { visionText ->
                cont.resume(visionText.text)
            }
            .addOnFailureListener { e ->
                cont.resumeWithException(e)
            }
    }

    private fun findSequence(data: ByteArray, seq: ByteArray, startIndex: Int): Int {
        if (seq.isEmpty()) return -1
        for (i in startIndex..data.size - seq.size) {
            var match = true
            for (j in seq.indices) {
                if (data[i + j] != seq[j]) {
                    match = false
                    break
                }
            }
            if (match) return i
        }
        return -1
    }

    private fun decompressFlate(data: ByteArray): ByteArray? {
        val decompressor = Inflater()
        decompressor.setInput(data)
        val bos = ByteArrayOutputStream(data.size * 2)
        val buf = ByteArray(4096)
        try {
            while (!decompressor.finished()) {
                val count = decompressor.inflate(buf)
                if (count == 0 && decompressor.needsInput()) {
                    break
                }
                bos.write(buf, 0, count)
            }
            bos.close()
            return bos.toByteArray()
        } catch (e: Exception) {
            return null
        } finally {
            decompressor.end()
        }
    }

    private fun extractPdfTextFromStream(streamText: String): String {
        val sb = StringBuilder()
        val matcher = Pattern.compile("\\((.*?)\\)\\s*T[jJ]").matcher(streamText)
        while (matcher.find()) {
            val text = matcher.group(1) ?: continue
            val clean = text.replace("\\\\", "").replace("\\(", "(").replace("\\)", ")")
            if (clean.length > 2 && !clean.all { it.isDigit() || it.isWhitespace() || it == '-' || it == '/' }) {
                sb.append(clean).append(" ")
            }
        }
        return sb.toString().trim()
    }
}
