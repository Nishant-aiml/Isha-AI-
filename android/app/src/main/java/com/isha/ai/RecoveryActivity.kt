package com.isha.ai

import android.app.Activity
import android.content.Intent
import android.os.Bundle
import android.widget.Button
import android.widget.TextView

class RecoveryActivity : Activity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_recovery)

        val txtLog = findViewById<TextView>(R.id.txtRecoveryLog)
        val btnWipeCache = findViewById<Button>(R.id.btnWipeCache)
        val btnDeleteModels = findViewById<Button>(R.id.btnDeleteModels)
        val btnVerifyStorage = findViewById<Button>(R.id.btnVerifyStorage)
        val btnExit = findViewById<Button>(R.id.btnExitRecovery)

        btnWipeCache.setOnClickListener {
            try {
                val cacheDir = cacheDir
                val files = cacheDir.listFiles()
                var count = 0
                files?.forEach {
                    if (it.deleteRecursively()) count++
                }
                txtLog.text = "Cache Purge Complete.\nSuccessfully deleted $count temporary files."
            } catch (e: Exception) {
                txtLog.text = "Error clearing cache: ${e.message}"
            }
        }

        btnDeleteModels.setOnClickListener {
            try {
                val filesDir = filesDir
                val files = filesDir.listFiles()
                var count = 0
                files?.forEach {
                    if (it.name.endsWith(".gguf") || it.name.endsWith(".part")) {
                        if (it.delete()) count++
                    }
                }
                txtLog.text = "GGUF Clean Complete.\nDeleted $count GGUF weights from internal filesDir."
            } catch (e: Exception) {
                txtLog.text = "Error deleting models: ${e.message}"
            }
        }

        btnVerifyStorage.setOnClickListener {
            try {
                val filesDir = filesDir
                val freeSpace = filesDir.freeSpace
                val freeSpaceGb = freeSpace / (1024.0 * 1024.0 * 1024.0)
                txtLog.text = String.format("Verified Storage Space:\nFree Space = %.3f GB\nPath = %s", freeSpaceGb, filesDir.absolutePath)
            } catch (e: Exception) {
                txtLog.text = "Error querying storage: ${e.message}"
            }
        }

        btnExit.setOnClickListener {
            val intent = Intent(this, MainActivity::class.java)
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TASK)
            startActivity(intent)
            android.os.Process.killProcess(android.os.Process.myPid())
        }
    }
}
