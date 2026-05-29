package com.isha.ai

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.os.BatteryManager
import android.os.Build
import android.os.PowerManager
import android.util.Log

class IshaThermalService(private val context: Context) {
    private val TAG = "IshaThermalService"
    private val powerManager = context.getSystemService(Context.POWER_SERVICE) as PowerManager
    private var thermalListener: Any? = null // Using Any? to allow clean compiler checks

    fun startMonitoring() {
        // 1. Authoritative Trigger: PowerManager Thermal Status Listener (Final Fix 7 compliance)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            val listener = PowerManager.OnThermalStatusChangedListener { status ->
                Log.i(TAG, "OS PowerManager thermal status changed: $status")
                IshaRuntime.updateThermalState(status)
            }
            thermalListener = listener
            try {
                val addMethod = PowerManager::class.java.getMethod(
                    "addOnThermalStatusChangedListener",
                    java.util.concurrent.Executor::class.java,
                    PowerManager.OnThermalStatusChangedListener::class.java
                )
                val executorMethod = Context::class.java.getMethod("getMainExecutor")
                val mainExecutor = executorMethod.invoke(context)
                addMethod.invoke(powerManager, mainExecutor, listener)

                val currentStatusMethod = PowerManager::class.java.getMethod("getCurrentThermalStatus")
                val currentStatus = currentStatusMethod.invoke(powerManager) as Int
                IshaRuntime.updateThermalState(currentStatus)
            } catch (e: Exception) {
                Log.e(TAG, "Reflection fallback for thermal status listener failed: ${e.message}")
            }
        } else {
            Log.w(TAG, "PowerManager OnThermalStatusChangedListener not supported below API 29.")
        }

        // 2. Secondary Heuristic: Battery temperature monitor as fallback
        val batteryFilter = IntentFilter(Intent.ACTION_BATTERY_CHANGED)
        context.registerReceiver(batteryReceiver, batteryFilter)
    }

    fun stopMonitoring() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q && thermalListener != null) {
            try {
                val removeMethod = PowerManager::class.java.getMethod(
                    "removeOnThermalStatusChangedListener",
                    PowerManager.OnThermalStatusChangedListener::class.java
                )
                removeMethod.invoke(powerManager, thermalListener)
            } catch (e: Exception) {
                Log.e(TAG, "Reflection fallback for thermal status removal failed: ${e.message}")
            }
        }
        try {
            context.unregisterReceiver(batteryReceiver)
        } catch (e: Exception) {
            // Safe ignore
        }
    }

    private val batteryReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            val temp = intent.getIntExtra(BatteryManager.EXTRA_TEMPERATURE, -1)
            if (temp != -1) {
                val tempCelsius = temp / 10.0
                Log.d(TAG, "Secondary Battery Temp Check: $tempCelsius°C")
            }
        }
    }
}
