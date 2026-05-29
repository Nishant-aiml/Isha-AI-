package com.isha.ai

import android.content.ComponentCallbacks2
import android.content.res.Configuration
import android.util.Log

class IshaMemoryPressureListener : ComponentCallbacks2 {
    private val TAG = "IshaMemoryPressure"

    override fun onConfigurationChanged(newConfig: Configuration) {}

    override fun onLowMemory() {
        Log.e(TAG, "CRITICAL: System low on memory. Triggering safe mode and emergency model unload.")
        IshaRuntime.enterSafeMode()
    }

    override fun onTrimMemory(level: Int) {
        Log.i(TAG, "onTrimMemory level = $level")
        if (level >= ComponentCallbacks2.TRIM_MEMORY_RUNNING_CRITICAL || 
            level == ComponentCallbacks2.TRIM_MEMORY_COMPLETE) {
            Log.w(TAG, "Severe memory pressure. Reclaiming active model capacity.")
            IshaRuntime.enterSafeMode()
        }
    }
}
