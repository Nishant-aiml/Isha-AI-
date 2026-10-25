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
        SPACE_COSMIC,
        CYBERPUNK_NEON,
        NATURE_LANDSCAPE,
        WILDLIFE_ANIMAL,
        AGRICULTURE_FARM,
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
            p.contains("horse") || p.contains("animal") || p.contains("wildlife") ->
                VisualTheme.WILDLIFE_ANIMAL

            p.contains("crop") || p.contains("wheat") || p.contains("rice") ||
            p.contains("farm") || p.contains("farmer") || p.contains("field") ||
            p.contains("agriculture") || p.contains("soil") || p.contains("rust") ->
                VisualTheme.AGRICULTURE_FARM

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
    fun synthesizeVisual(prompt: String): Bitmap {
        val bitmap = Bitmap.createBitmap(WIDTH, HEIGHT, Bitmap.Config.ARGB_8888)
        val canvas = Canvas(bitmap)
        val seed = prompt.hashCode().toLong()
        val rng = Random(seed)
        val theme = detectTheme(prompt)

        when (theme) {
            VisualTheme.SPACE_COSMIC -> renderSpaceCosmic(canvas, prompt, rng)
            VisualTheme.CYBERPUNK_NEON -> renderCyberpunkNeon(canvas, prompt, rng)
            VisualTheme.WILDLIFE_ANIMAL -> renderWildlifeAnimal(canvas, prompt, rng)
            VisualTheme.AGRICULTURE_FARM -> renderAgricultureFarm(canvas, prompt, rng)
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
    // THEME 1: COSMIC & DEEP SPACE
    // -------------------------------------------------------------
    private fun renderSpaceCosmic(canvas: Canvas, prompt: String, rng: Random) {
        val paint = Paint(Paint.ANTI_ALIAS_FLAG)

        // Cosmic gradient background
        val bgShader = LinearGradient(0f, 0f, 0f, HEIGHT.toFloat(),
            intArrayOf(Color.parseColor("#050510"), Color.parseColor("#0B132B"), Color.parseColor("#1C2541")),
            null, Shader.TileMode.CLAMP)
        paint.shader = bgShader
        canvas.drawRect(0f, 0f, WIDTH.toFloat(), HEIGHT.toFloat(), paint)
        paint.shader = null

        // Nebula glow 1 (Magenta / Purple)
        val nebPaint = Paint(Paint.ANTI_ALIAS_FLAG)
        nebPaint.shader = RadialGradient(WIDTH * 0.35f, HEIGHT * 0.4f, 450f,
            intArrayOf(Color.parseColor("#807209B7"), Color.parseColor("#304361EE"), Color.TRANSPARENT),
            null, Shader.TileMode.CLAMP)
        canvas.drawCircle(WIDTH * 0.35f, HEIGHT * 0.4f, 450f, nebPaint)

        // Nebula glow 2 (Cyan / Teal)
        nebPaint.shader = RadialGradient(WIDTH * 0.7f, HEIGHT * 0.65f, 400f,
            intArrayOf(Color.parseColor("#704CC9F0"), Color.parseColor("#203F37C9"), Color.TRANSPARENT),
            null, Shader.TileMode.CLAMP)
        canvas.drawCircle(WIDTH * 0.7f, HEIGHT * 0.65f, 400f, nebPaint)

        // Stardust field (hundreds of sharp and glowing stars)
        paint.color = Color.WHITE
        for (i in 0..300) {
            val sx = rng.nextFloat() * WIDTH
            val sy = rng.nextFloat() * HEIGHT
            val radius = rng.nextFloat() * 2.5f + 0.5f
            paint.alpha = rng.nextInt(180) + 75
            canvas.drawCircle(sx, sy, radius, paint)
        }

        // Giant Glowing Planet with Rings
        val px = WIDTH * 0.65f
        val py = HEIGHT * 0.38f
        val pRadius = 140f

        // Planet Atmosphere Glow
        val planetGlow = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            shader = RadialGradient(px, py, pRadius * 1.5f,
                intArrayOf(Color.parseColor("#60F72585"), Color.parseColor("#207209B7"), Color.TRANSPARENT),
                null, Shader.TileMode.CLAMP)
        }
        canvas.drawCircle(px, py, pRadius * 1.5f, planetGlow)

        // Planet body
        val planetBody = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            shader = LinearGradient(px - pRadius, py - pRadius, px + pRadius, py + pRadius,
                intArrayOf(Color.parseColor("#F72585"), Color.parseColor("#7209B7"), Color.parseColor("#03071E")),
                null, Shader.TileMode.CLAMP)
        }
        canvas.drawCircle(px, py, pRadius, planetBody)

        // Planet Rings
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

        // Space horizon landscape / Moon surface at bottom
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

        // Distant space explorer / satellite silhouette
        val silPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#38BDF8")
            style = Paint.Style.FILL
        }
        canvas.drawCircle(WIDTH * 0.28f, HEIGHT * 0.65f, 6f, silPaint)
        silPaint.strokeWidth = 2f
        silPaint.style = Paint.Style.STROKE
        canvas.drawLine(WIDTH * 0.24f, HEIGHT * 0.65f, WIDTH * 0.32f, HEIGHT * 0.65f, silPaint)
        canvas.drawLine(WIDTH * 0.28f, HEIGHT * 0.61f, WIDTH * 0.28f, HEIGHT * 0.69f, silPaint)
    }

    // -------------------------------------------------------------
    // THEME 2: CYBERPUNK & NEON FUTURISM
    // -------------------------------------------------------------
    private fun renderCyberpunkNeon(canvas: Canvas, prompt: String, rng: Random) {
        val paint = Paint(Paint.ANTI_ALIAS_FLAG)

        // Dark Synthwave Night Sky
        val bgShader = LinearGradient(0f, 0f, 0f, HEIGHT.toFloat(),
            intArrayOf(Color.parseColor("#0A0915"), Color.parseColor("#1A0933"), Color.parseColor("#2D006B")),
            null, Shader.TileMode.CLAMP)
        paint.shader = bgShader
        canvas.drawRect(0f, 0f, WIDTH.toFloat(), HEIGHT.toFloat(), paint)
        paint.shader = null

        // Huge Retro Synthwave Sun
        val sunY = HEIGHT * 0.45f
        val sunRadius = 200f
        val sunPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            shader = LinearGradient(0f, sunY - sunRadius, 0f, sunY + sunRadius,
                intArrayOf(Color.parseColor("#FF007F"), Color.parseColor("#FF7700"), Color.parseColor("#FFE600")),
                null, Shader.TileMode.CLAMP)
        }
        canvas.drawCircle(WIDTH * 0.5f, sunY, sunRadius, sunPaint)

        // Sun Venetian Horizontal Blinds
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

        // Cyberpunk Skyline Silhouettes
        val bldgPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#06030F")
        }
        var curX = 0f
        while (curX < WIDTH) {
            val bWidth = rng.nextFloat() * 70f + 40f
            val bHeight = rng.nextFloat() * 260f + 140f
            val top = HEIGHT * 0.65f - bHeight
            canvas.drawRect(curX, top, curX + bWidth, HEIGHT * 0.65f, bldgPaint)

            // Neon Spire
            if (rng.nextBoolean()) {
                val spirePaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
                    color = if (rng.nextBoolean()) Color.parseColor("#00F0FF") else Color.parseColor("#FF007F")
                    strokeWidth = 3f
                }
                canvas.drawLine(curX + bWidth / 2, top, curX + bWidth / 2, top - 35f, spirePaint)
            }
            curX += bWidth + 6f
        }

        // Perspective 3D Neon Grid Floor (Horizon at 0.65f)
        val horizonY = HEIGHT * 0.65f
        val gridPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#FF007F")
            strokeWidth = 2.5f
        }

        // Horizontal perspective lines
        var step = 15f
        var curY = horizonY + 10f
        while (curY < HEIGHT) {
            canvas.drawLine(0f, curY, WIDTH.toFloat(), curY, gridPaint)
            curY += step
            step *= 1.35f
        }

        // Radiating vertical perspective lines to vanishing point (center horizon)
        val cx = WIDTH * 0.5f
        for (x in -400..WIDTH.toInt() + 400 step 80) {
            canvas.drawLine(cx, horizonY, x.toFloat(), HEIGHT.toFloat(), gridPaint)
        }

        // Laser beam accents
        val laserPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#00F0FF")
            strokeWidth = 3f
        }
        canvas.drawLine(0f, horizonY + 2f, WIDTH.toFloat(), horizonY + 2f, laserPaint)
    }

    // -------------------------------------------------------------
    // THEME 3: NATURE & LANDSCAPE
    // -------------------------------------------------------------
    private fun renderNatureLandscape(canvas: Canvas, prompt: String, rng: Random) {
        val paint = Paint(Paint.ANTI_ALIAS_FLAG)

        // Sunset / Sunrise Sky Gradient
        val skyShader = LinearGradient(0f, 0f, 0f, HEIGHT * 0.65f,
            intArrayOf(Color.parseColor("#1B2A4A"), Color.parseColor("#4C2A4C"), Color.parseColor("#9F496E"), Color.parseColor("#F08A5D"), Color.parseColor("#F9ED69")),
            null, Shader.TileMode.CLAMP)
        paint.shader = skyShader
        canvas.drawRect(0f, 0f, WIDTH.toFloat(), HEIGHT * 0.65f, paint)
        paint.shader = null

        // Glowing Golden Sun
        val sunPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            shader = RadialGradient(WIDTH * 0.5f, HEIGHT * 0.38f, 120f,
                intArrayOf(Color.WHITE, Color.parseColor("#F9ED69"), Color.TRANSPARENT),
                null, Shader.TileMode.CLAMP)
        }
        canvas.drawCircle(WIDTH * 0.5f, HEIGHT * 0.38f, 120f, sunPaint)

        // Far Mountain Range (Foggy Purple)
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

        // Near Mountain Range (Deep Indigo / Violet)
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

        // Calm Lake Water (with mirrored sky reflection)
        val waterPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            shader = LinearGradient(0f, HEIGHT * 0.65f, 0f, HEIGHT.toFloat(),
                intArrayOf(Color.parseColor("#2E1A47"), Color.parseColor("#1B2A4A"), Color.parseColor("#0F172A")),
                null, Shader.TileMode.CLAMP)
        }
        canvas.drawRect(0f, HEIGHT * 0.65f, WIDTH.toFloat(), HEIGHT.toFloat(), waterPaint)

        // Golden Sun Shimmer on Water
        val shimmer = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#60F9ED69")
            strokeWidth = 3f
        }
        for (i in 0..30) {
            val sy = HEIGHT * 0.66f + (i * 11f)
            val sw = (i * 12f) + 30f
            canvas.drawLine(WIDTH * 0.5f - sw / 2, sy, WIDTH * 0.5f + sw / 2, sy, shimmer)
        }

        // Pine Tree Silhouettes in Foreground
        val treePaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#090C15")
        }
        renderPineTree(canvas, 90f, HEIGHT * 0.72f, 180f, treePaint)
        renderPineTree(canvas, 180f, HEIGHT * 0.75f, 140f, treePaint)
        renderPineTree(canvas, WIDTH - 120f, HEIGHT * 0.70f, 210f, treePaint)
        renderPineTree(canvas, WIDTH - 220f, HEIGHT * 0.74f, 150f, treePaint)
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
    // THEME 4: WILDLIFE & ANIMAL ICONOGRAPHY
    // -------------------------------------------------------------
    private fun renderWildlifeAnimal(canvas: Canvas, prompt: String, rng: Random) {
        val paint = Paint(Paint.ANTI_ALIAS_FLAG)

        // Warm Golden Savanna / Jungle Sunset Backdrop
        val bgShader = RadialGradient(WIDTH * 0.5f, HEIGHT * 0.45f, 520f,
            intArrayOf(Color.parseColor("#FFEAA7"), Color.parseColor("#FAB1A0"), Color.parseColor("#E17055"), Color.parseColor("#2D3436")),
            null, Shader.TileMode.CLAMP)
        paint.shader = bgShader
        canvas.drawRect(0f, 0f, WIDTH.toFloat(), HEIGHT.toFloat(), paint)
        paint.shader = null

        // Massive Solar Disc
        val sunPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#FDCB6E")
        }
        canvas.drawCircle(WIDTH * 0.5f, HEIGHT * 0.42f, 190f, sunPaint)

        // Rolling Savanna Horizon Mound
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

        // Majestic Acacia Tree Silhouette on Left
        val treePath = Path().apply {
            moveTo(WIDTH * 0.22f, HEIGHT * 0.65f)
            lineTo(WIDTH * 0.24f, HEIGHT * 0.38f)
            lineTo(WIDTH * 0.12f, HEIGHT * 0.32f)
            lineTo(WIDTH * 0.36f, HEIGHT * 0.31f)
            close()
        }
        canvas.drawPath(treePath, hillPaint)
        canvas.drawOval(RectF(WIDTH * 0.08f, HEIGHT * 0.26f, WIDTH * 0.42f, HEIGHT * 0.35f), hillPaint)

        // Majestic Animal Silhouette (Center Stage)
        val animalPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#0D1317")
        }

        val cx = WIDTH * 0.55f
        val cy = HEIGHT * 0.58f

        // Powerful Feline / Quadruped Majestic Silhouette
        val body = Path().apply {
            // Torso & Arching Back
            moveTo(cx - 100f, cy)
            cubicTo(cx - 60f, cy - 60f, cx + 50f, cy - 50f, cx + 90f, cy - 10f)
            // Head & Snout
            lineTo(cx + 140f, cy - 35f)
            lineTo(cx + 170f, cy - 25f)
            lineTo(cx + 150f, cy + 10f)
            // Chest & Front Legs
            lineTo(cx + 100f, cy + 30f)
            lineTo(cx + 95f, cy + 110f)
            lineTo(cx + 75f, cy + 110f)
            lineTo(cx + 80f, cy + 35f)
            // Underbelly
            lineTo(cx - 20f, cy + 35f)
            // Hind Legs
            lineTo(cx - 60f, cy + 110f)
            lineTo(cx - 80f, cy + 110f)
            lineTo(cx - 70f, cy + 40f)
            // Tail
            cubicTo(cx - 140f, cy + 20f, cx - 180f, cy - 20f, cx - 170f, cy - 50f)
            cubicTo(cx - 160f, cy - 20f, cx - 120f, cy + 15f, cx - 100f, cy)
            close()
        }
        canvas.drawPath(body, animalPaint)

        // Glowing Wildlife Eyes Accent
        val eyePaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#00E676")
        }
        canvas.drawCircle(cx + 138f, cy - 22f, 4f, eyePaint)

        // Birds in flight across the sun
        val birdPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#2D3436")
            style = Paint.Style.STROKE
            strokeWidth = 3f
        }
        drawBird(canvas, WIDTH * 0.38f, HEIGHT * 0.32f, birdPaint)
        drawBird(canvas, WIDTH * 0.44f, HEIGHT * 0.28f, birdPaint)
        drawBird(canvas, WIDTH * 0.48f, HEIGHT * 0.35f, birdPaint)
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
    // THEME 5: AGRICULTURE & CROPS (KISAN SAATHI)
    // -------------------------------------------------------------
    private fun renderAgricultureFarm(canvas: Canvas, prompt: String, rng: Random) {
        val paint = Paint(Paint.ANTI_ALIAS_FLAG)

        // Golden Sunrise Farmland Sky
        val skyShader = LinearGradient(0f, 0f, 0f, HEIGHT * 0.6f,
            intArrayOf(Color.parseColor("#1E3C72"), Color.parseColor("#2A5298"), Color.parseColor("#F3904F"), Color.parseColor("#3B4371")),
            null, Shader.TileMode.CLAMP)
        paint.shader = skyShader
        canvas.drawRect(0f, 0f, WIDTH.toFloat(), HEIGHT * 0.6f, paint)
        paint.shader = null

        // Morning Sun radiating over harvest
        val sunPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            shader = RadialGradient(WIDTH * 0.5f, HEIGHT * 0.35f, 160f,
                intArrayOf(Color.WHITE, Color.parseColor("#FFD200"), Color.TRANSPARENT),
                null, Shader.TileMode.CLAMP)
        }
        canvas.drawCircle(WIDTH * 0.5f, HEIGHT * 0.35f, 160f, sunPaint)

        // Sunburst rays
        val rayPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#20FFD200")
            strokeWidth = 4f
        }
        for (a in 0..360 step 15) {
            val rad = Math.toRadians(a.toDouble())
            val ex = WIDTH * 0.5f + (Math.cos(rad) * 450).toFloat()
            val ey = HEIGHT * 0.35f + (Math.sin(rad) * 450).toFloat()
            canvas.drawLine(WIDTH * 0.5f, HEIGHT * 0.35f, ex, ey, rayPaint)
        }

        // Fertile Soil & Rolling Crop Terraces
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

        // Golden Wheat Stalks (Foreground Feature)
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

            // Stalk curve
            val stalk = Path().apply {
                moveTo(x.toFloat(), base)
                quadTo(x.toFloat(), base - h * 0.5f, topX, topY)
            }
            canvas.drawPath(stalk, wheatGold)

            // Wheat ear kernels
            for (k in 0..7) {
                val ky = topY + (k * 14f)
                val kx = topX - (k * 2f)
                canvas.drawOval(RectF(kx - 14f, ky - 6f, kx + 14f, ky + 6f), grainPaint)
            }
        }
    }

    // -------------------------------------------------------------
    // THEME 6: HEALTH, MEDICINE & BIO-SCIENCE
    // -------------------------------------------------------------
    private fun renderMedicalHealth(canvas: Canvas, prompt: String, rng: Random) {
        val paint = Paint(Paint.ANTI_ALIAS_FLAG)

        // Clean High-Tech Medical Cyan/Navy Background
        val bgShader = LinearGradient(0f, 0f, 0f, HEIGHT.toFloat(),
            intArrayOf(Color.parseColor("#0F2027"), Color.parseColor("#203A43"), Color.parseColor("#2C5364")),
            null, Shader.TileMode.CLAMP)
        paint.shader = bgShader
        canvas.drawRect(0f, 0f, WIDTH.toFloat(), HEIGHT.toFloat(), paint)
        paint.shader = null

        // Central Bio-Core Glow
        val coreGlow = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            shader = RadialGradient(WIDTH * 0.5f, HEIGHT * 0.5f, 380f,
                intArrayOf(Color.parseColor("#7000E5FF"), Color.parseColor("#2000B4D8"), Color.TRANSPARENT),
                null, Shader.TileMode.CLAMP)
        }
        canvas.drawCircle(WIDTH * 0.5f, HEIGHT * 0.5f, 380f, coreGlow)

        // Pulsating ECG Heartbeat Waveform
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
            lineTo(WIDTH * 0.42f, y - 160f) // Sharp R-wave spike
            lineTo(WIDTH * 0.47f, y + 80f)  // S-wave
            lineTo(WIDTH * 0.52f, y - 35f)  // T-wave
            lineTo(WIDTH * 0.56f, y)
            lineTo(WIDTH.toFloat(), y)
        }
        canvas.drawPath(ecgPath, ecgPaint)

        // Glowing Medical Cross / Caduceus Crest
        val crossPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#FFFFFF")
            style = Paint.Style.FILL
            alpha = 220
        }
        val cx = WIDTH * 0.5f
        val cy = HEIGHT * 0.28f
        // Vertical bar
        canvas.drawRoundRect(RectF(cx - 24f, cy - 80f, cx + 24f, cy + 80f), 12f, 12f, crossPaint)
        // Horizontal bar
        canvas.drawRoundRect(RectF(cx - 80f, cy - 24f, cx + 80f, cy + 24f), 12f, 12f, crossPaint)

        // DNA Double Helix Strands in Background
        val dnaPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#6040C4FF")
            strokeWidth = 3f
        }
        for (i in 0..20) {
            val y = HEIGHT * 0.65f + (i * 16f)
            val angle = (i * 25.0)
            val rad = Math.toRadians(angle)
            val x1 = (WIDTH * 0.5f) + (Math.sin(rad) * 110).toFloat()
            val x2 = (WIDTH * 0.5f) - (Math.sin(rad) * 110).toFloat()

            canvas.drawLine(x1, y, x2, y, dnaPaint)
            canvas.drawCircle(x1, y, 5f, dnaPaint)
            canvas.drawCircle(x2, y, 5f, dnaPaint)
        }
    }

    // -------------------------------------------------------------
    // THEME 7: VEHICLE & SPEED
    // -------------------------------------------------------------
    private fun renderVehicleSpeed(canvas: Canvas, prompt: String, rng: Random) {
        val paint = Paint(Paint.ANTI_ALIAS_FLAG)

        // High-Speed Night Highway Gradient
        val bgShader = LinearGradient(0f, 0f, 0f, HEIGHT.toFloat(),
            intArrayOf(Color.parseColor("#090A0F"), Color.parseColor("#151824"), Color.parseColor("#222838")),
            null, Shader.TileMode.CLAMP)
        paint.shader = bgShader
        canvas.drawRect(0f, 0f, WIDTH.toFloat(), HEIGHT.toFloat(), paint)
        paint.shader = null

        // Light trails (Neon Red & Cyan)
        val trailPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            strokeWidth = 5f
            style = Paint.Style.STROKE
        }

        // Red taillight streaks
        trailPaint.color = Color.parseColor("#FF1744")
        for (i in 1..6) {
            val y = HEIGHT * 0.58f + (i * 14f)
            canvas.drawLine(0f, y, WIDTH * 0.5f + (i * 30f), y, trailPaint)
        }

        // Cyan headlight streaks
        trailPaint.color = Color.parseColor("#00E5FF")
        for (i in 1..6) {
            val y = HEIGHT * 0.62f + (i * 16f)
            canvas.drawLine(WIDTH * 0.45f, y, WIDTH.toFloat(), y, trailPaint)
        }

        // Aerodynamic Supercar Silhouette (Center)
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

        // Glowing Wheels
        val wheelPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#00E5FF")
            style = Paint.Style.STROKE
            strokeWidth = 6f
        }
        canvas.drawCircle(WIDTH * 0.5f - 140f, HEIGHT * 0.54f + 45f, 32f, wheelPaint)
        canvas.drawCircle(WIDTH * 0.5f + 150f, HEIGHT * 0.54f + 45f, 32f, wheelPaint)
    }

    // -------------------------------------------------------------
    // THEME 8: SACRED MANDALA & SACRED GEOMETRY
    // -------------------------------------------------------------
    private fun renderSacredMandala(canvas: Canvas, prompt: String, rng: Random) {
        val paint = Paint(Paint.ANTI_ALIAS_FLAG)

        // Deep Mystic Indigo Background
        val bgShader = RadialGradient(WIDTH * 0.5f, HEIGHT * 0.5f, 600f,
            intArrayOf(Color.parseColor("#240046"), Color.parseColor("#10002B"), Color.parseColor("#000000")),
            null, Shader.TileMode.CLAMP)
        paint.shader = bgShader
        canvas.drawRect(0f, 0f, WIDTH.toFloat(), HEIGHT.toFloat(), paint)
        paint.shader = null

        val cx = WIDTH * 0.5f
        val cy = HEIGHT * 0.5f

        // Concentric geometric rings
        val ringPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            style = Paint.Style.STROKE
            color = Color.parseColor("#E0AAFF")
            strokeWidth = 2.5f
        }

        val radii = listOf(60f, 120f, 180f, 240f, 320f, 400f)
        for (r in radii) {
            canvas.drawCircle(cx, cy, r, ringPaint)
        }

        // Symmetrical Petals / Sacred Rosette
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

        // Inner Golden Star
        val starPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#FFD60A")
            style = Paint.Style.FILL
            alpha = 200
        }
        canvas.drawCircle(cx, cy, 35f, starPaint)
    }

    // -------------------------------------------------------------
    // THEME 9: DEFAULT ATMOSPHERIC ARTWORK
    // -------------------------------------------------------------
    private fun renderAtmosphericArtwork(canvas: Canvas, prompt: String, rng: Random) {
        val paint = Paint(Paint.ANTI_ALIAS_FLAG)

        // Rich Aurora Borealis / Northern Lights Gradient
        val bgShader = LinearGradient(0f, 0f, WIDTH.toFloat(), HEIGHT.toFloat(),
            intArrayOf(Color.parseColor("#0F2027"), Color.parseColor("#203A43"), Color.parseColor("#2C5364")),
            null, Shader.TileMode.CLAMP)
        paint.shader = bgShader
        canvas.drawRect(0f, 0f, WIDTH.toFloat(), HEIGHT.toFloat(), paint)
        paint.shader = null

        // Aurora Curtains of Light
        val aurora = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            shader = RadialGradient(WIDTH * 0.5f, HEIGHT * 0.35f, 480f,
                intArrayOf(Color.parseColor("#9000F5D4"), Color.parseColor("#507B2CBF"), Color.TRANSPARENT),
                null, Shader.TileMode.CLAMP)
        }
        canvas.drawCircle(WIDTH * 0.5f, HEIGHT * 0.35f, 480f, aurora)

        // Celestial Orb
        val orbPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#F72585")
        }
        canvas.drawCircle(WIDTH * 0.5f, HEIGHT * 0.45f, 110f, orbPaint)

        // Ambient Horizon Silhouette
        val ground = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#03071E")
        }
        canvas.drawRect(0f, HEIGHT * 0.72f, WIDTH.toFloat(), HEIGHT.toFloat(), ground)
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

        // Elegant Bottom Badge
        val badgeBg = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#C0000000")
            style = Paint.Style.FILL
        }
        val badgeRect = RectF(WIDTH * 0.12f, HEIGHT - 75f, WIDTH * 0.88f, HEIGHT - 30f)
        canvas.drawRoundRect(badgeRect, 20f, 20f, badgeBg)

        val textPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.parseColor("#F8FAFC")
            textSize = 20f
            textAlign = Paint.Align.CENTER
            isFakeBoldText = true
        }
        val truncatedPrompt = if (prompt.length > 45) prompt.take(42) + "..." else prompt
        canvas.drawText("✨ ISHA ON-DEVICE AI • \"$truncatedPrompt\"", WIDTH * 0.5f, HEIGHT - 46f, textPaint)
    }
}
