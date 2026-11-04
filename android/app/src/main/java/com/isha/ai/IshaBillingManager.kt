package com.isha.ai

import android.content.Context
import android.util.Log
import java.security.MessageDigest
import javax.crypto.Mac
import javax.crypto.spec.SecretKeySpec

/**
 * Enterprise Pay-As-You-Go Billing & Wallet Ledger.
 * Enforces compulsory payment/metering for Cloud AI APIs while guaranteeing
 * 100% Free & Unlimited access to all Offline On-Device models & generators.
 */
object IshaBillingManager {
    private const val TAG = "IshaBillingManager"
    private const val PREFS_NAME = "isha_ai_billing_ledger"
    private const val KEY_BALANCE = "ledger_balance_credits"
    private const val KEY_SIGNATURE = "ledger_hmac_signature"
    private const val KEY_TOTAL_USAGE = "ledger_total_spent"

    // Credit Cost Catalog
    const val COST_CLOUD_CHAT = 1      // ~500-1000 tokens of Cloud LLM
    const val COST_CLOUD_IMAGE = 5     // 1 High-Res Cloud Diffusion (FLUX.1 / SDXL)
    const val COST_CLOUD_DECK = 10     // 1 Generated Presentation Slide Deck
    const val COST_CLOUD_WEB = 10      // 1 Generated Responsive Website
    const val COST_CLOUD_VIDEO = 15    // 1 Video Storyboard Preview
    const val COST_OFFLINE = 0         // 100% FREE FOREVER (Zero balance deducted)

    const val INITIAL_STARTER_CREDITS = 25 // Free starter credits for every new installation

    class InsufficientBalanceException(val required: Int, val available: Int) :
        Exception("Insufficient cloud credits ($available available, $required required). Recharge or switch to Free Offline Mode.")

    private fun generateSecretKey(context: Context): ByteArray {
        val seed = "isha_ai_ledger_salt_${context.packageName}_2026"
        return MessageDigest.getInstance("SHA-256").digest(seed.toByteArray(Charsets.UTF_8))
    }

    private fun computeHmac(balance: Int, keyBytes: ByteArray): String {
        val mac = Mac.getInstance("HmacSHA256")
        mac.init(SecretKeySpec(keyBytes, "HmacSHA256"))
        val hash = mac.doFinal("balance:$balance".toByteArray(Charsets.UTF_8))
        return hash.joinToString("") { "%02x".format(it) }
    }

    /**
     * Gets current verified wallet credit balance.
     * Recovers from tampering gracefully.
     */
    fun getCreditBalance(context: Context): Int {
        val prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
        val key = generateSecretKey(context)

        if (!prefs.contains(KEY_BALANCE)) {
            // First time initialization: grant initial starter credits
            setVerifiedBalance(context, INITIAL_STARTER_CREDITS)
            return INITIAL_STARTER_CREDITS
        }

        val balance = prefs.getInt(KEY_BALANCE, 0)
        val savedSig = prefs.getString(KEY_SIGNATURE, "") ?: ""
        val computedSig = computeHmac(balance, key)

        return if (savedSig == computedSig) {
            balance
        } else {
            Log.w(TAG, "Ledger HMAC signature mismatch! Recovering to safe 0 balance.")
            setVerifiedBalance(context, 0)
            0
        }
    }

    private fun setVerifiedBalance(context: Context, balance: Int) {
        val key = generateSecretKey(context)
        val sig = computeHmac(balance, key)
        val prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
        prefs.edit()
            .putInt(KEY_BALANCE, balance)
            .putString(KEY_SIGNATURE, sig)
            .apply()
    }

    /**
     * Checks if the user can execute a cloud operation.
     * Bypassed if Bring-Your-Own-Key (BYOK) is active.
     */
    fun canAffordCloud(context: Context, cost: Int, hasByok: Boolean): Boolean {
        if (hasByok) return true // BYOK users pay direct to provider, Isha balance not deducted
        return getCreditBalance(context) >= cost
    }

    /**
     * Deducts credits for a cloud operation.
     * Throws InsufficientBalanceException if unaffordable and not BYOK.
     */
    @Synchronized
    fun deductCloudCredits(context: Context, cost: Int, description: String, hasByok: Boolean): Boolean {
        if (cost == 0) return true // Offline generation is 100% free

        if (hasByok) {
            Log.i(TAG, "BYOK active: 0 Isha credits deducted for $description")
            return true
        }

        val current = getCreditBalance(context)
        if (current < cost) {
            Log.w(TAG, "Payment gate rejected: $description requires $cost credits, has $current")
            throw InsufficientBalanceException(cost, current)
        }

        val newBalance = current - cost
        setVerifiedBalance(context, newBalance)

        val prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
        val spent = prefs.getInt(KEY_TOTAL_USAGE, 0) + cost
        prefs.edit().putInt(KEY_TOTAL_USAGE, spent).apply()

        Log.i(TAG, "Deducted $cost credits for $description. Remaining balance: $newBalance credits")
        return true
    }

    /**
     * Adds credits to the user wallet upon top-up or recharge.
     */
    @Synchronized
    fun rechargeWallet(context: Context, amount: Int): Int {
        val current = getCreditBalance(context)
        val newBalance = current + amount
        setVerifiedBalance(context, newBalance)
        Log.i(TAG, "Recharged wallet with $amount credits. New balance: $newBalance")
        return newBalance
    }

    fun getTotalSpent(context: Context): Int {
        val prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
        return prefs.getInt(KEY_TOTAL_USAGE, 0)
    }
}
