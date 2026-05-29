package com.isha.ai

import android.util.Log

interface TokenCallback {
    fun onToken(token: String)
}

object IshaRuntime {
    private const val TAG = "IshaRuntime"

    init {
        try {
            System.loadLibrary("isha_jni")
            Log.i(TAG, "libisha_jni.so loaded successfully.")
        } catch (e: UnsatisfiedLinkError) {
            Log.e(TAG, "Failed to load native libisha_jni.so: ${e.message}")
        }
    }

    // JNI Native Declarations
    external fun initializeRuntime(filesDir: String, cacheDir: String, totalRamMb: Int, cpuCores: Int): Boolean
    external fun loadModelTier(modelName: String, sizeMb: Int): Boolean
    external fun unloadModelTier(): Boolean
    external fun generateTokens(prompt: String, maxTokens: Int): String
    external fun streamTokens(prompt: String, callback: TokenCallback): Boolean
    external fun getThermalState(): Int
    external fun getMemoryPressure(): Int
    external fun enterSafeMode()
    external fun checkpointSession(sessionId: String): Boolean
    external fun restoreSession(sessionId: String): Boolean
    external fun updateThermalState(thermalState: Int)

    // JNI Local RAG Declarations
    external fun ingestChunkNative(chunkId: String, chunkText: String, packId: String, isLast: Boolean): Boolean
    external fun retrieveContextNative(query: String, packId: String, limit: Int): String
    external fun clearPackNative(packId: String): Boolean
}
