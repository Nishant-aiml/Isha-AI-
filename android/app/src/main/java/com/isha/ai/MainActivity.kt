package com.isha.ai

import android.app.Activity
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.os.SystemClock
import android.text.method.ScrollingMovementMethod
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.widget.Button
import android.widget.EditText
import android.widget.LinearLayout
import android.widget.ProgressBar
import android.widget.TextView
import android.widget.ScrollView
import android.widget.Toast
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import java.io.File

class MainActivity : Activity() {
    private val TAG = "MainActivity"
    
    private val mainScope = CoroutineScope(Dispatchers.Main + Job())
    private var streamJob: Job? = null

    private lateinit var thermalService: IshaThermalService
    private val memoryListener = IshaMemoryPressureListener()

    private var coldStartStart: Long = 0

    // RAG components
    private lateinit var ragManager: IshaRAGManager
    private val PICK_DOCUMENT_REQUEST = 1001

    override fun onCreate(savedInstanceState: Bundle?) {
        coldStartStart = SystemClock.elapsedRealtime()
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        ragManager = IshaRAGManager(this)

        // Register memory pressure callbacks
        registerComponentCallbacks(memoryListener)

        // Initialize primary PowerManager thermal status listener
        thermalService = IshaThermalService(this)
        thermalService.startMonitoring()

        // Bind layout views
        val panelSafeMode = findViewById<LinearLayout>(R.id.panelSafeMode)
        val txtSafeModeReason = findViewById<TextView>(R.id.txtSafeModeReason)
        val txtColdStart = findViewById<TextView>(R.id.txtColdStart)
        val txtTokensPerSec = findViewById<TextView>(R.id.txtTokensPerSec)
        val txtActiveModel = findViewById<TextView>(R.id.txtActiveModel)
        val txtThermalZone = findViewById<TextView>(R.id.txtThermalZone)

        // Tab views
        val tabChat = findViewById<Button>(R.id.tabChat)
        val tabLibrary = findViewById<Button>(R.id.tabLibrary)
        val viewChatConsole = findViewById<ScrollView>(R.id.viewChatConsole)
        val viewDocLibrary = findViewById<LinearLayout>(R.id.viewDocLibrary)

        // RAG/Library views
        val btnPickDoc = findViewById<Button>(R.id.btnPickDoc)
        val layoutIngestProgress = findViewById<LinearLayout>(R.id.layoutIngestProgress)
        val progressIngest = findViewById<ProgressBar>(R.id.progressIngest)
        val txtIngestStatus = findViewById<TextView>(R.id.txtIngestStatus)
        val layoutDocList = findViewById<LinearLayout>(R.id.layoutDocList)
        val txtIndexChunks = findViewById<TextView>(R.id.txtIndexChunks)
        val txtIndexSize = findViewById<TextView>(R.id.txtIndexSize)

        // Grounded Citations
        val layoutCitations = findViewById<LinearLayout>(R.id.layoutCitations)
        val txtCitations = findViewById<TextView>(R.id.txtCitations)

        // Tab Switching Logic
        tabChat.setOnClickListener {
            tabChat.backgroundTintList = android.content.res.ColorStateList.valueOf(android.graphics.Color.parseColor("#2563EB"))
            tabChat.setTextColor(android.graphics.Color.WHITE)
            tabLibrary.backgroundTintList = android.content.res.ColorStateList.valueOf(android.graphics.Color.parseColor("#E5E7EB"))
            tabLibrary.setTextColor(android.graphics.Color.parseColor("#374151"))
            viewChatConsole.visibility = View.VISIBLE
            viewDocLibrary.visibility = View.GONE
        }

        tabLibrary.setOnClickListener {
            tabLibrary.backgroundTintList = android.content.res.ColorStateList.valueOf(android.graphics.Color.parseColor("#2563EB"))
            tabLibrary.setTextColor(android.graphics.Color.WHITE)
            tabChat.backgroundTintList = android.content.res.ColorStateList.valueOf(android.graphics.Color.parseColor("#E5E7EB"))
            tabChat.setTextColor(android.graphics.Color.parseColor("#374151"))
            viewDocLibrary.visibility = View.VISIBLE
            viewChatConsole.visibility = View.GONE
            refreshDocLibrary(layoutDocList, txtIndexChunks, txtIndexSize)
        }

        // JNI Crash containment lockfile detection
        val crashLockFile = File(filesDir, "jni_crash.lock")
        var previousCrashDetected = false
        if (crashLockFile.exists()) {
            previousCrashDetected = true
            Log.e(TAG, "🚨 Relocation error or native crash detected in previous run. Disabling auto-loads.")
            panelSafeMode.visibility = View.VISIBLE
            txtSafeModeReason.text = "Native crash detected. Skip auto-loading to prevent crash loops."
        } else {
            try {
                crashLockFile.createNewFile()
            } catch (e: Exception) {
                Log.e(TAG, "Failed to create crash lockfile")
            }
        }

        // Profile device memory tiering
        val actManager = getSystemService(ACTIVITY_SERVICE) as android.app.ActivityManager
        val memInfo = android.app.ActivityManager.MemoryInfo()
        actManager.getMemoryInfo(memInfo)
        val totalRamMb = (memInfo.totalMem / (1024 * 1024)).toInt()
        val cpuCores = Runtime.getRuntime().availableProcessors()

        // Init Isha Runtime natively
        val filesPath = filesDir.absolutePath
        val cachePath = cacheDir.absolutePath
        val initSuccess = IshaRuntime.initializeRuntime(filesPath, cachePath, totalRamMb, cpuCores)

        if (initSuccess && !previousCrashDetected) {
            if (crashLockFile.exists()) {
                crashLockFile.delete()
            }
        }
        
        val btnDownloadT0 = findViewById<Button>(R.id.btnDownloadT0)
        val btnDownloadT1 = findViewById<Button>(R.id.btnDownloadT1)
        val progressDownload = findViewById<ProgressBar>(R.id.progressDownload)
        
        val btnLoadT0 = findViewById<Button>(R.id.btnLoadT0)
        val btnLoadT1 = findViewById<Button>(R.id.btnLoadT1)
        val btnUnload = findViewById<Button>(R.id.btnUnload)

        val btnDeleteT0 = findViewById<Button>(R.id.btnDeleteT0)
        val btnDeleteT1 = findViewById<Button>(R.id.btnDeleteT1)
        
        val txtChatLog = findViewById<TextView>(R.id.txtChatLog)
        txtChatLog.movementMethod = ScrollingMovementMethod()
        
        val editPrompt = findViewById<EditText>(R.id.editPrompt)
        val btnSend = findViewById<Button>(R.id.btnSend)
        val btnOpenRecovery = findViewById<Button>(R.id.btnOpenRecovery)

        if (!previousCrashDetected) {
            panelSafeMode.visibility = View.VISIBLE
            txtSafeModeReason.text = String.format("Vivo T3 Hardware Profile: %d Core CPU, %dMB RAM. Qwen 1.5B Standard recommended.",
                cpuCores, totalRamMb)
        }

        // Display cold start timing
        val initEnd = SystemClock.elapsedRealtime()
        txtColdStart.text = String.format("Cold Start Latency: %d ms", initEnd - coldStartStart)

        // Initial setup for buttons based on file existence on boot
        if (File(filesDir, "qwen-0.5b.gguf").exists()) btnDownloadT0.text = "INSTALLED ✓"
        if (File(filesDir, "qwen-1.5b.gguf").exists()) btnDownloadT1.text = "INSTALLED ✓"

        // Model loaders
        btnLoadT0.setOnClickListener {
            val file = File(filesDir, "qwen-0.5b.gguf")
            if (!file.exists()) {
                txtChatLog.text = "[Error: Battery Saver (0.5B) weights are not downloaded yet.]"
                return@setOnClickListener
            }
            if (IshaRuntime.loadModelTier("qwen-0.5b", 84)) {
                txtActiveModel.text = "Active Model: Battery Saver (Qwen 0.5B)"
                txtChatLog.text = "[Success: Loaded Qwen2.5-0.5B-Instruct! Ready for local grounded chat.]"
            }
        }

        btnLoadT1.setOnClickListener {
            val file = File(filesDir, "qwen-1.5b.gguf")
            if (!file.exists()) {
                txtChatLog.text = "[Error: Standard (1.5B) weights are not downloaded yet.]"
                return@setOnClickListener
            }
            if (IshaRuntime.loadModelTier("qwen-1.5b", 700)) {
                txtActiveModel.text = "Active Model: Standard (Qwen 1.5B)"
                txtChatLog.text = "[Success: Loaded Qwen2.5-1.5B-Instruct! Ready for local grounded chat.]"
            }
        }

        btnUnload.setOnClickListener {
            if (IshaRuntime.unloadModelTier()) {
                txtActiveModel.text = "Active Model: UNLOADED"
                txtChatLog.text = "[Success: Model weights unloaded from RAM.]"
            }
        }

        btnDeleteT0.setOnClickListener {
            IshaRuntime.unloadModelTier()
            txtActiveModel.text = "Active Model: UNLOADED"
            val file = File(filesDir, "qwen-0.5b.gguf")
            if (file.exists() && file.delete()) {
                btnDownloadT0.text = "Download"
                txtChatLog.text = "[Success: Battery Saver model deleted.]"
            }
        }

        btnDeleteT1.setOnClickListener {
            IshaRuntime.unloadModelTier()
            txtActiveModel.text = "Active Model: UNLOADED"
            val file = File(filesDir, "qwen-1.5b.gguf")
            if (file.exists() && file.delete()) {
                btnDownloadT1.text = "Download"
                txtChatLog.text = "[Success: Standard model deleted.]"
            }
        }

        // Downloader
        fun triggerDownload(modelName: String, urlString: String, targetFileName: String, targetBtn: Button) {
            btnDownloadT0.isEnabled = false
            btnDownloadT1.isEnabled = false
            progressDownload.visibility = View.VISIBLE
            progressDownload.progress = 0
            txtChatLog.text = "[System: Starting download of $modelName weights...]\n"
            
            mainScope.launch(Dispatchers.IO) {
                try {
                    val url = java.net.URL(urlString)
                    val connection = url.openConnection() as java.net.HttpURLConnection
                    connection.connect()

                    if (connection.responseCode != java.net.HttpURLConnection.HTTP_OK) {
                        throw java.io.IOException("HTTP error code: " + connection.responseCode)
                    }

                    val fileLength = connection.contentLength
                    val input = connection.inputStream
                    
                    val partFile = File(cacheDir, "${targetFileName}.part")
                    if (partFile.exists()) partFile.delete()
                    
                    val output = java.io.FileOutputStream(partFile)
                    val data = ByteArray(8192)
                    var total: Long = 0
                    var count: Int
                    var lastUpdate = 0
                    
                    while (input.read(data).also { count = it } != -1) {
                        total += count
                        output.write(data, 0, count)
                        if (fileLength > 0) {
                            val currentProgress = ((total * 100) / fileLength).toInt()
                            if (currentProgress - lastUpdate >= 1) {
                                mainScope.launch {
                                    progressDownload.progress = currentProgress
                                    txtChatLog.text = String.format("[Downloading %s: %d%% completed]",
                                        modelName.uppercase(), currentProgress)
                                }
                                lastUpdate = currentProgress
                            }
                        }
                    }
                    output.flush()
                    output.close()
                    input.close()

                    val targetFile = File(filesDir, targetFileName)
                    if (targetFile.exists()) targetFile.delete()

                    if (partFile.renameTo(targetFile)) {
                        mainScope.launch {
                            progressDownload.visibility = View.GONE
                            btnDownloadT0.isEnabled = true
                            btnDownloadT1.isEnabled = true
                            targetBtn.text = "INSTALLED ✓"
                            txtChatLog.text = "[Success: Installed $modelName weights successfully!]"
                        }
                    }
                } catch (e: Exception) {
                    mainScope.launch {
                        btnDownloadT0.isEnabled = true
                        btnDownloadT1.isEnabled = true
                        txtChatLog.append("\n[System: Download failed: " + e.message + "]")
                    }
                }
            }
        }

        btnDownloadT0.setOnClickListener {
            triggerDownload("qwen-0.5b", "https://huggingface.co/Qwen/Qwen2.5-0.5B-Instruct-GGUF/resolve/main/qwen2.5-0.5b-instruct-q2_k.gguf", "qwen-0.5b.gguf", btnDownloadT0)
        }

        btnDownloadT1.setOnClickListener {
            triggerDownload("qwen-1.5b", "https://huggingface.co/Qwen/Qwen2.5-1.5B-Instruct-GGUF/resolve/main/qwen2.5-1.5b-instruct-q2_k.gguf", "qwen-1.5b.gguf", btnDownloadT1)
        }

        // Document Picker Triggers
        btnPickDoc.setOnClickListener {
            val intent = Intent(Intent.ACTION_OPEN_DOCUMENT).apply {
                addCategory(Intent.CATEGORY_OPENABLE)
                type = "*/*"
                putExtra(Intent.EXTRA_MIME_TYPES, arrayOf(
                    "text/plain",
                    "text/markdown",
                    "application/pdf",
                    "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
                    "image/jpeg",
                    "image/png"
                ))
            }
            startActivityForResult(intent, PICK_DOCUMENT_REQUEST)
        }

        // Chat Streaming & Context Retrieval
        btnSend.setOnClickListener {
            val prompt = editPrompt.text.toString().trim()
            if (prompt.isEmpty()) return@setOnClickListener
            editPrompt.text.clear()

            txtChatLog.text = "User: $prompt\n\nAI: "
            layoutCitations.visibility = View.GONE

            // Step 1: Query local vector index (Issue 4 - context caps)
            val contextPayload = IshaRuntime.retrieveContextNative(prompt, "local_rag", 4)
            val citationsList = mutableListOf<String>()
            val contextBuilder = StringBuilder()
            var contextLength = 0

            if (contextPayload.isNotEmpty()) {
                val lines = contextPayload.split("\n")
                for (line in lines) {
                    if (line.isEmpty()) continue
                    val parts = line.split("||")
                    if (parts.size >= 3) {
                        val chunkId = parts[0]
                        val text = parts[1]
                        val score = parts[2].toDoubleOrNull() ?: 0.0

                        // JNI retrieval already filters semantic matches at 0.35
                        // and applies BM25-lite fallback; accept all JNI outputs here.
                        citationsList.add("Source: $chunkId (score: %.2f)".format(score))
                        contextBuilder.append(text).append("\n\n")
                        contextLength += text.length
                        if (contextLength >= 6000 || citationsList.size >= 4) break
                    }
                }
            }

            // Document indexed check (Issue 10)
            val documentCount = db.getDocumentsList().size
            if (documentCount > 0 && citationsList.isEmpty()) {
                // Documents exist but similarity is below threshold: reject query safely to prevent hallucinations
                txtChatLog.text = "User: $prompt\n\nAI: I couldn't find enough reliable information in your documents."
                return@setOnClickListener
            }

            // Construct Grounded ChatTemplate (Issue 4 / Issue 10 compliance)
            val finalPrompt = if (citationsList.isNotEmpty()) {
                val contextText = contextBuilder.toString().trim()
                txtCitations.text = citationsList.joinToString("\n")
                layoutCitations.visibility = View.VISIBLE

                "<|im_start|>system\nYou are ISHA AI. Answer the user query using ONLY the verified context below. If the context does not contain the answer, say honestly that you do not know. DO NOT hallucinate.\n\n[CONTEXT]\n$contextText\n<|im_end|>\n<|im_start|>user\n$prompt<|im_end|>\n<|im_start|>assistant\n"
            } else {
                "<|im_start|>system\nYou are ISHA AI, a helpful, offline-first local assistant.<|im_end|>\n<|im_start|>user\n$prompt<|im_end|>\n<|im_start|>assistant\n"
            }

            streamJob?.cancel()
            val sessionChannel = Channel<String>(Channel.UNLIMITED)
            
            mainScope.launch(Dispatchers.IO) {
                IshaRuntime.streamTokens(finalPrompt, object : TokenCallback {
                    override fun onToken(token: String) {
                        sessionChannel.trySend(token)
                    }
                })
            }

            // 15Hz Refresh Gate UI loop (Phase 1 compliance)
            streamJob = mainScope.launch {
                val buffer = StringBuilder()
                var lastUpdate = SystemClock.elapsedRealtime()

                try {
                    while (true) {
                        val token = sessionChannel.receive()
                        buffer.append(token)

                        val now = SystemClock.elapsedRealtime()
                        if (now - lastUpdate >= 66 || sessionChannel.isEmpty) {
                            val cleanOutput = buffer.toString().trimStart()
                            txtChatLog.text = "User: $prompt\n\nAI: ${cleanOutput}"
                            lastUpdate = now
                        }
                    }
                } catch (e: Exception) {
                    // Channel closed or coroutine cancelled
                }
            }
        }

        btnOpenRecovery.setOnClickListener {
            val intent = Intent(this, RecoveryActivity::class.java)
            startActivity(intent)
        }

        // Thermal Monitoring Loop
        mainScope.launch {
            while (true) {
                delay(1000)
                val state = IshaRuntime.getThermalState()
                when (state) {
                    1 -> txtThermalZone.text = "Thermal State: Device warming up — optimizing performance."
                    2 -> txtThermalZone.text = "Thermal State: Throttling active. Indexing paused."
                    3 -> txtThermalZone.text = "Thermal State: Critical. Inference disabled."
                    else -> txtThermalZone.text = "Thermal State: NOMINAL"
                }
            }
        }
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode == PICK_DOCUMENT_REQUEST && resultCode == RESULT_OK) {
            val uri = data?.data ?: return
            
            // Acquire persistent URI read permissions (Scoped storage rules)
            try {
                contentResolver.takePersistableUriPermission(uri, Intent.FLAG_GRANT_READ_URI_PERMISSION)
            } catch (e: SecurityException) {
                Log.w(TAG, "Failed to acquire persistable URI permission")
            }

            val mimeType = contentResolver.getType(uri)
            val layoutIngestProgress = findViewById<LinearLayout>(R.id.layoutIngestProgress)
            val progressIngest = findViewById<ProgressBar>(R.id.progressIngest)
            val txtIngestStatus = findViewById<TextView>(R.id.txtIngestStatus)
            val layoutDocList = findViewById<LinearLayout>(R.id.layoutDocList)
            val txtIndexChunks = findViewById<TextView>(R.id.txtIndexChunks)
            val txtIndexSize = findViewById<TextView>(R.id.txtIndexSize)

            layoutIngestProgress.visibility = View.VISIBLE
            progressIngest.progress = 0

            mainScope.launch {
                ragManager.ingestDocument(uri, mimeType, object : IshaRAGManager.IngestionCallback {
                    override fun onProgress(percentage: Int, status: String) {
                        mainScope.launch {
                            progressIngest.progress = percentage
                            txtIngestStatus.text = status
                        }
                    }

                    override fun onSuccess(chunkCount: Int) {
                        mainScope.launch {
                            layoutIngestProgress.visibility = View.GONE
                            Toast.makeText(this@MainActivity, "Successfully indexed $chunkCount chunks!", Toast.LENGTH_LONG).show()
                            refreshDocLibrary(layoutDocList, txtIndexChunks, txtIndexSize)
                        }
                    }

                    override fun onFailure(error: String) {
                        mainScope.launch {
                            layoutIngestProgress.visibility = View.GONE
                            Toast.makeText(this@MainActivity, "Ingestion Failed: $error", Toast.LENGTH_LONG).show()
                        }
                    }
                })
            }
        }
    }

    private fun refreshDocLibrary(layoutList: LinearLayout, txtChunks: TextView, txtSize: TextView) {
        layoutList.removeAllViews()
        val docList = db.getDocumentsList()
        var totalChunks = 0

        val inflater = LayoutInflater.from(this)

        for (doc in docList) {
            totalChunks += doc.chunkCount
            val view = inflater.inflate(android.R.layout.simple_list_item_2, layoutList, false)
            val text1 = view.findViewById<TextView>(android.R.id.text1)
            val text2 = view.findViewById<TextView>(android.R.id.text2)

            text1.text = doc.path
            text1.setTextColor(android.graphics.Color.parseColor("#1F2937"))
            text1.textSize = 14f

            val sizeKb = doc.fileSize / 1024
            text2.text = "Size: %d KB | Chunks: %d".format(sizeKb, doc.chunkCount)
            text2.setTextColor(android.graphics.Color.parseColor("#6B7280"))
            text2.textSize = 12f

            // Add simple delete button to layout next to the list item
            val itemContainer = LinearLayout(this).apply {
                orientation = LinearLayout.HORIZONTAL
                layoutParams = LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT)
                setPadding(8, 8, 8, 8)
            }
            itemContainer.addView(view, LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1.0f))
            
            val deleteBtn = Button(this).apply {
                text = "Delete"
                textSize = 10f
                backgroundTintList = android.content.res.ColorStateList.valueOf(android.graphics.Color.parseColor("#EF4444"))
                setTextColor(android.graphics.Color.WHITE)
                setOnClickListener {
                    db.deleteDocument(doc.path)
                    IshaRuntime.clearPackNative("local_rag")
                    refreshDocLibrary(layoutList, txtChunks, txtSize)
                }
            }
            itemContainer.addView(deleteBtn, LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT))
            
            layoutList.addView(itemContainer)
        }

        txtChunks.text = "Active Chunks: $totalChunks"
        // Estimate vector file sizes: each chunk vector has 384 dimensions of 4-byte float32 vectors (~1.5KB per vector)
        val indexFile = File(filesDir, "rag_index.efvi")
        val sizeKb = if (indexFile.exists()) indexFile.length() / 1024 else 0
        txtSize.text = "Index Memory footprint: $sizeKb KB"
    }

    override fun onDestroy() {
        super.onDestroy()
        thermalService.stopMonitoring()
        unregisterComponentCallbacks(memoryListener)
    }
}
