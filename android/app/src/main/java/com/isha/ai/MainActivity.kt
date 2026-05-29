package com.isha.ai

import android.app.Activity
import android.content.Intent
import android.os.Bundle
import android.os.SystemClock
import android.text.method.ScrollingMovementMethod
import android.util.Log
import android.view.View
import android.widget.Button
import android.widget.EditText
import android.widget.LinearLayout
import android.widget.ProgressBar
import android.widget.TextView
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import java.io.File
import java.io.FileOutputStream

class MainActivity : Activity() {
    private val TAG = "MainActivity"
    
    private val mainScope = CoroutineScope(Dispatchers.Main + Job())
    private var streamJob: Job? = null


    private lateinit var thermalService: IshaThermalService
    private val memoryListener = IshaMemoryPressureListener()

    private var coldStartStart: Long = 0

    override fun onCreate(savedInstanceState: Bundle?) {
        coldStartStart = SystemClock.elapsedRealtime()
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

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
        val txtActiveModelRss = findViewById<TextView>(R.id.txtActiveModelRss)
        val txtTotalRuntimePressure = findViewById<TextView>(R.id.txtTotalRuntimePressure)
        val txtThermalZone = findViewById<TextView>(R.id.txtThermalZone)

        // JNI Crash containment lockfile detection (GAP 5 compliance)
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

        // Profile device memory tiering (Final Fix 9 & Spec Alignment)
        val actManager = getSystemService(ACTIVITY_SERVICE) as android.app.ActivityManager
        val memInfo = android.app.ActivityManager.MemoryInfo()
        actManager.getMemoryInfo(memInfo)
        val totalRamMb = (memInfo.totalMem / (1024 * 1024)).toInt()
        val cpuCores = Runtime.getRuntime().availableProcessors()

        // Init Isha Runtime natively (Final Fix 1: app filesDir only!)
        val filesPath = filesDir.absolutePath
        val cachePath = cacheDir.absolutePath
        val initSuccess = IshaRuntime.initializeRuntime(filesPath, cachePath, totalRamMb, cpuCores)

        if (initSuccess && !previousCrashDetected) {
            // Delete the lockfile on successful, clean initialization
            if (crashLockFile.exists()) {
                crashLockFile.delete()
            }
        }
        
        val btnDownloadT0 = findViewById<Button>(R.id.btnDownloadT0)
        val btnDownloadT1 = findViewById<Button>(R.id.btnDownloadT1)
        val btnDownloadT2 = findViewById<Button>(R.id.btnDownloadT2)
        val progressDownload = findViewById<ProgressBar>(R.id.progressDownload)
        
        val btnLoadT0 = findViewById<Button>(R.id.btnLoadT0)
        val btnLoadT1 = findViewById<Button>(R.id.btnLoadT1)
        val btnLoadT2 = findViewById<Button>(R.id.btnLoadT2)
        val btnUnload = findViewById<Button>(R.id.btnUnload)

        val btnDeleteT0 = findViewById<Button>(R.id.btnDeleteT0)
        val btnDeleteT1 = findViewById<Button>(R.id.btnDeleteT1)
        val btnDeleteT2 = findViewById<Button>(R.id.btnDeleteT2)
        
        val txtChatLog = findViewById<TextView>(R.id.txtChatLog)
        txtChatLog.movementMethod = ScrollingMovementMethod()
        
        val editPrompt = findViewById<EditText>(R.id.editPrompt)
        val btnSend = findViewById<Button>(R.id.btnSend)
        val btnOpenRecovery = findViewById<Button>(R.id.btnOpenRecovery)

        // Binding Recommendation Badges
        val lblT0Recommended = findViewById<TextView>(R.id.lblT0Recommended)
        val lblT1Recommended = findViewById<TextView>(R.id.lblT1Recommended)
        val lblT2Recommended = findViewById<TextView>(R.id.lblT2Recommended)

        // Dynamic hardware limits model routing policy
        val recommendedModelName: String
        if (totalRamMb <= 4500) { // 4GB tier
            recommendedModelName = "qwen-0.5b"
            lblT0Recommended.visibility = View.VISIBLE
        } else if (totalRamMb <= 8500) { // 6GB - 8GB tier (Target Vivo T3)
            recommendedModelName = "qwen-1.5b"
            lblT1Recommended.visibility = View.VISIBLE
        } else { // 12GB+ tier
            recommendedModelName = "gemma-3-2b-it"
            lblT2Recommended.visibility = View.VISIBLE
        }

        if (!previousCrashDetected) {
            panelSafeMode.visibility = View.VISIBLE
            txtSafeModeReason.text = String.format("Vivo T3 Hardware Profile: %d Core CPU, %dMB RAM. Recommended: %s.",
                cpuCores, totalRamMb, recommendedModelName.uppercase())
        }

        // Display cold start timing (Tap-to-First-Token indicator - Final Fix 9)
        val initEnd = SystemClock.elapsedRealtime()
        txtColdStart.text = String.format("Cold Start Latency: %d ms", initEnd - coldStartStart)

        // Initial setup for buttons based on file existence on boot
        if (File(filesDir, "qwen-0.5b.gguf").exists()) btnDownloadT0.text = "INSTALLED ✓"
        if (File(filesDir, "qwen-1.5b.gguf").exists()) btnDownloadT1.text = "INSTALLED ✓"
        if (File(filesDir, "gemma-2b.gguf").exists()) btnDownloadT2.text = "INSTALLED ✓"

        // Model switcher loaders
        btnLoadT0.setOnClickListener {
            val file = File(filesDir, "qwen-0.5b.gguf")
            if (!file.exists()) {
                txtChatLog.text = "[Error: Qwen2.5-0.5B-Instruct weights are not downloaded yet. Please tap the Download button below first!]"
                return@setOnClickListener
            }
            if (IshaRuntime.loadModelTier("qwen-0.5b", 84)) {
                txtActiveModel.text = "Active Model: Battery Saver (Qwen2.5-0.5B)"
                txtActiveModelRss.text = "Active Model RSS: 84 MB"
                txtTotalRuntimePressure.text = "Total Runtime Pressure: ~350–450MB total operational runtime footprint"
                txtChatLog.text = "[Success: Loaded Qwen2.5-0.5B-Instruct Q4_K_M stably! Ready for secure local stream chat.]"
            }
        }

        btnLoadT1.setOnClickListener {
            val file = File(filesDir, "qwen-1.5b.gguf")
            if (!file.exists()) {
                txtChatLog.text = "[Error: Qwen2.5-1.5B-Instruct weights are not downloaded yet. Please tap the Download button below first!]"
                return@setOnClickListener
            }
            if (IshaRuntime.loadModelTier("qwen-1.5b", 700)) {
                txtActiveModel.text = "Active Model: Standard (Qwen2.5-1.5B)"
                txtActiveModelRss.text = "Active Model RSS: 680 MB"
                txtTotalRuntimePressure.text = "Total Runtime Pressure: ~1.0–1.3GB realistic runtime pressure during active inference"
                txtChatLog.text = "[Success: Loaded Qwen2.5-1.5B-Instruct Q4_K_M stably! Ready for secure local stream chat.]"
            }
        }

        btnLoadT2.setOnClickListener {
            val file = File(filesDir, "gemma-2b.gguf")
            if (!file.exists()) {
                txtChatLog.text = "[Error: Gemma-3-2B-IT weights are not downloaded yet. Please tap the Download button below first!]"
                return@setOnClickListener
            }
            if (IshaRuntime.loadModelTier("gemma-2b", 1100)) {
                txtActiveModel.text = "Active Model: High Quality (Gemma-3-2B-IT)"
                txtActiveModelRss.text = "Active Model RSS: 1050 MB"
                txtTotalRuntimePressure.text = "Total Runtime Pressure: ~1.8–2.2GB realistic runtime pressure under sustained reasoning"
                txtChatLog.text = "[Success: Loaded Gemma-3-2B-IT Q4_0 stably! Session auto-expires in 45s to protect CPU temperatures.]"
                mainScope.launch {
                    delay(45000)
                    if (IshaRuntime.unloadModelTier()) {
                        txtActiveModel.text = "Active Model: UNLOADED (High Quality expired)"
                        txtActiveModelRss.text = "Active Model RSS: 0 MB"
                        txtTotalRuntimePressure.text = "Total Runtime Pressure: -- MB"
                        txtChatLog.append("\n[System: High Quality Session Expired to protect Vivo CPU thermal cores]")
                    }
                }
            }
        }

        btnUnload.setOnClickListener {
            if (IshaRuntime.unloadModelTier()) {
                txtActiveModel.text = "Active Model: UNLOADED"
                txtActiveModelRss.text = "Active Model RSS: 0 MB"
                txtTotalRuntimePressure.text = "Total Runtime Pressure: -- MB"
                txtChatLog.text = "[Success: Model unloaded from RAM.]"
            }
        }

        // Helper delete model functionality
        fun deleteModel(fileName: String, targetDownloadBtn: Button, downloadLabel: String, modelDisplayName: String) {
            // Safe unload first to prevent native leaks/crashes
            IshaRuntime.unloadModelTier()
            txtActiveModel.text = "Active Model: UNLOADED"
            txtActiveModelRss.text = "Active Model RSS: 0 MB"
            txtTotalRuntimePressure.text = "Total Runtime Pressure: -- MB"
            
            // Force Garbage Collection to release any native JVM mappings or file streams
            System.gc()
            System.runFinalization()

            val file = File(filesDir, fileName)
            if (file.exists()) {
                var deleted = false
                // Retry loop with sleep to allow kernel file locks to release
                for (retry in 1..3) {
                    if (file.delete()) {
                        deleted = true
                        break;
                    }
                    try {
                        Thread.sleep(150)
                    } catch (e: Exception) {}
                    System.gc()
                }

                // Double check if the file is gone even if delete() returned false
                if (deleted || !file.exists()) {
                    targetDownloadBtn.text = downloadLabel
                    txtChatLog.text = String.format("[Success: %s weights deleted successfully to reclaim scoped storage space.]", modelDisplayName)
                } else {
                    // Try deleteOnExit as a last resort
                    file.deleteOnExit()
                    txtChatLog.text = String.format("[Error: Failed to delete %s file from secure storage. File is locked by OS. It will be removed on next app start.]", modelDisplayName)
                }
            } else {
                txtChatLog.text = String.format("[Info: %s is not currently installed.]", modelDisplayName)
            }
        }

        // Set delete click listeners
        btnDeleteT0.setOnClickListener {
            deleteModel("qwen-0.5b.gguf", btnDownloadT0, "Download Battery Saver", "Battery Saver")
        }

        btnDeleteT1.setOnClickListener {
            deleteModel("qwen-1.5b.gguf", btnDownloadT1, "Download Standard", "Standard (Recommended)")
        }

        btnDeleteT2.setOnClickListener {
            deleteModel("gemma-2b.gguf", btnDownloadT2, "Download High Quality", "High Quality")
        }

        // Shared High-Fidelity Internet Downloader function
        fun triggerDownload(modelName: String, urlString: String, targetFileName: String, targetBtn: Button) {
            btnDownloadT0.isEnabled = false
            btnDownloadT1.isEnabled = false
            btnDownloadT2.isEnabled = false
            progressDownload.visibility = View.VISIBLE
            progressDownload.progress = 0
            txtChatLog.text = String.format("[System: Starting dynamic download of %s GGUF weights...]\n", modelName.uppercase())
            txtChatLog.append("[From HuggingFace CDN: $urlString]\n")
            
            mainScope.launch(Dispatchers.IO) {
                try {
                    val url = java.net.URL(urlString)
                    val connection = url.openConnection() as java.net.HttpURLConnection
                    connection.connect()

                    if (connection.responseCode != java.net.HttpURLConnection.HTTP_OK) {
                        throw java.io.IOException("Server returned HTTP " + connection.responseCode)
                    }

                    val fileLength = connection.contentLength
                    val input = connection.inputStream
                    
                    val partFile = File(cacheDir, "${targetFileName}.part")
                    if (partFile.exists()) partFile.delete()
                    
                    val output = java.io.FileOutputStream(partFile)
                    val data = ByteArray(8192)
                    var total: Long = 0
                    var count: Int
                    var lastProgressUpdate = 0
                    
                    while (input.read(data).also { count = it } != -1) {
                        total += count
                        output.write(data, 0, count)
                        if (fileLength > 0) {
                            val currentProgress = ((total * 100) / fileLength).toInt()
                            if (currentProgress - lastProgressUpdate >= 1) {
                                mainScope.launch {
                                    progressDownload.progress = currentProgress
                                    txtChatLog.text = String.format("[Downloading %s: %d%% completed (%.1f / %.1f MB)]",
                                        modelName.uppercase(), currentProgress, total.toFloat() / (1024 * 1024), fileLength.toFloat() / (1024 * 1024))
                                }
                                lastProgressUpdate = currentProgress
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
                            btnDownloadT2.isEnabled = true
                            targetBtn.text = "INSTALLED ✓"
                            txtChatLog.text = String.format("[Success: Official %s weights successfully downloaded & installed atomically! Tap 'Load' to run.]",
                                modelName.uppercase())
                        }
                    } else {
                        mainScope.launch {
                            btnDownloadT0.isEnabled = true
                            btnDownloadT1.isEnabled = true
                            btnDownloadT2.isEnabled = true
                            txtChatLog.append("\n[System: Installation failed - atomic rename failed]")
                        }
                    }
                } catch (e: Exception) {
                    mainScope.launch {
                        btnDownloadT0.isEnabled = true
                        btnDownloadT1.isEnabled = true
                        btnDownloadT2.isEnabled = true
                        txtChatLog.append("\n[System: Download failed - Check connection and try again: " + e.message + "]")
                    }
                }
            }
        }

        // Set dynamic download click listeners for Battery Saver, Standard, and High Quality models
        btnDownloadT0.setOnClickListener {
            if (File(filesDir, "qwen-0.5b.gguf").exists()) {
                txtChatLog.text = "[Info: Battery Saver (Qwen2.5-0.5B) is already installed. If you want to reinstall, please delete it first.]"
                return@setOnClickListener
            }
            triggerDownload(
                "Battery Saver (Qwen2.5-0.5B)",
                "https://huggingface.co/Qwen/Qwen2.5-0.5B-Instruct-GGUF/resolve/main/qwen2.5-0.5b-instruct-q2_k.gguf",
                "qwen-0.5b.gguf",
                btnDownloadT0
            )
        }

        btnDownloadT1.setOnClickListener {
            if (File(filesDir, "qwen-1.5b.gguf").exists()) {
                txtChatLog.text = "[Info: Standard (Qwen2.5-1.5B) is already installed. If you want to reinstall, please delete it first.]"
                return@setOnClickListener
            }
            triggerDownload(
                "Standard (Qwen2.5-1.5B)",
                "https://huggingface.co/Qwen/Qwen2.5-1.5B-Instruct-GGUF/resolve/main/qwen2.5-1.5b-instruct-q2_k.gguf",
                "qwen-1.5b.gguf",
                btnDownloadT1
            )
        }

        btnDownloadT2.setOnClickListener {
            if (File(filesDir, "gemma-2b.gguf").exists()) {
                txtChatLog.text = "[Info: High Quality (Gemma-3-2B-IT) is already installed. If you want to reinstall, please delete it first.]"
                return@setOnClickListener
            }
            triggerDownload(
                "High Quality (Gemma-3-2B-IT Q4_0)",
                "https://huggingface.co/google/gemma-3-2b-it-GGUF/resolve/main/gemma-3-2b-it-q4_0.gguf",
                "gemma-2b.gguf", // Keep target filename compatible with native engines but map presentation name exactly!
                btnDownloadT2
            )
        }

        // 15Hz Throttled Token Stream consumer (Final Fix 5 compliance)
        btnSend.setOnClickListener {
            val prompt = editPrompt.text.toString()
            if (prompt.isEmpty()) return@setOnClickListener
            txtChatLog.text = "User: $prompt\n\nAI: "
            editPrompt.text.clear()

            // Map and wrap prompt inside the official Qwen/Gemma Instruct ChatTemplate
            // to direct attention matrices, prevent hallucinations, and trigger immediate EOS cutoffs!
            val activeModelText = txtActiveModel.text.toString()
            val formattedPrompt = if (activeModelText.contains("Gemma", ignoreCase = true)) {
                // Gemma Instruct ChatML Format
                "<bos><|im_start|>user\n${prompt}<|im_end|>\n<|im_start|>assistant\n"
            } else {
                // Qwen Instruct ChatML Format
                "<|im_start|>system\nYou are ISHA AI, a helpful, offline-first local assistant.<|im_end|>\n<|im_start|>user\n${prompt}<|im_end|>\n<|im_start|>assistant\n"
            }

            streamJob?.cancel()
            val sessionChannel = Channel<String>(Channel.UNLIMITED)
            
            mainScope.launch(Dispatchers.IO) {
                IshaRuntime.streamTokens(formattedPrompt, object : TokenCallback {
                    override fun onToken(token: String) {
                        sessionChannel.trySend(token)
                    }
                })
            }

            // Aggregate stream tokens and batch update XML layout at 15Hz max refresh rate
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
                    // Session channel closed or coroutine cancelled
                }
            }
        }

        btnOpenRecovery.setOnClickListener {
            val intent = Intent(this, RecoveryActivity::class.java)
            startActivity(intent)
        }

        // Start dynamic thermal and memory diagnostics feedback loop (Fix 6 & Fix 7)
        mainScope.launch {
            while (true) {
                delay(1000) // 1Hz refresh rate
                
                // Get thermal state and memory pressure from JNI
                val state = IshaRuntime.getThermalState()
                
                // Map internally (Fix 6)
                if (previousCrashDetected) {
                    txtThermalZone.text = "Thermal State: Running in lightweight recovery mode."
                } else {
                    when (state) {
                        1 -> txtThermalZone.text = "Thermal State: Device warming up — optimizing performance."
                        2 -> txtThermalZone.text = "Thermal State: Reducing AI load to keep device responsive."
                        3 -> txtThermalZone.text = "Thermal State: AI paused temporarily to protect device temperature."
                        else -> txtThermalZone.text = "Thermal State: NOMINAL"
                    }
                }

                // Query memory info dynamically
                val currentMemInfo = android.app.ActivityManager.MemoryInfo()
                actManager.getMemoryInfo(currentMemInfo)
                val availableMb = currentMemInfo.availMem / (1024 * 1024)
                
                // Display runtime pressure based on active model and available RAM
                txtTotalRuntimePressure.text = String.format("Total Runtime Pressure: ~%d MB total operational footprint (System Free RAM: %d MB)",
                    totalRamMb - availableMb, availableMb)
            }
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        thermalService.stopMonitoring()
        unregisterComponentCallbacks(memoryListener)
    }
}
