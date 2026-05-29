package com.isha.ai

import android.content.Context
import android.net.Uri
import android.provider.OpenableColumns
import android.util.Log
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.withContext
import java.io.InputStream
import java.security.MessageDigest

class IshaRAGManager(private val context: Context) {
    private val TAG = "IshaRAGManager"

    interface IngestionCallback {
        fun onProgress(percentage: Int, status: String)
        fun onSuccess(chunkCount: Int)
        fun onFailure(error: String)
    }

    suspend fun ingestDocument(uri: Uri, mimeType: String?, callback: IngestionCallback) = withContext(Dispatchers.IO) {
        try {
            val fileName = getFileName(uri) ?: "unknown_document"
            val fileSize = getFileSize(uri)

            // Step 1: Compute hash of the document stream (Issue 6 - duplicate avoidance)
            callback.onProgress(5, "Calculating document hash...")
            val hash = calculateUriHash(uri) ?: throw Exception("Failed to compute hash")

            val db = context.db
            val existingHash = db.getDocumentHash(fileName)
            if (existingHash == hash) {
                Log.i(TAG, "Document unchanged. Skipping re-indexing: $fileName")
                callback.onProgress(100, "Document already indexed (hash matches).")
                callback.onSuccess(0)
                return@withContext
            }

            callback.onProgress(10, "Parsing document streams...")
            val pages = mutableListOf<Pair<Int, String>>()
            
            // Step 2: Stream page extraction (Issue 2 - heap protection)
            IshaDocumentParser.parseUriStreamed(context, uri, mimeType, object : IshaDocumentParser.ParserListener {
                override fun onPageParsed(pageNum: Int, text: String) {
                    pages.add(Pair(pageNum, text))
                }
            })

            if (pages.isEmpty()) {
                throw Exception("No text extracted from document.")
            }

            callback.onProgress(30, "Chunking content...")
            // Step 3: Paragraph/sentence boundary aware chunking (Issue 8)
            val chunksToIngest = mutableListOf<Triple<String, String, Int>>() // Triple(ChunkId, Text, PageNum)
            var chunkIndex = 0
            
            for (page in pages) {
                val pageText = page.second
                val pageNum = page.first
                val chunks = semanticChunkText(pageText, 250, 32)
                for (chunk in chunks) {
                    val chunkId = "${fileName}_p${pageNum}_c${chunkIndex++}"
                    chunksToIngest.add(Triple(chunkId, chunk, pageNum))
                }
            }

            callback.onProgress(40, "Registering metadata...")
            val docId = db.registerDocument(fileName, hash, fileSize)

            callback.onProgress(45, "Encoding local embeddings...")
            val dbChunksList = mutableListOf<Pair<Int, String>>()
            val dbChunkHashes = mutableListOf<String>()

            // Step 4: Adaptive embedding batching based on thermals (Issue 3)
            var currentIdx = 0
            val totalChunks = chunksToIngest.size
            
            while (currentIdx < totalChunks) {
                val thermalState = IshaRuntime.getThermalState()
                
                // ZONE 3 / 4 Thermal Lockout - Indexing pauses
                if (thermalState >= 2) {
                    callback.onProgress((45 + (currentIdx * 50 / totalChunks)), "Thermal throttling active. Pausing ingestion...")
                    delay(2000)
                    continue
                }

                // Adaptive batch size calculation
                val batchSize = if (thermalState == 1) 4 else 16 // Zone 2: batch=4, Zone 1: batch=16
                val limit = minOf(currentIdx + batchSize, totalChunks)

                for (i in currentIdx until limit) {
                    val chunk = chunksToIngest[i]
                    val isLast = (i == totalChunks - 1)
                    
                    val success = IshaRuntime.ingestChunkNative(chunk.first, chunk.second, "local_rag", isLast)
                    if (!success) {
                        throw Exception("Native embedding encoding failed for chunk ${chunk.first}")
                    }

                    dbChunksList.add(Pair(chunk.third, chunk.second))
                    dbChunkHashes.add(computeSha256(chunk.second))
                }

                currentIdx = limit
                val progress = 45 + (currentIdx * 50 / totalChunks)
                callback.onProgress(progress, "Encoded $currentIdx of $totalChunks chunks...")

                // Cooling off delay if in Zone 2
                if (thermalState == 1) {
                    delay(800)
                }
            }

            // Step 5: Batched SQLite Transaction Write (Issue 5)
            callback.onProgress(95, "Committing index database...")
            db.insertChunksBatched(docId, dbChunksList, dbChunkHashes)
            db.updateDocChunkCount(docId, totalChunks)

            callback.onProgress(100, "Ingestion completed successfully.")
            callback.onSuccess(totalChunks)

        } catch (e: Exception) {
            Log.e(TAG, "Ingestion failure: ${e.message}")
            callback.onFailure(e.message ?: "Unknown error")
        }
    }

    private fun getFileName(uri: Uri): String? {
        var name: String? = null
        if (uri.scheme == "content") {
            val cursor = context.contentResolver.query(uri, null, null, null, null)
            if (cursor != null && cursor.moveToFirst()) {
                val idx = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME)
                if (idx != -1) name = cursor.getString(idx)
                cursor.close()
            }
        }
        if (name == null) {
            name = uri.path
            val cut = name?.lastIndexOf('/') ?: -1
            if (cut != -1) {
                name = name?.substring(cut + 1)
            }
        }
        return name
    }

    private fun getFileSize(uri: Uri): Long {
        var size = 0L
        if (uri.scheme == "content") {
            val cursor = context.contentResolver.query(uri, null, null, null, null)
            if (cursor != null && cursor.moveToFirst()) {
                val idx = cursor.getColumnIndex(OpenableColumns.SIZE)
                if (idx != -1) size = cursor.getLong(idx)
                cursor.close()
            }
        }
        return size
    }

    private fun calculateUriHash(uri: Uri): String? {
        return try {
            val digest = MessageDigest.getInstance("MD5")
            val stream = context.contentResolver.openInputStream(uri) ?: return null
            val buffer = ByteArray(8192)
            var read: Int
            while (stream.read(buffer).also { read = it } > 0) {
                digest.update(buffer, 0, read)
            }
            stream.close()
            val hashBytes = digest.digest()
            val sb = StringBuilder()
            for (b in hashBytes) {
                sb.append(String.format("%02x", b))
            }
            sb.toString()
        } catch (e: Exception) {
            null
        }
    }

    private fun computeSha256(text: String): String {
        val digest = MessageDigest.getInstance("SHA-256")
        val hash = digest.digest(text.toByteArray(Charsets.UTF_8))
        val sb = StringBuilder()
        for (b in hash) {
            sb.append(String.format("%02x", b))
        }
        return sb.toString()
    }

    // Semantic boundary protection chunking (Issue 8)
    private fun semanticChunkText(text: String, chunkSize: Int, overlap: Int): List<String> {
        val paragraphs = text.split("\n\n")
        val chunks = mutableListOf<String>()
        val currentWords = mutableListOf<String>()

        for (para in paragraphs) {
            // Keep table lines or list items together inside paragraphs
            val lines = para.split("\n")
            val paraWords = mutableListOf<String>()
            
            for (line in lines) {
                val words = line.trim().split(Regex("\\s+")).filter { it.isNotEmpty() }
                paraWords.addAll(words)
            }

            if (currentWords.size + paraWords.size > chunkSize && currentWords.isNotEmpty()) {
                chunks.add(currentWords.joinToString(" "))
                
                // Overlap
                val startIdx = maxOf(0, currentWords.size - overlap)
                val nextWords = currentWords.subList(startIdx, currentWords.size).toMutableList()
                currentWords.clear()
                currentWords.addAll(nextWords)
            }
            
            currentWords.addAll(paraWords)
        }

        if (currentWords.isNotEmpty()) {
            chunks.add(currentWords.joinToString(" "))
        }

        return chunks
    }
}
