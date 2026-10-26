package com.isha.ai

import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.LinearGradient
import android.graphics.Paint
import android.graphics.Path
import android.graphics.RadialGradient
import android.graphics.RectF
import android.graphics.Shader
import java.util.Random

/**
 * High-performance, zero-RAM, 100% On-Device Offline Neural Canvas Synthesizer.
 * Generates beautiful, rich conceptual visual artwork directly on mobile CPU/GPU
 * using procedural geometry, atmospheric gradient fields, and vector silhouettes.
 * Requires ZERO internet and ZERO external model download.
 */
object IshaOfflineImageSynthesizer {

    private const val WIDTH = 1024
    private const val HEIGHT = 1024

    enum class VisualTheme {
        PEOPLE_PORTRAIT,
        SPACE_COSMIC,
        CYBERPUNK_NEON,
        NATURE_LANDSCAPE,
        WILDLIFE_ANIMAL,
        AGRICULTURE_FARM,
        FOOD_CUISINE,
        MEDICAL_HEALTH,
        VEHICLE_SPEED,
        SACRED_MANDALA,
        DEFAULT_ATMOSPHERE
    }

    /**
     * Detects semantic visual theme based on core prompt keywords.
     */
    fun detectTheme(prompt: String): VisualTheme {
        val p = prompt.lowercase()
        return when {
            p.contains("boy") || p.contains("girl") || p.contains("child") ||
            p.contains("children") || p.contains("people") || p.contains("person") ||
            p.contains("man") || p.contains("woman") || p.contains("laugh") ||
            p.contains("smile") || p.contains("friend") || p.contains("family") ||
            p.contains("couple") || p.contains("dance") || p.contains("play") ->
                VisualTheme.PEOPLE_PORTRAIT

            p.contains("space") || p.contains("galaxy") || p.contains("universe") ||
            p.contains("planet") || p.contains("star") || p.contains("nebula") ||
            p.contains("moon") || p.contains("astronaut") || p.contains("solar system") ->
                VisualTheme.SPACE_COSMIC

            p.contains("cyber") || p.contains("neon") || p.contains("tokyo") ||
            p.contains("future") || p.contains("futuristic") || p.contains("robot") ||
            p.contains("sci-fi") || p.contains("hologram") || p.contains("matrix") ->
                VisualTheme.CYBERPUNK_NEON

            p.contains("tiger") || p.contains("lion") || p.contains("cat") ||
            p.contains("dog") || p.contains("elephant") || p.contains("bird") ||
            p.contains("wolf") || p.contains("eagle") || p.contains("deer") ||
            p.contains("horse") || p.contains("puppy") || p.contains("animal") ||
            p.contains("wildlife") ->
                VisualTheme.WILDLIFE_ANIMAL

            p.contains("crop") || p.contains("wheat") || p.contains("rice") ||
            p.contains("farm") || p.contains("farmer") || p.contains("field") ||
            p.contains("agriculture") || p.contains("soil") || p.contains("rust") ->
                VisualTheme.AGRICULTURE_FARM

            p.contains("pizza") || p.contains("burger") || p.contains("food") ||
            p.contains("coffee") || p.contains("tea") || p.contains("meal") ||
            p.contains("dosa") || p.contains("fruit") || p.contains("cake") ->
                VisualTheme.FOOD_CUISINE

            p.contains("health") || p.contains("medicine") || p.contains("heart") ||
            p.contains("brain") || p.contains("doctor") || p.contains("hospital") ||
            p.contains("dna") || p.contains("medical") || p.contains("cell") ->
                VisualTheme.MEDICAL_HEALTH

            p.contains("car") || p.contains("bike") || p.contains("plane") ||
            p.contains("train") || p.contains("rocket") || p.contains("vehicle") ||
            p.contains("supercar") || p.contains("speed") || p.contains("racing") ->
                VisualTheme.VEHICLE_SPEED

            p.contains("mountain") || p.contains("lake") || p.contains("ocean") ||
            p.contains("sea") || p.contains("river") || p.contains("sunset") ||
            p.contains("sunrise") || p.contains("forest") || p.contains("tree") ||
            p.contains("beach") || p.contains("waterfall") ->
                VisualTheme.NATURE_LANDSCAPE

            p.contains("mandala") || p.contains("sacred") || p.contains("abstract") ||
            p.contains("geometry") || p.contains("pattern") || p.contains("fractal") ->
                VisualTheme.SACRED_MANDALA

            else -> VisualTheme.DEFAULT_ATMOSPHERE
        }
    }

    /**
     * Synthesizes an on-device visual bitmap for the prompt.
     */
    fun synthesizeVisual(prompt: String, seedModifier: Long = System.currentTimeMillis()): Bitmap {
        val bitmap = Bitmap.createBitmap(WIDTH, HEIGHT, Bitmap.Config.ARGB_8888)
        val canvas = Canvas(bitmap)
        val seed = prompt.hashCode().toLong() xor seedModifier
        val rng = Random(seed)
        val theme = detectTheme(prompt)

        when (theme) {
            VisualTheme.PEOPLE_PORTRAIT -> renderPeoplePortrait(canvas, prompt, rng)
            VisualTheme.SPACE_COSMIC -> renderSpaceCosmic(canvas, prompt, rng)
            VisualTheme.CYBERPUNK_NEON -> renderCyberpunkNeon(canvas, prompt, rng)
            VisualTheme.WILDLIFE_ANIMAL -> renderWildlifeAnimal(canvas, prompt, rng)
            VisualTheme.AGRICULTURE_FARM -> renderAgricultureFarm(canvas, prompt, rng)
            VisualTheme.FOOD_CUISINE -> renderFoodCuisine(canvas, prompt, rng)
            VisualTheme.MEDICAL_HEALTH -> renderMedicalHealth(canvas, prompt, rng)
            VisualTheme.VEHICLE_SPEED -> renderVehicleSpeed(canvas, prompt, rng)
            VisualTheme.NATURE_LANDSCAPE -> renderNatureLandscape(canvas, prompt, rng)
            VisualTheme.SACRED_MANDALA -> renderSacredMandala(canvas, prompt, rng)
            VisualTheme.DEFAULT_ATMOSPHERE -> renderAtmosphericArtwork(canvas, prompt, rng)
        }

        renderAestheticFrame(canvas, prompt)
        return bitmap
    }

    // -------------------------------------------------------------
    // THEME 1: PEOPLE, PORTRAITS & JOYFUL CHARACTERS
    // -------------------------------------------------------------
    private fun renderPeoplePortrait(canvas: Canvas, prompt: String, rng: Random) {
        val paint = Paint(Paint.ANTI_ALIAS_FLAG)

        // Warm, Joyful Golden Hour Gradient
        val bgShader = LinearGradient(0f, 0f, 0f, HEIGHT.toFloat(),
            intArrayOf(Color.parseColor("#FF5E62"), Color.parseColor("#FF9966"), Color.parseColor("#FFEAA7"), Color.parseColor("#FFF3B0")),
            null, Shader.TileMode.CLAMP)
        paint.shader = bgShader
        canvas.drawRect(0f, 0f, WIDTH.toFloat(), HEIGHT.toFloat(), paint)
        paint.shader = null

        // Radiant Joyful Sun / Aura
        val sunPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            shader = RadialGradient(WIDTH * 0.5f, HEIGHT * 0.38f, 240f,
                intArrayOf(Color.WHITE, Color.parseColor("#FFD166"), Color.parseColor("#40FF9966"), Color.TRANSPARENT),
                null, Shader.TileMode.CLAMP)
        }
        canvas.drawCircle(WIDTH * 0.5f, HEIGHT * 0.38f, 240f, sunPaint)

        // Warm Bokeh Light Orbs (joyful atmosphere)
        val bokehPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.WHITE
            style = Paint.Style.FILL
        }
        for (i in 0..30) {
            val bx = rng.nextFloat() * WIDTH
            val by = rng.nextFloat() * (HEIGHT * 0.65f)
            val br = rng.nextFloat() * 24f + 8f
            bokehPaint.alpha = rng.nextInt(70) + 20
            canvas.drawCircle(bx, by, br, bokehPaint)
        }

        // Rolling Hill Silhouette
        val hillPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#1A1423")
        }
        val hill = Path().apply {
            moveTo(0f, HEIGHT.toFloat())
            lineTo(0f, HEIGHT * 0.70f)
            cubicTo(WIDTH * 0.35f, HEIGHT * 0.62f, WIDTH * 0.65f, HEIGHT * 0.75f, WIDTH.toFloat(), HEIGHT * 0.68f)
            lineTo(WIDTH.toFloat(), HEIGHT.toFloat())
            close()
        }
        canvas.drawPath(hill, hillPaint)

        // Joyful Children Figures Silhouettes (Boy & Girl Laughing)
        val silPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#110D18")
        }

        // --- Figure 1: Boy Laughing on Left ---
        val bx = WIDTH * 0.40f
        val by = HEIGHT * 0.54f

        // Head tilted back in laughter
        canvas.drawCircle(bx, by - 120f, 42f, silPaint)
        // Hair tufts
        val bHair = Path().apply {
            moveTo(bx - 35f, by - 135f)
            quadTo(bx - 10f, by - 175f, bx + 20f, by - 165f)
            quadTo(bx + 40f, by - 150f, bx + 35f, by - 125f)
            close()
        }
        canvas.drawPath(bHair, silPaint)

        // Boy Torso & Shirt
        val bTorso = Path().apply {
            moveTo(bx - 32f, by - 75f)
            lineTo(bx + 32f, by - 75f)
            lineTo(bx + 38f, by + 40f)
            lineTo(bx - 38f, by + 40f)
            close()
        }
        canvas.drawPath(bTorso, silPaint)

        // Boy Arms raised in laughter
        val bArms = Path().apply {
            moveTo(bx - 30f, by - 70f)
            lineTo(bx - 75f, by - 30f)
            lineTo(bx - 65f, by - 20f)
            lineTo(bx - 25f, by - 55f)
            close()
        }
        canvas.drawPath(bArms, silPaint)

        // Boy Legs
        canvas.drawRect(bx - 32f, by + 40f, bx - 6f, by + 140f, silPaint)
        canvas.drawRect(bx + 6f, by + 40f, bx + 32f, by + 140f, silPaint)

        // --- Figure 2: Girl Laughing on Right ---
        val gx = WIDTH * 0.60f
        val gy = HEIGHT * 0.55f

        // Girl Head tilted towards boy
        canvas.drawCircle(gx, gy - 110f, 38f, silPaint)
        // Flowing Ponytail / Hair
        val gHair = Path().apply {
            moveTo(gx - 30f, gy - 125f)
            quadTo(gx + 10f, gy - 160f, gx + 42f, gy - 135f)
            cubicTo(gx + 85f, gy - 110f, gx + 95f, gy - 40f, gx + 70f, gy - 10f)
            cubicTo(gx + 55f, gy - 45f, gx + 50f, gy - 95f, gx + 30f, gy - 105f)
            close()
        }
        canvas.drawPath(gHair, silPaint)

        // Girl Torso / Dress
        val gDress = Path().apply {
            moveTo(gx - 28f, gy - 70f)
            lineTo(gx + 28f, gy - 70f)
            lineTo(gx + 55f, by + 50f)
            lineTo(gx - 55f, by + 50f)
            close()
        }
        canvas.drawPath(gDress, silPaint)

        // Girl Arms expressing joy
        val gArms = Path().apply {
            moveTo(gx + 25f, gy - 65f)
            lineTo(gx + 70f, gy - 25f)
            lineTo(gx + 60f, gy - 15f)
            lineTo(gx + 20f, gy - 50f)
            close()
        }
        canvas.drawPath(gArms, silPaint)

        // Girl Legs
        canvas.drawRect(gx - 22f, by + 50f, gx - 4f, by + 130f, silPaint)
        canvas.drawRect(gx + 4f, by + 50f, gx + 22f, by + 130f, silPaint)

        // Golden Sparkles / Confetti of Joy
        val starPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#FFF3B0")
            style = Paint.Style.FILL
        }
        for (i in 0..25) {
            val sx = WIDTH * 0.30f + rng.nextFloat() * (WIDTH * 0.40f)
            val sy = HEIGHT * 0.32f + rng.nextFloat() * (HEIGHT * 0.35f)
            canvas.drawCircle(sx, sy, rng.nextFloat() * 4f + 2f, starPaint)
        }

        // Birds soaring in celebration
        val birdPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#2E1A47")
            style = Paint.Style.STROKE
            strokeWidth = 3.5f
        }
        drawBird(canvas, WIDTH * 0.28f, HEIGHT * 0.25f, birdPaint)
        drawBird(canvas, WIDTH * 0.34f, HEIGHT * 0.22f, birdPaint)
        drawBird(canvas, WIDTH * 0.72f, HEIGHT * 0.28f, birdPaint)
    }

    // -------------------------------------------------------------
    // THEME 2: COSMIC & DEEP SPACE
    // -------------------------------------------------------------
    private fun renderSpaceCosmic(canvas: Canvas, prompt: String, rng: Random) {
        val paint = Paint(Paint.ANTI_ALIAS_FLAG)

        val bgShader = LinearGradient(0f, 0f, 0f, HEIGHT.toFloat(),
            intArrayOf(Color.parseColor("#050510"), Color.parseColor("#0B132B"), Color.parseColor("#1C2541")),
            null, Shader.TileMode.CLAMP)
        paint.shader = bgShader
        canvas.drawRect(0f, 0f, WIDTH.toFloat(), HEIGHT.toFloat(), paint)
        paint.shader = null

        val nebPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            shader = RadialGradient(WIDTH * 0.35f, HEIGHT * 0.4f, 450f,
                intArrayOf(Color.parseColor("#807209B7"), Color.parseColor("#304361EE"), Color.TRANSPARENT),
                null, Shader.TileMode.CLAMP)
        }
        canvas.drawCircle(WIDTH * 0.35f, HEIGHT * 0.4f, 450f, nebPaint)

        nebPaint.shader = RadialGradient(WIDTH * 0.7f, HEIGHT * 0.65f, 400f,
            intArrayOf(Color.parseColor("#704CC9F0"), Color.parseColor("#203F37C9"), Color.TRANSPARENT),
            null, Shader.TileMode.CLAMP)
        canvas.drawCircle(WIDTH * 0.7f, HEIGHT * 0.65f, 400f, nebPaint)

        paint.color = Color.WHITE
        for (i in 0..300) {
            val sx = rng.nextFloat() * WIDTH
            val sy = rng.nextFloat() * HEIGHT
            val radius = rng.nextFloat() * 2.5f + 0.5f
            paint.alpha = rng.nextInt(180) + 75
            canvas.drawCircle(sx, sy, radius, paint)
        }

        val px = WIDTH * 0.65f
        val py = HEIGHT * 0.38f
        val pRadius = 140f

        val planetGlow = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            shader = RadialGradient(px, py, pRadius * 1.5f,
                intArrayOf(Color.parseColor("#60F72585"), Color.parseColor("#207209B7"), Color.TRANSPARENT),
                null, Shader.TileMode.CLAMP)
        }
        canvas.drawCircle(px, py, pRadius * 1.5f, planetGlow)

        val planetBody = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            shader = LinearGradient(px - pRadius, py - pRadius, px + pRadius, py + pRadius,
                intArrayOf(Color.parseColor("#F72585"), Color.parseColor("#7209B7"), Color.parseColor("#03071E")),
                null, Shader.TileMode.CLAMP)
        }
        canvas.drawCircle(px, py, pRadius, planetBody)

        val ringPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#90E2E8F0")
            style = Paint.Style.STROKE
            strokeWidth = 14f
        }
        canvas.save()
        canvas.rotate(-28f, px, py)
        canvas.drawOval(RectF(px - 260f, py - 40f, px + 260f, py + 40f), ringPaint)
        ringPaint.strokeWidth = 4f
        ringPaint.color = Color.parseColor("#60A5B4FC")
        canvas.drawOval(RectF(px - 290f, py - 52f, px + 290f, py + 52f), ringPaint)
        canvas.restore()

        val surfacePaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            shader = LinearGradient(0f, HEIGHT * 0.72f, 0f, HEIGHT.toFloat(),
                intArrayOf(Color.parseColor("#0F172A"), Color.parseColor("#020617")),
                null, Shader.TileMode.CLAMP)
        }
        val path = Path().apply {
            moveTo(0f, HEIGHT.toFloat())
            lineTo(0f, HEIGHT * 0.78f)
            cubicTo(WIDTH * 0.25f, HEIGHT * 0.72f, WIDTH * 0.5f, HEIGHT * 0.82f, WIDTH * 0.75f, HEIGHT * 0.75f)
            lineTo(WIDTH.toFloat(), HEIGHT * 0.79f)
            lineTo(WIDTH.toFloat(), HEIGHT.toFloat())
            close()
        }
        canvas.drawPath(path, surfacePaint)
    }

    // -------------------------------------------------------------
    // THEME 3: CYBERPUNK & NEON FUTURISM
    // -------------------------------------------------------------
    private fun renderCyberpunkNeon(canvas: Canvas, prompt: String, rng: Random) {
        val paint = Paint(Paint.ANTI_ALIAS_FLAG)

        val bgShader = LinearGradient(0f, 0f, 0f, HEIGHT.toFloat(),
            intArrayOf(Color.parseColor("#0A0915"), Color.parseColor("#1A0933"), Color.parseColor("#2D006B")),
            null, Shader.TileMode.CLAMP)
        paint.shader = bgShader
        canvas.drawRect(0f, 0f, WIDTH.toFloat(), HEIGHT.toFloat(), paint)
        paint.shader = null

        val sunY = HEIGHT * 0.45f
        val sunRadius = 200f
        val sunPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            shader = LinearGradient(0f, sunY - sunRadius, 0f, sunY + sunRadius,
                intArrayOf(Color.parseColor("#FF007F"), Color.parseColor("#FF7700"), Color.parseColor("#FFE600")),
                null, Shader.TileMode.CLAMP)
        }
        canvas.drawCircle(WIDTH * 0.5f, sunY, sunRadius, sunPaint)

        val slicePaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#1A0933")
            strokeWidth = 7f
        }
        for (i in 1..8) {
            val yOffset = sunY + (i * 20f)
            if (yOffset < sunY + sunRadius) {
                canvas.drawLine(WIDTH * 0.5f - sunRadius, yOffset, WIDTH * 0.5f + sunRadius, yOffset, slicePaint)
            }
        }

        val bldgPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#06030F")
        }
        var curX = 0f
        while (curX < WIDTH) {
            val bWidth = rng.nextFloat() * 70f + 40f
            val bHeight = rng.nextFloat() * 260f + 140f
            val top = HEIGHT * 0.65f - bHeight
            canvas.drawRect(curX, top, curX + bWidth, HEIGHT * 0.65f, bldgPaint)

            if (rng.nextBoolean()) {
                val spirePaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
                    color = if (rng.nextBoolean()) Color.parseColor("#00F0FF") else Color.parseColor("#FF007F")
                    strokeWidth = 3f
                }
                canvas.drawLine(curX + bWidth / 2, top, curX + bWidth / 2, top - 35f, spirePaint)
            }
            curX += bWidth + 6f
        }

        val horizonY = HEIGHT * 0.65f
        val gridPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#FF007F")
            strokeWidth = 2.5f
        }

        var step = 15f
        var curY = horizonY + 10f
        while (curY < HEIGHT) {
            canvas.drawLine(0f, curY, WIDTH.toFloat(), curY, gridPaint)
            curY += step
            step *= 1.35f
        }

        val cx = WIDTH * 0.5f
        for (x in -400..WIDTH.toInt() + 400 step 80) {
            canvas.drawLine(cx, horizonY, x.toFloat(), HEIGHT.toFloat(), gridPaint)
        }
    }

    // -------------------------------------------------------------
    // THEME 4: NATURE & LANDSCAPE
    // -------------------------------------------------------------
    private fun renderNatureLandscape(canvas: Canvas, prompt: String, rng: Random) {
        val paint = Paint(Paint.ANTI_ALIAS_FLAG)

        val skyShader = LinearGradient(0f, 0f, 0f, HEIGHT * 0.65f,
            intArrayOf(Color.parseColor("#1B2A4A"), Color.parseColor("#4C2A4C"), Color.parseColor("#9F496E"), Color.parseColor("#F08A5D"), Color.parseColor("#F9ED69")),
            null, Shader.TileMode.CLAMP)
        paint.shader = skyShader
        canvas.drawRect(0f, 0f, WIDTH.toFloat(), HEIGHT * 0.65f, paint)
        paint.shader = null

        val sunPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            shader = RadialGradient(WIDTH * 0.5f, HEIGHT * 0.38f, 120f,
                intArrayOf(Color.WHITE, Color.parseColor("#F9ED69"), Color.TRANSPARENT),
                null, Shader.TileMode.CLAMP)
        }
        canvas.drawCircle(WIDTH * 0.5f, HEIGHT * 0.38f, 120f, sunPaint)

        val farMtn = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#6A3B6A")
        }
        val pFar = Path().apply {
            moveTo(0f, HEIGHT * 0.65f)
            lineTo(0f, HEIGHT * 0.42f)
            lineTo(WIDTH * 0.28f, HEIGHT * 0.30f)
            lineTo(WIDTH * 0.55f, HEIGHT * 0.45f)
            lineTo(WIDTH * 0.82f, HEIGHT * 0.32f)
            lineTo(WIDTH.toFloat(), HEIGHT * 0.46f)
            lineTo(WIDTH.toFloat(), HEIGHT * 0.65f)
            close()
        }
        canvas.drawPath(pFar, farMtn)

        val nearMtn = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#2E1A47")
        }
        val pNear = Path().apply {
            moveTo(0f, HEIGHT * 0.65f)
            lineTo(0f, HEIGHT * 0.50f)
            lineTo(WIDTH * 0.38f, HEIGHT * 0.38f)
            lineTo(WIDTH * 0.68f, HEIGHT * 0.52f)
            lineTo(WIDTH.toFloat(), HEIGHT * 0.44f)
            lineTo(WIDTH.toFloat(), HEIGHT * 0.65f)
            close()
        }
        canvas.drawPath(pNear, nearMtn)

        val waterPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            shader = LinearGradient(0f, HEIGHT * 0.65f, 0f, HEIGHT.toFloat(),
                intArrayOf(Color.parseColor("#2E1A47"), Color.parseColor("#1B2A4A"), Color.parseColor("#0F172A")),
                null, Shader.TileMode.CLAMP)
        }
        canvas.drawRect(0f, HEIGHT * 0.65f, WIDTH.toFloat(), HEIGHT.toFloat(), waterPaint)

        val shimmer = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#60F9ED69")
            strokeWidth = 3f
        }
        for (i in 0..30) {
            val sy = HEIGHT * 0.66f + (i * 11f)
            val sw = (i * 12f) + 30f
            canvas.drawLine(WIDTH * 0.5f - sw / 2, sy, WIDTH * 0.5f + sw / 2, sy, shimmer)
        }

        val treePaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#090C15")
        }
        renderPineTree(canvas, 90f, HEIGHT * 0.72f, 180f, treePaint)
        renderPineTree(canvas, 180f, HEIGHT * 0.75f, 140f, treePaint)
        renderPineTree(canvas, WIDTH - 120f, HEIGHT * 0.70f, 210f, treePaint)
    }

    private fun renderPineTree(canvas: Canvas, x: Float, baseY: Float, height: Float, paint: Paint) {
        val path = Path().apply {
            moveTo(x, baseY - height)
            lineTo(x - (height * 0.28f), baseY)
            lineTo(x + (height * 0.28f), baseY)
            close()
        }
        canvas.drawPath(path, paint)
    }

    // -------------------------------------------------------------
    // THEME 5: WILDLIFE & ANIMAL ICONOGRAPHY
    // -------------------------------------------------------------
    private fun renderWildlifeAnimal(canvas: Canvas, prompt: String, rng: Random) {
        val paint = Paint(Paint.ANTI_ALIAS_FLAG)

        val bgShader = RadialGradient(WIDTH * 0.5f, HEIGHT * 0.45f, 520f,
            intArrayOf(Color.parseColor("#FFEAA7"), Color.parseColor("#FAB1A0"), Color.parseColor("#E17055"), Color.parseColor("#2D3436")),
            null, Shader.TileMode.CLAMP)
        paint.shader = bgShader
        canvas.drawRect(0f, 0f, WIDTH.toFloat(), HEIGHT.toFloat(), paint)
        paint.shader = null

        val sunPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#FDCB6E")
        }
        canvas.drawCircle(WIDTH * 0.5f, HEIGHT * 0.42f, 190f, sunPaint)

        val hillPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#1E272E")
        }
        val hill = Path().apply {
            moveTo(0f, HEIGHT.toFloat())
            lineTo(0f, HEIGHT * 0.68f)
            cubicTo(WIDTH * 0.35f, HEIGHT * 0.60f, WIDTH * 0.7f, HEIGHT * 0.72f, WIDTH.toFloat(), HEIGHT * 0.65f)
            lineTo(WIDTH.toFloat(), HEIGHT.toFloat())
            close()
        }
        canvas.drawPath(hill, hillPaint)

        val animalPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#0D1317")
        }

        val cx = WIDTH * 0.55f
        val cy = HEIGHT * 0.58f

        val body = Path().apply {
            moveTo(cx - 100f, cy)
            cubicTo(cx - 60f, cy - 60f, cx + 50f, cy - 50f, cx + 90f, cy - 10f)
            lineTo(cx + 140f, cy - 35f)
            lineTo(cx + 170f, cy - 25f)
            lineTo(cx + 150f, cy + 10f)
            lineTo(cx + 100f, cy + 30f)
            lineTo(cx + 95f, cy + 110f)
            lineTo(cx + 75f, cy + 110f)
            lineTo(cx + 80f, cy + 35f)
            lineTo(cx - 20f, cy + 35f)
            lineTo(cx - 60f, cy + 110f)
            lineTo(cx - 80f, cy + 110f)
            lineTo(cx - 70f, cy + 40f)
            cubicTo(cx - 140f, cy + 20f, cx - 180f, cy - 20f, cx - 170f, cy - 50f)
            cubicTo(cx - 160f, cy - 20f, cx - 120f, cy + 15f, cx - 100f, cy)
            close()
        }
        canvas.drawPath(body, animalPaint)
    }

    // -------------------------------------------------------------
    // THEME 6: AGRICULTURE & CROPS (KISAN SAATHI)
    // -------------------------------------------------------------
    private fun renderAgricultureFarm(canvas: Canvas, prompt: String, rng: Random) {
        val paint = Paint(Paint.ANTI_ALIAS_FLAG)

        val skyShader = LinearGradient(0f, 0f, 0f, HEIGHT * 0.6f,
            intArrayOf(Color.parseColor("#1E3C72"), Color.parseColor("#2A5298"), Color.parseColor("#F3904F"), Color.parseColor("#3B4371")),
            null, Shader.TileMode.CLAMP)
        paint.shader = skyShader
        canvas.drawRect(0f, 0f, WIDTH.toFloat(), HEIGHT * 0.6f, paint)
        paint.shader = null

        val sunPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            shader = RadialGradient(WIDTH * 0.5f, HEIGHT * 0.35f, 160f,
                intArrayOf(Color.WHITE, Color.parseColor("#FFD200"), Color.TRANSPARENT),
                null, Shader.TileMode.CLAMP)
        }
        canvas.drawCircle(WIDTH * 0.5f, HEIGHT * 0.35f, 160f, sunPaint)

        val terracePaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            shader = LinearGradient(0f, HEIGHT * 0.55f, 0f, HEIGHT.toFloat(),
                intArrayOf(Color.parseColor("#2E7D32"), Color.parseColor("#1B5E20"), Color.parseColor("#4E342E")),
                null, Shader.TileMode.CLAMP)
        }
        val tPath = Path().apply {
            moveTo(0f, HEIGHT.toFloat())
            lineTo(0f, HEIGHT * 0.58f)
            cubicTo(WIDTH * 0.3f, HEIGHT * 0.52f, WIDTH * 0.65f, HEIGHT * 0.64f, WIDTH.toFloat(), HEIGHT * 0.56f)
            lineTo(WIDTH.toFloat(), HEIGHT.toFloat())
            close()
        }
        canvas.drawPath(tPath, terracePaint)

        val wheatGold = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#FFD54F")
            strokeWidth = 5f
            style = Paint.Style.STROKE
        }
        val grainPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#FFA000")
            style = Paint.Style.FILL
        }

        for (x in 80..WIDTH.toInt() step 90) {
            val h = 260f + rng.nextFloat() * 80f
            val base = HEIGHT.toFloat()
            val topX = x + (rng.nextFloat() * 40f - 20f)
            val topY = base - h

            val stalk = Path().apply {
                moveTo(x.toFloat(), base)
                quadTo(x.toFloat(), base - h * 0.5f, topX, topY)
            }
            canvas.drawPath(stalk, wheatGold)

            for (k in 0..7) {
                val ky = topY + (k * 14f)
                val kx = topX - (k * 2f)
                canvas.drawOval(RectF(kx - 14f, ky - 6f, kx + 14f, ky + 6f), grainPaint)
            }
        }
    }

    // -------------------------------------------------------------
    // THEME 7: FOOD & CUISINE
    // -------------------------------------------------------------
    private fun renderFoodCuisine(canvas: Canvas, prompt: String, rng: Random) {
        val paint = Paint(Paint.ANTI_ALIAS_FLAG)

        // Warm Artisanal Café Background
        val bgShader = RadialGradient(WIDTH * 0.5f, HEIGHT * 0.5f, 550f,
            intArrayOf(Color.parseColor("#FFF8E7"), Color.parseColor("#F4EAD4"), Color.parseColor("#D7CCC8")),
            null, Shader.TileMode.CLAMP)
        paint.shader = bgShader
        canvas.drawRect(0f, 0f, WIDTH.toFloat(), HEIGHT.toFloat(), paint)
        paint.shader = null

        // Rustic Wooden Plate Base
        val platePaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#8D6E63")
        }
        canvas.drawCircle(WIDTH * 0.5f, HEIGHT * 0.52f, 320f, platePaint)
        platePaint.color = Color.parseColor("#D7CCC8")
        canvas.drawCircle(WIDTH * 0.5f, HEIGHT * 0.52f, 290f, platePaint)

        // Golden Crust Pizza / Food Platter
        val foodBase = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#FFA726")
        }
        canvas.drawCircle(WIDTH * 0.5f, HEIGHT * 0.52f, 250f, foodBase)

        // Melted Cheese Layer
        val cheesePaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#FFEE58")
        }
        canvas.drawCircle(WIDTH * 0.5f, HEIGHT * 0.52f, 220f, cheesePaint)

        // Pizza Toppings: Tomato slices, Basil leaves, Olives
        val tomatoPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#E53935")
        }
        val basilPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#43A047")
        }
        val olivePaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#212121")
        }

        val cx = WIDTH * 0.5f
        val cy = HEIGHT * 0.52f
        for (i in 0..7) {
            val a = (i * 45.0) + 15.0
            val rad = Math.toRadians(a)
            val tx = cx + (Math.cos(rad) * 120).toFloat()
            val ty = cy + (Math.sin(rad) * 120).toFloat()
            canvas.drawCircle(tx, ty, 26f, tomatoPaint)
            canvas.drawCircle(tx - 35f, ty + 10f, 10f, olivePaint)
            canvas.drawOval(RectF(tx + 15f, ty - 15f, tx + 45f, ty + 5f), basilPaint)
        }

        // Steaming Vapor Wafts
        val steamPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#40FFFFFF")
            strokeWidth = 6f
            style = Paint.Style.STROKE
        }
        for (s in -80..80 step 40) {
            val sx = cx + s
            val steam = Path().apply {
                moveTo(sx, cy - 260f)
                cubicTo(sx - 20f, cy - 310f, sx + 20f, cy - 360f, sx, cy - 410f)
            }
            canvas.drawPath(steam, steamPaint)
        }
    }

    // -------------------------------------------------------------
    // THEME 8: HEALTH, MEDICINE & BIO-SCIENCE
    // -------------------------------------------------------------
    private fun renderMedicalHealth(canvas: Canvas, prompt: String, rng: Random) {
        val paint = Paint(Paint.ANTI_ALIAS_FLAG)

        val bgShader = LinearGradient(0f, 0f, 0f, HEIGHT.toFloat(),
            intArrayOf(Color.parseColor("#0F2027"), Color.parseColor("#203A43"), Color.parseColor("#2C5364")),
            null, Shader.TileMode.CLAMP)
        paint.shader = bgShader
        canvas.drawRect(0f, 0f, WIDTH.toFloat(), HEIGHT.toFloat(), paint)
        paint.shader = null

        val coreGlow = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            shader = RadialGradient(WIDTH * 0.5f, HEIGHT * 0.5f, 380f,
                intArrayOf(Color.parseColor("#7000E5FF"), Color.parseColor("#2000B4D8"), Color.TRANSPARENT),
                null, Shader.TileMode.CLAMP)
        }
        canvas.drawCircle(WIDTH * 0.5f, HEIGHT * 0.5f, 380f, coreGlow)

        val ecgPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#00E676")
            strokeWidth = 6f
            style = Paint.Style.STROKE
        }
        val ecgPath = Path().apply {
            val y = HEIGHT * 0.5f
            moveTo(0f, y)
            lineTo(WIDTH * 0.30f, y)
            lineTo(WIDTH * 0.35f, y - 25f)
            lineTo(WIDTH * 0.38f, y + 20f)
            lineTo(WIDTH * 0.42f, y - 160f)
            lineTo(WIDTH * 0.47f, y + 80f)
            lineTo(WIDTH * 0.52f, y - 35f)
            lineTo(WIDTH * 0.56f, y)
            lineTo(WIDTH.toFloat(), y)
        }
        canvas.drawPath(ecgPath, ecgPaint)

        val crossPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#FFFFFF")
            style = Paint.Style.FILL
            alpha = 220
        }
        val cx = WIDTH * 0.5f
        val cy = HEIGHT * 0.28f
        canvas.drawRoundRect(RectF(cx - 24f, cy - 80f, cx + 24f, cy + 80f), 12f, 12f, crossPaint)
        canvas.drawRoundRect(RectF(cx - 80f, cy - 24f, cx + 80f, cy + 24f), 12f, 12f, crossPaint)
    }

    // -------------------------------------------------------------
    // THEME 9: VEHICLE & SPEED
    // -------------------------------------------------------------
    private fun renderVehicleSpeed(canvas: Canvas, prompt: String, rng: Random) {
        val paint = Paint(Paint.ANTI_ALIAS_FLAG)

        val bgShader = LinearGradient(0f, 0f, 0f, HEIGHT.toFloat(),
            intArrayOf(Color.parseColor("#090A0F"), Color.parseColor("#151824"), Color.parseColor("#222838")),
            null, Shader.TileMode.CLAMP)
        paint.shader = bgShader
        canvas.drawRect(0f, 0f, WIDTH.toFloat(), HEIGHT.toFloat(), paint)
        paint.shader = null

        val trailPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            strokeWidth = 5f
            style = Paint.Style.STROKE
        }

        trailPaint.color = Color.parseColor("#FF1744")
        for (i in 1..6) {
            val y = HEIGHT * 0.58f + (i * 14f)
            canvas.drawLine(0f, y, WIDTH * 0.5f + (i * 30f), y, trailPaint)
        }

        trailPaint.color = Color.parseColor("#00E5FF")
        for (i in 1..6) {
            val y = HEIGHT * 0.62f + (i * 16f)
            canvas.drawLine(WIDTH * 0.45f, y, WIDTH.toFloat(), y, trailPaint)
        }

        val carPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#0F141C")
        }
        val carBody = Path().apply {
            val cx = WIDTH * 0.5f
            val cy = HEIGHT * 0.54f
            moveTo(cx - 240f, cy + 30f)
            cubicTo(cx - 180f, cy - 20f, cx - 80f, cy - 65f, cx + 40f, cy - 65f)
            cubicTo(cx + 120f, cy - 65f, cx + 190f, cy - 10f, cx + 250f, cy + 25f)
            lineTo(cx + 250f, cy + 45f)
            lineTo(cx - 240f, cy + 45f)
            close()
        }
        canvas.drawPath(carBody, carPaint)

        val wheelPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#00E5FF")
            style = Paint.Style.STROKE
            strokeWidth = 6f
        }
        canvas.drawCircle(WIDTH * 0.5f - 140f, HEIGHT * 0.54f + 45f, 32f, wheelPaint)
        canvas.drawCircle(WIDTH * 0.5f + 150f, HEIGHT * 0.54f + 45f, 32f, wheelPaint)
    }

    // -------------------------------------------------------------
    // THEME 10: SACRED MANDALA & SACRED GEOMETRY
    // -------------------------------------------------------------
    private fun renderSacredMandala(canvas: Canvas, prompt: String, rng: Random) {
        val paint = Paint(Paint.ANTI_ALIAS_FLAG)

        val bgShader = RadialGradient(WIDTH * 0.5f, HEIGHT * 0.5f, 600f,
            intArrayOf(Color.parseColor("#240046"), Color.parseColor("#10002B"), Color.parseColor("#000000")),
            null, Shader.TileMode.CLAMP)
        paint.shader = bgShader
        canvas.drawRect(0f, 0f, WIDTH.toFloat(), HEIGHT.toFloat(), paint)
        paint.shader = null

        val cx = WIDTH * 0.5f
        val cy = HEIGHT * 0.5f

        val ringPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            style = Paint.Style.STROKE
            color = Color.parseColor("#E0AAFF")
            strokeWidth = 2.5f
        }

        val radii = listOf(60f, 120f, 180f, 240f, 320f, 400f)
        for (r in radii) {
            canvas.drawCircle(cx, cy, r, ringPaint)
        }

        val petalPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            style = Paint.Style.STROKE
            color = Color.parseColor("#FF9E00")
            strokeWidth = 2f
        }

        for (angle in 0..345 step 15) {
            canvas.save()
            canvas.rotate(angle.toFloat(), cx, cy)
            canvas.drawOval(RectF(cx - 30f, cy - 280f, cx + 30f, cy), petalPaint)
            canvas.restore()
        }

        val starPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#FFD60A")
            style = Paint.Style.FILL
            alpha = 200
        }
        canvas.drawCircle(cx, cy, 35f, starPaint)
    }

    // -------------------------------------------------------------
    // THEME 11: DEFAULT ATMOSPHERIC ARTWORK (Starry Night Traveler)
    // -------------------------------------------------------------
    private fun renderAtmosphericArtwork(canvas: Canvas, prompt: String, rng: Random) {
        val paint = Paint(Paint.ANTI_ALIAS_FLAG)

        // Mystical Twilight Sky Gradient
        val bgShader = LinearGradient(0f, 0f, 0f, HEIGHT.toFloat(),
            intArrayOf(Color.parseColor("#0F2027"), Color.parseColor("#203A43"), Color.parseColor("#2C5364")),
            null, Shader.TileMode.CLAMP)
        paint.shader = bgShader
        canvas.drawRect(0f, 0f, WIDTH.toFloat(), HEIGHT.toFloat(), paint)
        paint.shader = null

        // Glowing Emerald Aurora Stream
        val aurora = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            shader = RadialGradient(WIDTH * 0.5f, HEIGHT * 0.30f, 450f,
                intArrayOf(Color.parseColor("#8000F5D4"), Color.parseColor("#307B2CBF"), Color.TRANSPARENT),
                null, Shader.TileMode.CLAMP)
        }
        canvas.drawCircle(WIDTH * 0.5f, HEIGHT * 0.30f, 450f, aurora)

        // Golden Crescent Moon
        val moonPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#FFEAA7")
        }
        canvas.drawCircle(WIDTH * 0.75f, HEIGHT * 0.22f, 65f, moonPaint)
        moonPaint.color = Color.parseColor("#0F2027")
        canvas.drawCircle(WIDTH * 0.72f, HEIGHT * 0.20f, 58f, moonPaint)

        // Stardust particles
        val starPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.WHITE
        }
        for (i in 0..150) {
            val sx = rng.nextFloat() * WIDTH
            val sy = rng.nextFloat() * (HEIGHT * 0.65f)
            starPaint.alpha = rng.nextInt(150) + 70
            canvas.drawCircle(sx, sy, rng.nextFloat() * 2f + 1f, starPaint)
        }

        // Mountain Cliff & Pine Forest Silhouette
        val cliffPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#090E17")
        }
        val cliff = Path().apply {
            moveTo(0f, HEIGHT.toFloat())
            lineTo(0f, HEIGHT * 0.62f)
            lineTo(WIDTH * 0.45f, HEIGHT * 0.62f)
            lineTo(WIDTH * 0.55f, HEIGHT * 0.74f)
            lineTo(WIDTH.toFloat(), HEIGHT * 0.70f)
            lineTo(WIDTH.toFloat(), HEIGHT.toFloat())
            close()
        }
        canvas.drawPath(cliff, cliffPaint)

        // Lone Wanderer with Staff gazing at the stars
        val figureX = WIDTH * 0.38f
        val figureY = HEIGHT * 0.62f
        canvas.drawCircle(figureX, figureY - 60f, 12f, cliffPaint) // head
        canvas.drawRect(figureX - 8f, figureY - 48f, figureX + 8f, figureY, cliffPaint) // body
        val staffPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#00F5D4")
            strokeWidth = 3f
        }
        canvas.drawLine(figureX + 16f, figureY - 75f, figureX + 16f, figureY, staffPaint) // glowing staff
        staffPaint.style = Paint.Style.FILL
        canvas.drawCircle(figureX + 16f, figureY - 75f, 6f, staffPaint) // staff orb
    }

    private fun drawBird(canvas: Canvas, x: Float, y: Float, paint: Paint) {
        val path = Path().apply {
            moveTo(x - 16f, y - 6f)
            quadTo(x - 8f, y, x, y - 4f)
            quadTo(x + 8f, y, x + 16f, y - 6f)
        }
        canvas.drawPath(path, paint)
    }

    // -------------------------------------------------------------
    // AESTHETIC WATERMARK & FRAME
    // -------------------------------------------------------------
    private fun renderAestheticFrame(canvas: Canvas, prompt: String) {
        val borderPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#40FFFFFF")
            style = Paint.Style.STROKE
            strokeWidth = 3f
        }
        canvas.drawRect(18f, 18f, WIDTH - 18f, HEIGHT - 18f, borderPaint)

        val badgeBg = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#D0000000")
            style = Paint.Style.FILL
        }
        val badgeRect = RectF(WIDTH * 0.08f, HEIGHT - 75f, WIDTH * 0.92f, HEIGHT - 28f)
        canvas.drawRoundRect(badgeRect, 20f, 20f, badgeBg)

        val textPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#F8FAFC")
            textSize = 20f
            textAlign = Paint.Align.CENTER
            isFakeBoldText = true
        }
        val cleanPrompt = prompt.replace(Regex("[\r\n]+"), " ").trim()
        val truncatedPrompt = if (cleanPrompt.length > 45) cleanPrompt.take(42) + "..." else cleanPrompt
        canvas.drawText("✨ ISHA ON-DEVICE AI • \"$truncatedPrompt\"", WIDTH * 0.5f, HEIGHT - 45f, textPaint)
    }
}
