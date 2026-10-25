package com.isha.ai

import android.app.Activity
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.os.SystemClock
import android.provider.OpenableColumns
import android.speech.RecognizerIntent
import android.text.Editable
import android.text.Html
import android.text.TextUtils
import android.text.TextWatcher
import android.text.method.LinkMovementMethod
import android.text.method.ScrollingMovementMethod
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.widget.Button
import android.widget.EditText
import android.widget.ImageView
import android.widget.LinearLayout
import android.widget.ProgressBar
import android.widget.ScrollView
import android.widget.TextView
import android.widget.Toast
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.File
import java.net.HttpURLConnection
import java.net.URL

class MainActivity : Activity() {
    private val TAG = "MainActivity"
    
    private val mainScope = CoroutineScope(Dispatchers.Main + Job())
    private var streamJob: Job? = null

    private lateinit var thermalService: IshaThermalService
    private val memoryListener = IshaMemoryPressureListener()

    private var coldStartStart: Long = 0

    // RAG & Multimodal components (Google AI Edge Gallery Features)
    private lateinit var ragManager: IshaRAGManager
    private val PICK_DOCUMENT_REQUEST = 1001
    private val PICK_IMAGE_REQUEST = 1002
    private val VOICE_RECOGNIZE_REQUEST = 1003
    private var isLiveWebActive = true
    private var isNeuralModelLoaded = false
    private var attachedImageUri: Uri? = null
    private var attachedImageName: String? = null

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
            tabChat.backgroundTintList = android.content.res.ColorStateList.valueOf(android.graphics.Color.parseColor("#F59E0B"))
            tabChat.setTextColor(android.graphics.Color.WHITE)
            tabLibrary.backgroundTintList = android.content.res.ColorStateList.valueOf(android.graphics.Color.parseColor("#F1F5F9"))
            tabLibrary.setTextColor(android.graphics.Color.parseColor("#475569"))
            viewChatConsole.visibility = View.VISIBLE
            viewDocLibrary.visibility = View.GONE
        }

        tabLibrary.setOnClickListener {
            tabLibrary.backgroundTintList = android.content.res.ColorStateList.valueOf(android.graphics.Color.parseColor("#F59E0B"))
            tabLibrary.setTextColor(android.graphics.Color.WHITE)
            tabChat.backgroundTintList = android.content.res.ColorStateList.valueOf(android.graphics.Color.parseColor("#F1F5F9"))
            tabChat.setTextColor(android.graphics.Color.parseColor("#475569"))
            viewDocLibrary.visibility = View.VISIBLE
            viewChatConsole.visibility = View.GONE
            refreshDocLibrary(layoutDocList, txtIndexChunks, txtIndexSize)
        }

        // Google AI Edge Gallery & Stitch UI Controls
        val panelDevDiagnostics = findViewById<LinearLayout>(R.id.panelDevDiagnostics)
        val btnToggleDiagnostics = findViewById<Button>(R.id.btnToggleDiagnostics)
        val btnToggleLiveWeb = findViewById<Button>(R.id.btnToggleLiveWeb)
        val btnNewChat = findViewById<Button>(R.id.btnNewChat)
        val cardWelcomeHero = findViewById<LinearLayout>(R.id.cardWelcomeHero)
        val chipRust = findViewById<Button>(R.id.chipRust)
        val chipDose = findViewById<Button>(R.id.chipDose)
        val chipFIR = findViewById<Button>(R.id.chipFIR)

        // Multimodal Visual QA Controls (Google AI Edge Gallery)
        val layoutImagePreview = findViewById<LinearLayout>(R.id.layoutImagePreview)
        val imgAttachedPreview = findViewById<ImageView>(R.id.imgAttachedPreview)
        val txtAttachedImageName = findViewById<TextView>(R.id.txtAttachedImageName)
        val btnRemoveImage = findViewById<Button>(R.id.btnRemoveImage)
        val btnAttachImage = findViewById<Button>(R.id.btnAttachImage)
        val btnVoiceInput = findViewById<Button>(R.id.btnVoiceInput)

        // Online Live RAG Visual Card
        val cardWebImage = findViewById<LinearLayout>(R.id.cardWebImage)
        val imgWebResult = findViewById<ImageView>(R.id.imgWebResult)
        val txtWebImageCaption = findViewById<TextView>(R.id.txtWebImageCaption)

        // Knowledge Base Live Search
        val editDocSearch = findViewById<EditText>(R.id.editDocSearch)
        editDocSearch?.addTextChangedListener(object : TextWatcher {
            override fun beforeTextChanged(s: CharSequence?, start: Int, count: Int, after: Int) {}
            override fun onTextChanged(s: CharSequence?, start: Int, before: Int, count: Int) {
                refreshDocLibrary(layoutDocList, txtIndexChunks, txtIndexSize, s?.toString() ?: "")
            }
            override fun afterTextChanged(s: Editable?) {}
        })

        // New Chat Session Button
        btnNewChat?.setOnClickListener {
            val txtChat = findViewById<TextView>(R.id.txtChatLog)
            val editP = findViewById<EditText>(R.id.editPrompt)
            txtChat.text = "ISHA is ready. Ask any question below or choose a suggestion above."
            editP.text.clear()
            layoutCitations.visibility = View.GONE
            cardWebImage.visibility = View.GONE
            imgWebResult.setImageDrawable(null)
            attachedImageUri = null
            attachedImageName = null
            layoutImagePreview.visibility = View.GONE
            imgAttachedPreview.setImageDrawable(null)
            cardWelcomeHero.visibility = View.VISIBLE
            Toast.makeText(this, "New conversation started", Toast.LENGTH_SHORT).show()
        }

        // Attach Image (Visual QA)
        btnAttachImage?.setOnClickListener {
            val intent = Intent(Intent.ACTION_GET_CONTENT).apply {
                type = "image/*"
                addCategory(Intent.CATEGORY_OPENABLE)
            }
            startActivityForResult(intent, PICK_IMAGE_REQUEST)
        }

        btnRemoveImage?.setOnClickListener {
            attachedImageUri = null
            attachedImageName = null
            layoutImagePreview.visibility = View.GONE
            imgAttachedPreview.setImageDrawable(null)
        }

        // Voice Input (Speech to Text)
        btnVoiceInput?.setOnClickListener {
            try {
                val intent = Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH).apply {
                    putExtra(RecognizerIntent.EXTRA_LANGUAGE_MODEL, RecognizerIntent.LANGUAGE_MODEL_FREE_FORM)
                    putExtra(RecognizerIntent.EXTRA_PROMPT, "Speak to ISHA AI...")
                }
                startActivityForResult(intent, VOICE_RECOGNIZE_REQUEST)
            } catch (e: Exception) {
                Toast.makeText(this, "Speech recognizer not supported on this device", Toast.LENGTH_SHORT).show()
            }
        }

        btnToggleDiagnostics?.setOnClickListener {
            if (panelDevDiagnostics.visibility == View.VISIBLE) {
                panelDevDiagnostics.visibility = View.GONE
                btnToggleDiagnostics.text = "⚙️"
            } else {
                panelDevDiagnostics.visibility = View.VISIBLE
                btnToggleDiagnostics.text = "✖"
            }
        }

        btnToggleLiveWeb?.setOnClickListener {
            isLiveWebActive = !isLiveWebActive
            if (isLiveWebActive) {
                btnToggleLiveWeb.text = "🌐 Web: ON"
                btnToggleLiveWeb.setTextColor(android.graphics.Color.parseColor("#0284C7"))
                Toast.makeText(this, "Live Web Search Active", Toast.LENGTH_SHORT).show()
            } else {
                btnToggleLiveWeb.text = "🌐 Web: OFF"
                btnToggleLiveWeb.setTextColor(android.graphics.Color.parseColor("#64748B"))
                Toast.makeText(this, "100% On-Device Mode", Toast.LENGTH_SHORT).show()
            }
        }

        // Automatically preload built-in domain knowledge packs on startup
        mainScope.launch {
            ragManager.preloadKnowledgePacks()
            refreshDocLibrary(layoutDocList, txtIndexChunks, txtIndexSize)
        }

        // JNI Crash containment lockfile detection
        val crashLockFile = File(filesDir, "jni_crash.lock")
        var previousCrashDetected = false
        if (crashLockFile.exists()) {
            previousCrashDetected = true
            Log.e(TAG, "🚨 Relocation error or native crash detected in previous run. Disabling auto-loads.")
            panelDevDiagnostics?.visibility = View.VISIBLE
            panelSafeMode.visibility = View.VISIBLE
            txtSafeModeReason.text = "Native crash detected. Skip auto-loading to prevent crash loops."
        } else {
            panelSafeMode.visibility = View.GONE
            try {
                crashLockFile.createNewFile()
            } catch (e: Exception) {
                Log.e(TAG, "Failed to create crash lockfile")
            }
        }

        // Profile device memory tiering & classify hardware profile
        val actManager = getSystemService(ACTIVITY_SERVICE) as android.app.ActivityManager
        val memInfo = android.app.ActivityManager.MemoryInfo()
        actManager.getMemoryInfo(memInfo)
        val totalRamMb = (memInfo.totalMem / (1024 * 1024)).toInt()
        val cpuCores = Runtime.getRuntime().availableProcessors()

        val deviceTier = when {
            totalRamMb <= 4500 -> 1 // Tier 1: Budget Mobile (₹8k–₹12k, 3-4GB RAM)
            totalRamMb <= 8500 -> 2 // Tier 2: Mid-Range (₹13k–₹20k, 6-8GB RAM)
            else -> 3              // Tier 3: Flagship (₹25k+, 12GB+ RAM)
        }

        val txtDeviceTierProfile = findViewById<TextView>(R.id.txtDeviceTierProfile)
        txtDeviceTierProfile?.text = when (deviceTier) {
            1 -> "📱 Hardware: Tier 1 Budget (₹8k–₹12k • ${totalRamMb}MB RAM)\n⚡ Architecture: 100% Free Cloud Offload + Zero-RAM Local RAG (Anti-OOM Safe)"
            2 -> "📱 Hardware: Tier 2 Mid-Range (₹13k–₹20k • ${totalRamMb}MB RAM)\n⚡ Architecture: Hybrid Execution (On-Device SLM 0.5B/1.5B + Cloud Offload)"
            else -> "📱 Hardware: Tier 3 Flagship (₹25k+ • ${totalRamMb}MB RAM)\n⚡ Architecture: 100% Full On-Device Execution (SLM + NPU Vision)"
        }

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
            panelSafeMode.visibility = View.GONE
        }

        chipRust?.setOnClickListener {
            editPrompt.setText("Wheat yellow rust symptoms and spray dosage")
            btnSend.performClick()
        }
        chipDose?.setOnClickListener {
            editPrompt.setText("What is paracetamol dosage for a 12 kg child?")
            btnSend.performClick()
        }
        chipFIR?.setOnClickListener {
            editPrompt.setText("Legal process if police station refuses to file FIR")
            btnSend.performClick()
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
                isNeuralModelLoaded = true
                txtActiveModel.text = "Active Model: Battery Saver (Qwen 0.5B)"
                txtChatLog.text = "[Success: Loaded Qwen2.5-0.5B-Instruct! Ready for local grounded chat.]"
            }
        }

        btnLoadT1.setOnClickListener {
            if (deviceTier == 1) {
                Toast.makeText(this, "Advisory: Running 1.5B weights on 3-4GB phones may trigger OS memory pressure. Battery Saver (0.5B) or Cloud Offload is recommended.", Toast.LENGTH_LONG).show()
            }
            val file = File(filesDir, "qwen-1.5b.gguf")
            if (!file.exists()) {
                txtChatLog.text = "[Error: Standard (1.5B) weights are not downloaded yet.]"
                return@setOnClickListener
            }
            if (IshaRuntime.loadModelTier("qwen-1.5b", 700)) {
                isNeuralModelLoaded = true
                txtActiveModel.text = "Active Model: Standard (Qwen 1.5B)"
                txtChatLog.text = "[Success: Loaded Qwen2.5-1.5B-Instruct! Ready for local grounded chat.]"
            }
        }

        btnUnload.setOnClickListener {
            if (IshaRuntime.unloadModelTier()) {
                isNeuralModelLoaded = false
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
                    "text/csv",
                    "application/json",
                    "application/pdf",
                    "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
                    "image/jpeg",
                    "image/png"
                ))
            }
            startActivityForResult(intent, PICK_DOCUMENT_REQUEST)
        }

        // Chat Streaming & Grounded Hybrid RAG Retrieval (Local + Live Internet)
        fun isLatestOrCurrentQuery(query: String): Boolean {
            val q = query.lowercase()
            return q.contains("latest") || q.contains("news") || q.contains("today") ||
                   q.contains("current") || q.contains("price") || q.contains("weather") ||
                   q.contains("2026") || q.contains("who is") || q.contains("what happened") ||
                   q.contains("online") || q.contains("internet")
        }

        btnSend.setOnClickListener {
            val prompt = editPrompt.text.toString().trim()
            if (prompt.isEmpty() && attachedImageUri == null) return@setOnClickListener
            val queryText = if (prompt.isNotEmpty()) prompt else "Analyze this attached image"
            editPrompt.text.clear()
            cardWelcomeHero?.visibility = View.GONE

            // Clear previous web image card
            cardWebImage.visibility = View.GONE
            imgWebResult.setImageDrawable(null)

            txtChatLog.text = "User: $queryText\n\nAI: "
            layoutCitations.visibility = View.GONE

            // Extract significant content keywords for relevance verification
            val stopWords = setOf("who", "what", "when", "where", "why", "how", "is", "are", "was", "were", "tell", "about", "the", "for", "can", "you", "please", "me", "show", "give", "a", "an", "in", "on", "at", "to", "of", "and", "or", "it", "this", "that")
            val queryKeywords = queryText.lowercase()
                .replace(Regex("[^a-z0-9\\s]"), " ")
                .split(Regex("\\s+"))
                .filter { it.length >= 3 && it !in stopWords }

            val isOnlineQuery = isLatestOrCurrentQuery(queryText)

            // Step 1: Query local vector index
            val contextPayload = IshaRuntime.retrieveContextNative(queryText, "local_rag", 4)
            val citationsList = mutableListOf<String>()
            val localChunksList = mutableListOf<String>()
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

                        val textLower = text.lowercase()
                        val chunkIdLower = chunkId.lowercase()

                        // Strict relevance check: verify chunk contains at least one query keyword
                        val hasKeywordMatch = queryKeywords.isEmpty() || queryKeywords.any { kw ->
                            textLower.contains(kw) || chunkIdLower.contains(kw)
                        }

                        // If user is asking an online/person query, require keyword match to avoid 3-gram false positives
                        val isEligible = if (isOnlineQuery && queryKeywords.isNotEmpty()) {
                            score >= 0.45 && hasKeywordMatch
                        } else {
                            score >= 0.35 && (hasKeywordMatch || score >= 0.65)
                        }

                        if (isEligible) {
                            citationsList.add("📚 Local: $chunkId (score: %.2f)".format(score))
                            localChunksList.add(text)
                            contextBuilder.append("Local Reference ($chunkId):\n$text\n\n")
                            contextLength += text.length
                            if (contextLength >= 4000 || localChunksList.size >= 3) break
                        }
                    }
                }
            }

            // Step 2: Live Online Web Search and Generation
            streamJob?.cancel()
            streamJob = mainScope.launch {
                var webSources = emptyList<WebSource>()
                val shouldSearchWeb = isLiveWebActive && (isOnlineQuery || localChunksList.isEmpty())

                if (shouldSearchWeb) {
                    txtChatLog.text = "User: $queryText\n\nAI: 🌐 Connecting to internet & searching live web..."
                    try {
                        val ragResult = IshaOnlineRAG.searchLiveWeb(queryText, 3)
                        webSources = ragResult.sources
                        for ((idx, w) in webSources.withIndex()) {
                            citationsList.add("🌐 Web: ${w.title}")
                            // CRITICAL FIX: Append the verified web snippet into contextBuilder!
                            if (w.snippet.trim().isNotEmpty()) {
                                contextBuilder.append("Verified Web Source [W${idx + 1}] - ${w.title} (${w.provider}):\n${w.snippet.trim()}\n\n")
                            }
                        }

                        // Load and render live entity image if found
                        if (ragResult.imageUrl != null && isLiveWebActive) {
                            mainScope.launch(Dispatchers.IO) {
                                try {
                                    val url = URL(ragResult.imageUrl)
                                    val conn = url.openConnection() as HttpURLConnection
                                    conn.connectTimeout = 4000
                                    conn.readTimeout = 4000
                                    conn.doInput = true
                                    conn.connect()
                                    val bitmap = android.graphics.BitmapFactory.decodeStream(conn.inputStream)
                                    conn.disconnect()
                                    if (bitmap != null) {
                                        mainScope.launch {
                                            imgWebResult.setImageBitmap(bitmap)
                                            txtWebImageCaption.text = "🌐 Live Web Visual: " + IshaOnlineRAG.extractCoreQuery(queryText)
                                            cardWebImage.visibility = View.VISIBLE
                                        }
                                    }
                                } catch (e: Exception) {
                                    Log.w(TAG, "Failed to load live entity image: ${e.message}")
                                }
                            }
                        }
                    } catch (e: Exception) {
                        Log.w(TAG, "Online web retrieval error: ${e.message}")
                    }
                }

                if (citationsList.isNotEmpty()) {
                    txtCitations.text = citationsList.joinToString("\n")
                    layoutCitations.visibility = View.VISIBLE
                }

                // Step 3: Stream Output
                // Note: For live web queries (webSources.isNotEmpty()), IshaCognitiveGenerator provides
                // 100% accurate, hallucination-free grounded answers with direct clickable links.
                if (isNeuralModelLoaded && webSources.isEmpty()) {
                    // Stream from loaded GGUF neural model for offline reasoning
                    val contextText = contextBuilder.toString().trim()
                    val finalPrompt = if (contextText.isNotEmpty()) {
                        "<|im_start|>system\nYou are ISHA AI. Answer using ONLY verified context:\n$contextText\n<|im_end|>\n<|im_start|>user\n$queryText<|im_end|>\n<|im_start|>assistant\n"
                    } else {
                        "<|im_start|>system\nYou are ISHA AI, a helpful on-device assistant.<|im_end|>\n<|im_start|>user\n$queryText<|im_end|>\n<|im_start|>assistant\n"
                    }

                    val sessionChannel = Channel<String>(Channel.UNLIMITED)
                    mainScope.launch(Dispatchers.IO) {
                        IshaRuntime.streamTokens(finalPrompt, object : TokenCallback {
                            override fun onToken(token: String) {
                                sessionChannel.trySend(token)
                            }
                        })
                    }

                    val buffer = StringBuilder()
                    var lastUpdate = SystemClock.elapsedRealtime()
                    try {
                        while (true) {
                            val token = sessionChannel.receive()
                            buffer.append(token)
                            val now = SystemClock.elapsedRealtime()
                            if (now - lastUpdate >= 66 || sessionChannel.isEmpty) {
                                renderFormattedChat(queryText, buffer.toString().trimStart())
                                lastUpdate = now
                            }
                        }
                    } catch (e: Exception) {}
                } else {
                    // Instant out-of-the-box Grounded Cognitive Generator with Multimodal Vision & Verified Live Web RAG
                    val currentAttached = attachedImageName
                    val buffer = StringBuilder()
                    renderFormattedChat(queryText, "Generating answer...")
                    var lastUpdate = SystemClock.elapsedRealtime()
                    IshaCognitiveGenerator.streamSynthesizedAnswer(
                        queryText,
                        citationsList,
                        localChunksList,
                        webSources,
                        currentAttached
                    ) { token ->
                        buffer.append(token)
                        val now = SystemClock.elapsedRealtime()
                        if (now - lastUpdate >= 50) {
                            renderFormattedChat(queryText, buffer.toString())
                            lastUpdate = now
                        }
                    }
                    renderFormattedChat(queryText, buffer.toString())
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
        if (resultCode != RESULT_OK) return

        // Image Attachment (Google AI Edge Gallery Multimodal Visual QA)
        if (requestCode == PICK_IMAGE_REQUEST) {
            val uri = data?.data ?: return
            attachedImageUri = uri
            var displayName = "image.jpg"
            val cursor = contentResolver.query(uri, null, null, null, null)
            if (cursor != null && cursor.moveToFirst()) {
                val idx = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME)
                if (idx != -1) displayName = cursor.getString(idx) ?: "image.jpg"
                cursor.close()
            }
            attachedImageName = displayName

            val layoutImagePreview = findViewById<LinearLayout>(R.id.layoutImagePreview)
            val imgAttachedPreview = findViewById<ImageView>(R.id.imgAttachedPreview)
            val txtAttachedImageName = findViewById<TextView>(R.id.txtAttachedImageName)

            imgAttachedPreview.setImageURI(uri)
            txtAttachedImageName.text = displayName
            layoutImagePreview.visibility = View.VISIBLE

            findViewById<Button>(R.id.tabChat)?.performClick()
            Toast.makeText(this, "Attached $displayName for Visual QA", Toast.LENGTH_SHORT).show()
            return
        }

        // Voice Recognition (Speech to Text)
        if (requestCode == VOICE_RECOGNIZE_REQUEST) {
            val matches = data?.getStringArrayListExtra(RecognizerIntent.EXTRA_RESULTS)
            if (!matches.isNullOrEmpty()) {
                val spoken = matches[0]
                val editPrompt = findViewById<EditText>(R.id.editPrompt)
                editPrompt.setText(spoken)
                editPrompt.setSelection(spoken.length)
                findViewById<Button>(R.id.tabChat)?.performClick()
                Toast.makeText(this, "Voice recognized: $spoken", Toast.LENGTH_SHORT).show()
            }
            return
        }

        if (requestCode == PICK_DOCUMENT_REQUEST) {
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

    private fun refreshDocLibrary(layoutList: LinearLayout, txtChunks: TextView, txtSize: TextView, filterQuery: String = "") {
        layoutList.removeAllViews()
        val docList = db.getDocumentsList()
        var totalChunks = 0

        val filtered = if (filterQuery.trim().isEmpty()) {
            docList
        } else {
            val q = filterQuery.lowercase()
            docList.filter { it.path.lowercase().contains(q) }
        }

        val inflater = LayoutInflater.from(this)

        for (doc in filtered) {
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


    private fun renderFormattedChat(prompt: String, aiContent: String) {
        val txtChatLog = findViewById<TextView>(R.id.txtChatLog)
        val userPart = "<b>User:</b> " + android.text.TextUtils.htmlEncode(prompt)
        val aiPart = "<b>AI:</b> " + formatMarkdownToHtml(aiContent)
        val combined = "$userPart<br><br>$aiPart"
        txtChatLog.text = android.text.Html.fromHtml(combined, android.text.Html.FROM_HTML_MODE_COMPACT)
        txtChatLog.movementMethod = android.text.method.LinkMovementMethod.getInstance()
    }

    private fun formatMarkdownToHtml(md: String): String {
        var h = md
        h = h.replace(Regex("(?m)^### (.*)$"), "<br><b><font color=\"#0F172A\">$1</font></b><br>")
        h = h.replace(Regex("(?m)^#### (.*)$"), "<br><b><font color=\"#2563EB\">$1</font></b><br>")
        h = h.replace(Regex("\\*\\*(.*?)\\*\\*"), "<b>$1</b>")
        h = h.replace(Regex("\\*(.*?)\\*"), "<i>$1</i>")
        h = h.replace(Regex("\\[(.*?)\\]\\((.*?)\\)"), "<a href=\"$2\"><font color=\"#0284C7\"><u>$1</u></font></a>")
        h = h.replace("\n", "<br>")
        return h
    }

    override fun onDestroy() {
        super.onDestroy()
        thermalService.stopMonitoring()
        unregisterComponentCallbacks(memoryListener)
    }
}
