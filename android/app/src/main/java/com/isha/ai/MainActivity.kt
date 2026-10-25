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
import android.widget.RadioButton
import android.widget.RadioGroup
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

        val tabModels = findViewById<Button>(R.id.tabModels)
        val viewModelsHub = findViewById<ScrollView>(R.id.viewModelsHub)

        fun selectTab(selectedTab: Button, targetView: View) {
            val tabs = listOf(tabChat, tabLibrary, tabModels)
            val views = listOf(viewChatConsole, viewDocLibrary, viewModelsHub)
            for (t in tabs) {
                if (t == selectedTab) {
                    t?.backgroundTintList = android.content.res.ColorStateList.valueOf(android.graphics.Color.parseColor("#F59E0B"))
                    t?.setTextColor(android.graphics.Color.WHITE)
                } else {
                    t?.backgroundTintList = android.content.res.ColorStateList.valueOf(android.graphics.Color.parseColor("#F1F5F9"))
                    t?.setTextColor(android.graphics.Color.parseColor("#475569"))
                }
            }
            for (v in views) {
                v?.visibility = if (v == targetView) View.VISIBLE else View.GONE
            }
        }

        tabChat.setOnClickListener {
            selectTab(tabChat, viewChatConsole)
        }

        tabLibrary.setOnClickListener {
            selectTab(tabLibrary, viewDocLibrary)
            refreshDocLibrary(layoutDocList, txtIndexChunks, txtIndexSize)
        }

        var updateAllModelCards: (() -> Unit)? = null

        tabModels?.setOnClickListener {
            selectTab(tabModels, viewModelsHub)
            updateAllModelCards?.invoke()
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

        // AI Image Generation Trigger Button (FLUX.1 Schnell)
        val btnGenerateImage = findViewById<Button>(R.id.btnGenerateImage)
        btnGenerateImage?.setOnClickListener {
            val editP = findViewById<EditText>(R.id.editPrompt)
            editP.setText("Generate image of ")
            editP.setSelection(editP.text.length)
            editP.requestFocus()
            findViewById<Button>(R.id.tabChat)?.performClick()
            Toast.makeText(this, "Enter prompt to generate high-res FLUX.1 visual", Toast.LENGTH_SHORT).show()
        }

        // Multimodal Visual QA Action Buttons
        val btnAnalyzeAttachedImage = findViewById<Button>(R.id.btnAnalyzeAttachedImage)
        val chipAnalyzeCrop = findViewById<Button>(R.id.chipAnalyzeCrop)
        val chipAnalyzeMeds = findViewById<Button>(R.id.chipAnalyzeMeds)
        val chipAnalyzeGeneral = findViewById<Button>(R.id.chipAnalyzeGeneral)

        btnAnalyzeAttachedImage?.setOnClickListener {
            val editP = findViewById<EditText>(R.id.editPrompt)
            editP.setText("Analyze this attached image in detail and describe its features.")
            findViewById<Button>(R.id.btnSend)?.performClick()
        }

        chipAnalyzeCrop?.setOnClickListener {
            val editP = findViewById<EditText>(R.id.editPrompt)
            editP.setText("Inspect this crop leaf image for pests, fungal rust, or disease symptoms and recommend sprays.")
            findViewById<Button>(R.id.btnSend)?.performClick()
        }

        chipAnalyzeMeds?.setOnClickListener {
            val editP = findViewById<EditText>(R.id.editPrompt)
            editP.setText("Transcribe this prescription and medicine packaging, checking pediatric and adult dosage guidelines.")
            findViewById<Button>(R.id.btnSend)?.performClick()
        }

        chipAnalyzeGeneral?.setOnClickListener {
            val editP = findViewById<EditText>(R.id.editPrompt)
            editP.setText("Identify and describe all objects, text, and scene context in this photo.")
            findViewById<Button>(R.id.btnSend)?.performClick()
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

            // Clear previous image cards
            cardWebImage.visibility = View.GONE
            imgWebResult.setImageDrawable(null)

            val cardGeneratedImage = findViewById<LinearLayout>(R.id.cardGeneratedImage)
            val imgGeneratedResult = findViewById<ImageView>(R.id.imgGeneratedResult)
            val txtGeneratedPrompt = findViewById<TextView>(R.id.txtGeneratedPrompt)
            cardGeneratedImage?.visibility = View.GONE
            imgGeneratedResult?.setImageDrawable(null)

            // Check if this is an AI Image Generation Request (FLUX.1 Schnell)
            if (IshaImageGenerator.isImageGenerationIntent(queryText)) {
                val corePrompt = IshaImageGenerator.extractImagePrompt(queryText)
                txtChatLog.text = "User: $queryText\n\nAI: 🎨 Generating high-resolution visual with FLUX.1 [schnell]..."
                mainScope.launch {
                    val bitmap = IshaImageGenerator.generateImage(corePrompt)
                    if (bitmap != null) {
                        imgGeneratedResult?.setImageBitmap(bitmap)
                        txtGeneratedPrompt?.text = "Prompt: \"$corePrompt\""
                        cardGeneratedImage?.visibility = View.VISIBLE
                        txtChatLog.text = "User: $queryText\n\nAI: ✨ Here is your generated visual! Powered by FLUX.1 [schnell]."
                    } else {
                        txtChatLog.text = "User: $queryText\n\nAI: ❌ Failed to generate visual. Please check internet connection."
                    }
                }
                return@setOnClickListener
            }

            // Extract Base64 if image is attached (for Multimodal Visual QA)
            var attachedBase64: String? = null
            if (attachedImageUri != null) {
                try {
                    val stream = contentResolver.openInputStream(attachedImageUri!!)
                    val bytes = stream?.readBytes()
                    stream?.close()
                    if (bytes != null) {
                        attachedBase64 = android.util.Base64.encodeToString(bytes, android.util.Base64.NO_WRAP)
                    }
                } catch (e: Exception) {
                    Log.w(TAG, "Failed to encode image to base64: ${e.message}")
                }
            }

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
                // Priority 1: Free Cloud AI Offload (Zero Hallucination, Instant GPT-4 / Gemini Quality)
                val fullContext = contextBuilder.toString().trim()
                var cloudHandled = false
                if (IshaCloudAI.isCloudAvailable(this@MainActivity)) {
                    renderFormattedChat(queryText, "☁️ Thinking with Cloud AI...")
                    val buffer = StringBuilder()
                    var lastUpdate = SystemClock.elapsedRealtime()
                    val success = IshaCloudAI.streamCloudInference(
                        this@MainActivity,
                        queryText,
                        fullContext,
                        attachedBase64
                    ) { token ->
                        buffer.append(token)
                        val now = SystemClock.elapsedRealtime()
                        if (now - lastUpdate >= 50) {
                            renderFormattedChat(queryText, buffer.toString())
                            lastUpdate = now
                        }
                    }
                    if (success && buffer.isNotEmpty()) {
                        renderFormattedChat(queryText, buffer.toString())
                        cloudHandled = true
                    }
                }

                if (!cloudHandled) {
                    if (isNeuralModelLoaded && webSources.isEmpty()) {
                        // Stream from loaded GGUF neural model for offline reasoning
                        val finalPrompt = if (fullContext.isNotEmpty()) {
                            "<|im_start|>system\nYou are ISHA AI. Answer using ONLY verified context:\n$fullContext\n<|im_end|>\n<|im_start|>user\n$queryText<|im_end|>\n<|im_start|>assistant\n"
                        } else {
                            "<|im_start|>system\nYou are ISHA AI, an accurate on-device assistant. Answer factually.<|im_end|>\n<|im_start|>user\n$queryText<|im_end|>\n<|im_start|>assistant\n"
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
        }

                // Free Cloud AI & Hybrid Routing Controls
        val rgCloudProvider = findViewById<RadioGroup>(R.id.rgCloudProvider)
        val rbProviderHybrid = findViewById<RadioButton>(R.id.rbProviderHybrid)
        val rbProviderGroq = findViewById<RadioButton>(R.id.rbProviderGroq)
        val rbProviderGemini = findViewById<RadioButton>(R.id.rbProviderGemini)
        val editCloudApiKey = findViewById<EditText>(R.id.editCloudApiKey)
        val btnSaveCloudKey = findViewById<Button>(R.id.btnSaveCloudKey)
        val btnClearCloudKey = findViewById<Button>(R.id.btnClearCloudKey)
        val txtCloudStatus = findViewById<TextView>(R.id.txtCloudStatus)

        fun updateCloudStatusText() {
            val key = IshaCloudAI.getApiKey(this)
            val prov = IshaCloudAI.getProvider(this)
            if (key.isNotEmpty()) {
                val masked = if (key.length > 8) "${key.take(4)}...${key.takeLast(4)}" else "***"
                txtCloudStatus.text = "Status: Connected ($prov: $masked)"
                txtCloudStatus.setTextColor(android.graphics.Color.parseColor("#10B981"))
            } else {
                txtCloudStatus.text = "Status: Hybrid Grounding (Web RAG + Cognitive Active)"
                txtCloudStatus.setTextColor(android.graphics.Color.parseColor("#0284C7"))
            }
        }

        val savedProv = IshaCloudAI.getProvider(this)
        when (savedProv) {
            IshaCloudAI.PROVIDER_GROQ -> rbProviderGroq.isChecked = true
            IshaCloudAI.PROVIDER_GEMINI -> rbProviderGemini.isChecked = true
            else -> rbProviderHybrid.isChecked = true
        }
        val savedKey = IshaCloudAI.getApiKey(this)
        if (savedKey.isNotEmpty()) {
            editCloudApiKey.setText(savedKey)
        }
        updateCloudStatusText()

        btnSaveCloudKey.setOnClickListener {
            val enteredKey = editCloudApiKey.text.toString().trim()
            val selectedProv = when {
                rbProviderGroq.isChecked -> IshaCloudAI.PROVIDER_GROQ
                rbProviderGemini.isChecked -> IshaCloudAI.PROVIDER_GEMINI
                else -> IshaCloudAI.PROVIDER_HYBRID
            }
            IshaCloudAI.saveConfig(this, selectedProv, enteredKey)
            updateCloudStatusText()
            Toast.makeText(this, "Cloud AI configuration saved!", Toast.LENGTH_SHORT).show()
        }

        btnClearCloudKey.setOnClickListener {
            IshaCloudAI.clearConfig(this)
            editCloudApiKey.text.clear()
            rbProviderHybrid.isChecked = true
            updateCloudStatusText()
            Toast.makeText(this, "Cloud configuration reset to local hybrid.", Toast.LENGTH_SHORT).show()
        }

        rgCloudProvider.setOnCheckedChangeListener { _, checkedId ->
            val selectedProv = when (checkedId) {
                R.id.rbProviderGroq -> IshaCloudAI.PROVIDER_GROQ
                R.id.rbProviderGemini -> IshaCloudAI.PROVIDER_GEMINI
                else -> IshaCloudAI.PROVIDER_HYBRID
            }
            IshaCloudAI.saveConfig(this, selectedProv, editCloudApiKey.text.toString().trim())
            updateCloudStatusText()
        }

        btnOpenRecovery.setOnClickListener {
            val intent = Intent(this, RecoveryActivity::class.java)
            startActivity(intent)
        }

        // Model Hub: Device Tier & Telemetry
        val txtHubDeviceTier = findViewById<TextView>(R.id.txtHubDeviceTier)
        val txtHubActiveEngine = findViewById<TextView>(R.id.txtHubActiveEngine)
        txtHubDeviceTier?.text = when (deviceTier) {
            1 -> "Tier 1: Budget Mobile (₹8k–₹12k • ${totalRamMb}MB RAM)\nStrategy: 100% Free Cloud Offload + Zero-RAM Local RAG"
            2 -> "Tier 2: Mid-Range (₹13k–₹20k • ${totalRamMb}MB RAM)\nStrategy: Hybrid Execution (GGUF SLMs + Cloud Offload)"
            else -> "Tier 3: Flagship (₹25k+ • ${totalRamMb}MB RAM)\nStrategy: 100% On-Device SLM + NPU Vision"
        }

        // Model Hub: Cloud Models Selector
        val rgHubCloudModels = findViewById<RadioGroup>(R.id.rgHubCloudModels)
        val rbHubGeminiFlash = findViewById<RadioButton>(R.id.rbHubGeminiFlash)
        val rbHubGroqLlama70B = findViewById<RadioButton>(R.id.rbHubGroqLlama70B)
        val rbHubGroqLlama3B = findViewById<RadioButton>(R.id.rbHubGroqLlama3B)
        val rbHubGroqMistral = findViewById<RadioButton>(R.id.rbHubGroqMistral)
        val editHubApiKey = findViewById<EditText>(R.id.editHubApiKey)
        val btnHubSaveKey = findViewById<Button>(R.id.btnHubSaveKey)

        val activeCloudModel = IshaCloudAI.getSelectedModel(this)
        when (activeCloudModel) {
            IshaCloudAI.MODEL_GROQ_LLAMA_70B -> rbHubGroqLlama70B?.isChecked = true
            IshaCloudAI.MODEL_GROQ_LLAMA_3B -> rbHubGroqLlama3B?.isChecked = true
            IshaCloudAI.MODEL_GROQ_MISTRAL_7B -> rbHubGroqMistral?.isChecked = true
            else -> rbHubGeminiFlash?.isChecked = true
        }
        val existingKey = IshaCloudAI.getApiKey(this)
        if (existingKey.isNotEmpty()) {
            editHubApiKey?.setText(existingKey)
        }

        btnHubSaveKey?.setOnClickListener {
            val key = editHubApiKey?.text?.toString()?.trim() ?: ""
            val (prov, mod) = when {
                rbHubGroqLlama70B?.isChecked == true -> Pair(IshaCloudAI.PROVIDER_GROQ, IshaCloudAI.MODEL_GROQ_LLAMA_70B)
                rbHubGroqLlama3B?.isChecked == true -> Pair(IshaCloudAI.PROVIDER_GROQ, IshaCloudAI.MODEL_GROQ_LLAMA_3B)
                rbHubGroqMistral?.isChecked == true -> Pair(IshaCloudAI.PROVIDER_GROQ, IshaCloudAI.MODEL_GROQ_MISTRAL_7B)
                else -> Pair(IshaCloudAI.PROVIDER_GEMINI, IshaCloudAI.MODEL_GEMINI_FLASH)
            }
            IshaCloudAI.saveConfig(this, prov, key, mod)
            txtHubActiveEngine?.text = "Active Engine: $prov ($mod)"
            Toast.makeText(this, "Activated $mod successfully!", Toast.LENGTH_SHORT).show()
        }

        // Model Hub: Tier 2 On-Device SLMs (Qwen 0.5B, Llama 1B, Qwen 1.5B)
        val btnHubDownload05B = findViewById<Button>(R.id.btnHubDownload05B)
        val btnHubLoad05B = findViewById<Button>(R.id.btnHubLoad05B)
        val btnHubDelete05B = findViewById<Button>(R.id.btnHubDelete05B)
        val txtHubStatus05B = findViewById<TextView>(R.id.txtHubStatus05B)
        val layoutHubProgress05B = findViewById<LinearLayout>(R.id.layoutHubProgress05B)
        val txtHubProgress05B = findViewById<TextView>(R.id.txtHubProgress05B)
        val progressHub05B = findViewById<ProgressBar>(R.id.progressHub05B)

        val btnHubDownload1B = findViewById<Button>(R.id.btnHubDownload1B)
        val btnHubLoad1B = findViewById<Button>(R.id.btnHubLoad1B)
        val btnHubDelete1B = findViewById<Button>(R.id.btnHubDelete1B)
        val txtHubStatus1B = findViewById<TextView>(R.id.txtHubStatus1B)
        val layoutHubProgress1B = findViewById<LinearLayout>(R.id.layoutHubProgress1B)
        val txtHubProgress1B = findViewById<TextView>(R.id.txtHubProgress1B)
        val progressHub1B = findViewById<ProgressBar>(R.id.progressHub1B)

        val btnHubDownload15B = findViewById<Button>(R.id.btnHubDownload15B)
        val btnHubLoad15B = findViewById<Button>(R.id.btnHubLoad15B)
        val btnHubDelete15B = findViewById<Button>(R.id.btnHubDelete15B)
        val txtHubStatus15B = findViewById<TextView>(R.id.txtHubStatus15B)
        val layoutHubProgress15B = findViewById<LinearLayout>(R.id.layoutHubProgress15B)
        val txtHubProgress15B = findViewById<TextView>(R.id.txtHubProgress15B)
        val progressHub15B = findViewById<ProgressBar>(R.id.progressHub15B)

        val btnHubUnloadModel = findViewById<Button>(R.id.btnHubUnloadModel)

        updateAllModelCards = {
            // Model 1: Qwen 0.5B
            val f05 = File(filesDir, "qwen-0.5b.gguf")
            if (f05.exists() && f05.length() > 0) {
                val mb = f05.length() / (1024.0 * 1024.0)
                txtHubStatus05B?.text = String.format("🟢 Installed (%.1f MB)", mb)
                txtHubStatus05B?.setTextColor(android.graphics.Color.parseColor("#10B981"))
                btnHubDownload05B?.text = "Installed ✓"
                btnHubDownload05B?.isEnabled = false
                btnHubLoad05B?.isEnabled = true
                btnHubDelete05B?.isEnabled = true
                btnHubDelete05B?.visibility = View.VISIBLE
            } else {
                txtHubStatus05B?.text = "⚪ Not Downloaded (0 MB)"
                txtHubStatus05B?.setTextColor(android.graphics.Color.parseColor("#64748B"))
                btnHubDownload05B?.text = "⬇️ Download"
                btnHubDownload05B?.isEnabled = true
                btnHubLoad05B?.isEnabled = false
                btnHubDelete05B?.isEnabled = false
                btnHubDelete05B?.visibility = View.GONE
            }

            // Model 2: Llama 1B
            val f1 = File(filesDir, "llama-1b.gguf")
            if (f1.exists() && f1.length() > 0) {
                val mb = f1.length() / (1024.0 * 1024.0)
                txtHubStatus1B?.text = String.format("🟢 Installed (%.1f MB)", mb)
                txtHubStatus1B?.setTextColor(android.graphics.Color.parseColor("#10B981"))
                btnHubDownload1B?.text = "Installed ✓"
                btnHubDownload1B?.isEnabled = false
                btnHubLoad1B?.isEnabled = true
                btnHubDelete1B?.isEnabled = true
                btnHubDelete1B?.visibility = View.VISIBLE
            } else {
                txtHubStatus1B?.text = "⚪ Not Downloaded (0 MB)"
                txtHubStatus1B?.setTextColor(android.graphics.Color.parseColor("#64748B"))
                btnHubDownload1B?.text = "⬇️ Download"
                btnHubDownload1B?.isEnabled = true
                btnHubLoad1B?.isEnabled = false
                btnHubDelete1B?.isEnabled = false
                btnHubDelete1B?.visibility = View.GONE
            }

            // Model 3: Qwen 1.5B
            val f15 = File(filesDir, "qwen-1.5b.gguf")
            if (f15.exists() && f15.length() > 0) {
                val mb = f15.length() / (1024.0 * 1024.0)
                txtHubStatus15B?.text = String.format("🟢 Installed (%.1f MB)", mb)
                txtHubStatus15B?.setTextColor(android.graphics.Color.parseColor("#10B981"))
                btnHubDownload15B?.text = "Installed ✓"
                btnHubDownload15B?.isEnabled = false
                btnHubLoad15B?.isEnabled = true
                btnHubDelete15B?.isEnabled = true
                btnHubDelete15B?.visibility = View.VISIBLE
            } else {
                txtHubStatus15B?.text = "⚪ Not Downloaded (0 MB)"
                txtHubStatus15B?.setTextColor(android.graphics.Color.parseColor("#64748B"))
                btnHubDownload15B?.text = "⬇️ Download"
                btnHubDownload15B?.isEnabled = true
                btnHubLoad15B?.isEnabled = false
                btnHubDelete15B?.isEnabled = false
                btnHubDelete15B?.visibility = View.GONE
            }

            // Keep Diagnostics panel in sync
            btnDownloadT0?.text = if (f05.exists() && f05.length() > 0) "INSTALLED ✓" else "Download"
            btnDownloadT1?.text = if (f15.exists() && f15.length() > 0) "INSTALLED ✓" else "Download"
        }

        fun deleteModelFile(targetFileName: String, modelName: String) {
            IshaRuntime.unloadModelTier()
            isNeuralModelLoaded = false
            txtHubActiveEngine?.text = "Active Engine: UNLOADED (Using Hybrid Cloud & RAG)"
            txtActiveModel?.text = "Active Model: UNLOADED"

            val targetFile = File(filesDir, targetFileName)
            val partFile = File(cacheDir, "${targetFileName}.part")
            val existed = targetFile.exists()
            val freedBytes = if (existed) targetFile.length() else 0L

            if (partFile.exists()) partFile.delete()
            val deleted = targetFile.delete()

            updateAllModelCards?.invoke()

            if (deleted || !existed) {
                val freedMb = freedBytes / (1024.0 * 1024.0)
                val msg = "🗑️ Deleted $modelName! Freed %.1f MB storage.".format(freedMb)
                Toast.makeText(this, msg, Toast.LENGTH_SHORT).show()
                txtChatLog.text = "[System: $msg]"
            } else {
                Toast.makeText(this, "⚠️ Failed to delete $modelName file", Toast.LENGTH_SHORT).show()
            }
        }

        fun triggerDownloadModel(
            modelName: String,
            urlString: String,
            targetFileName: String,
            btnDownload: Button?,
            layoutProgress: LinearLayout?,
            txtProgress: TextView?,
            progressBar: ProgressBar?,
            txtStatus: TextView?,
            btnDelete: Button?,
            btnLoad: Button?
        ) {
            btnDownload?.isEnabled = false
            btnDownload?.text = "Downloading..."
            btnDelete?.isEnabled = false
            btnLoad?.isEnabled = false
            layoutProgress?.visibility = View.VISIBLE
            progressBar?.progress = 0
            txtProgress?.text = "Connecting to server..."
            txtStatus?.text = "⏳ Downloading..."
            txtStatus?.setTextColor(android.graphics.Color.parseColor("#2563EB"))

            progressDownload.visibility = View.VISIBLE
            progressDownload.progress = 0
            txtChatLog.text = "[System: Starting download of " + modelName + " weights...]\n"

            mainScope.launch(Dispatchers.IO) {
                try {
                    val url = java.net.URL(urlString)
                    val connection = url.openConnection() as java.net.HttpURLConnection
                    connection.connect()

                    if (connection.responseCode != java.net.HttpURLConnection.HTTP_OK) {
                        throw java.io.IOException("HTTP error code: ${connection.responseCode}")
                    }

                    val fileLength = connection.contentLengthLong
                    val input = connection.inputStream

                    val partFile = File(cacheDir, "${targetFileName}.part")
                    if (partFile.exists()) partFile.delete()

                    val output = java.io.FileOutputStream(partFile)
                    val data = ByteArray(16384)
                    var total: Long = 0
                    var count: Int
                    var lastUpdate = 0
                    val startTime = System.currentTimeMillis()

                    while (input.read(data).also { count = it } != -1) {
                        total += count
                        output.write(data, 0, count)
                        if (fileLength > 0) {
                            val currentProgress = ((total * 100) / fileLength).toInt()
                            if (currentProgress - lastUpdate >= 1 || total == fileLength) {
                                val downloadedMb = total / (1024.0 * 1024.0)
                                val totalMb = fileLength / (1024.0 * 1024.0)
                                val elapsedSec = (System.currentTimeMillis() - startTime) / 1000.0
                                val speedMbS = if (elapsedSec > 0.5) downloadedMb / elapsedSec else 0.0

                                mainScope.launch {
                                    progressBar?.progress = currentProgress
                                    progressDownload.progress = currentProgress
                                    txtProgress?.text = String.format("Downloading: %.1f MB / %.1f MB (%d%%) • %.1f MB/s",
                                        downloadedMb, totalMb, currentProgress, speedMbS)
                                    txtChatLog.text = String.format("[Downloading %s: %.1f MB / %.1f MB (%d%%)]",
                                        modelName.uppercase(), downloadedMb, totalMb, currentProgress)
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
                            layoutProgress?.visibility = View.GONE
                            progressDownload.visibility = View.GONE
                            updateAllModelCards?.invoke()
                            val totalMb = targetFile.length() / (1024.0 * 1024.0)
                            val successMsg = "✅ Installed $modelName successfully! (%.1f MB)".format(totalMb)
                            Toast.makeText(this@MainActivity, successMsg, Toast.LENGTH_SHORT).show()
                            txtChatLog.text = "[Success: Installed $modelName weights successfully! (%.1f MB)]".format(totalMb)
                        }
                    } else {
                        throw java.io.IOException("Failed to rename temporary download to target file")
                    }
                } catch (e: Exception) {
                    mainScope.launch {
                        layoutProgress?.visibility = View.GONE
                        progressDownload.visibility = View.GONE
                        updateAllModelCards?.invoke()
                        Toast.makeText(this@MainActivity, "❌ Download failed: ${e.message}", Toast.LENGTH_LONG).show()
                        txtChatLog.append("\n[System: Download failed: " + e.message + "]")
                    }
                }
            }
        }

        btnHubDownload05B?.setOnClickListener {
            triggerDownloadModel(
                "Qwen 2.5 0.5B",
                "https://huggingface.co/Qwen/Qwen2.5-0.5B-Instruct-GGUF/resolve/main/qwen2.5-0.5b-instruct-q4_k_m.gguf",
                "qwen-0.5b.gguf",
                btnHubDownload05B, layoutHubProgress05B, txtHubProgress05B, progressHub05B,
                txtHubStatus05B, btnHubDelete05B, btnHubLoad05B
            )
        }
        btnHubLoad05B?.setOnClickListener {
            val file = File(filesDir, "qwen-0.5b.gguf")
            if (file.exists() && IshaRuntime.loadModelTier("qwen-0.5b", 350)) {
                isNeuralModelLoaded = true
                txtHubActiveEngine?.text = "Active Engine: On-Device Qwen 2.5 0.5B (Active in RAM)"
                txtActiveModel?.text = "Active Model: Battery Saver (Qwen 0.5B)"
                txtHubStatus05B?.text = String.format("🟣 Active in RAM (%.1f MB)", file.length() / (1024.0 * 1024.0))
                txtHubStatus05B?.setTextColor(android.graphics.Color.parseColor("#7C3AED"))
                Toast.makeText(this, "Loaded Qwen 2.5 0.5B into RAM", Toast.LENGTH_SHORT).show()
            } else {
                Toast.makeText(this, "Model not downloaded yet. Tap Download first.", Toast.LENGTH_SHORT).show()
            }
        }
        btnHubDelete05B?.setOnClickListener {
            deleteModelFile("qwen-0.5b.gguf", "Qwen 2.5 0.5B")
        }

        btnHubDownload1B?.setOnClickListener {
            triggerDownloadModel(
                "Llama 3.2 1B",
                "https://huggingface.co/bartowski/Llama-3.2-1B-Instruct-GGUF/resolve/main/Llama-3.2-1B-Instruct-Q4_K_M.gguf",
                "llama-1b.gguf",
                btnHubDownload1B, layoutHubProgress1B, txtHubProgress1B, progressHub1B,
                txtHubStatus1B, btnHubDelete1B, btnHubLoad1B
            )
        }
        btnHubLoad1B?.setOnClickListener {
            val file = File(filesDir, "llama-1b.gguf")
            if (file.exists() && IshaRuntime.loadModelTier("llama-1b", 500)) {
                isNeuralModelLoaded = true
                txtHubActiveEngine?.text = "Active Engine: On-Device Llama 3.2 1B (Active in RAM)"
                txtActiveModel?.text = "Active Model: Llama 3.2 1B"
                txtHubStatus1B?.text = String.format("🟣 Active in RAM (%.1f MB)", file.length() / (1024.0 * 1024.0))
                txtHubStatus1B?.setTextColor(android.graphics.Color.parseColor("#7C3AED"))
                Toast.makeText(this, "Loaded Llama 3.2 1B into RAM", Toast.LENGTH_SHORT).show()
            } else {
                Toast.makeText(this, "Model not downloaded yet. Tap Download first.", Toast.LENGTH_SHORT).show()
            }
        }
        btnHubDelete1B?.setOnClickListener {
            deleteModelFile("llama-1b.gguf", "Llama 3.2 1B")
        }

        btnHubDownload15B?.setOnClickListener {
            triggerDownloadModel(
                "Qwen 2.5 1.5B",
                "https://huggingface.co/Qwen/Qwen2.5-1.5B-Instruct-GGUF/resolve/main/qwen2.5-1.5b-instruct-q4_k_m.gguf",
                "qwen-1.5b.gguf",
                btnHubDownload15B, layoutHubProgress15B, txtHubProgress15B, progressHub15B,
                txtHubStatus15B, btnHubDelete15B, btnHubLoad15B
            )
        }
        btnHubLoad15B?.setOnClickListener {
            if (deviceTier == 1) {
                Toast.makeText(this, "Advisory: Running 1.5B on 3-4GB phones may trigger OS memory pressure.", Toast.LENGTH_LONG).show()
            }
            val file = File(filesDir, "qwen-1.5b.gguf")
            if (file.exists() && IshaRuntime.loadModelTier("qwen-1.5b", 700)) {
                isNeuralModelLoaded = true
                txtHubActiveEngine?.text = "Active Engine: On-Device Qwen 2.5 1.5B (Active in RAM)"
                txtActiveModel?.text = "Active Model: Standard (Qwen 1.5B)"
                txtHubStatus15B?.text = String.format("🟣 Active in RAM (%.1f MB)", file.length() / (1024.0 * 1024.0))
                txtHubStatus15B?.setTextColor(android.graphics.Color.parseColor("#7C3AED"))
                Toast.makeText(this, "Loaded Qwen 2.5 1.5B into RAM", Toast.LENGTH_SHORT).show()
            } else {
                Toast.makeText(this, "Model not downloaded yet. Tap Download first.", Toast.LENGTH_SHORT).show()
            }
        }
        btnHubDelete15B?.setOnClickListener {
            deleteModelFile("qwen-1.5b.gguf", "Qwen 2.5 1.5B")
        }

        btnHubUnloadModel?.setOnClickListener {
            IshaRuntime.unloadModelTier()
            isNeuralModelLoaded = false
            txtHubActiveEngine?.text = "Active Engine: UNLOADED (Using Hybrid Cloud & RAG)"
            txtActiveModel?.text = "Active Model: UNLOADED"
            updateAllModelCards?.invoke()
            Toast.makeText(this, "Model unloaded from RAM", Toast.LENGTH_SHORT).show()
        }

        // Diagnostics Panel Bridge
        btnDownloadT0.setOnClickListener { btnHubDownload05B?.performClick() }
        btnDownloadT1.setOnClickListener { btnHubDownload15B?.performClick() }
        btnLoadT0.setOnClickListener { btnHubLoad05B?.performClick() }
        btnLoadT1.setOnClickListener { btnHubLoad15B?.performClick() }
        btnUnload.setOnClickListener { btnHubUnloadModel?.performClick() }
        btnDeleteT0.setOnClickListener { btnHubDelete05B?.performClick() }
        btnDeleteT1.setOnClickListener { btnHubDelete15B?.performClick() }

        // Initial scan of installed models
        updateAllModelCards?.invoke()

        // Model Hub: Image Generation Test
        val editHubImagePrompt = findViewById<EditText>(R.id.editHubImagePrompt)
        val btnHubGenerateImage = findViewById<Button>(R.id.btnHubGenerateImage)
        btnHubGenerateImage?.setOnClickListener {
            val prompt = editHubImagePrompt?.text?.toString()?.trim() ?: "Cybernetic tiger in neon Tokyo, 8k"
            val editPrompt = findViewById<EditText>(R.id.editPrompt)
            editPrompt.setText("Generate image: $prompt")
            findViewById<Button>(R.id.tabChat)?.performClick()
            findViewById<Button>(R.id.btnSend)?.performClick()
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
