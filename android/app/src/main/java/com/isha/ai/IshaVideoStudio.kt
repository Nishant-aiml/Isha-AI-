package com.isha.ai

/**
 * Video Storyboard Studio.
 * Generates structured 8-second multi-scene promotional video storyboard sequences
 * with camera instructions, audio voiceover scripts, and keyframe visual briefs.
 * Works 100% Offline with zero cloud credits required.
 */
object IshaVideoStudio {

    data class SceneShot(
        val shotNumber: Int,
        val timestampRange: String,
        val cameraMotion: String,
        val visualBrief: String,
        val voiceoverScript: String,
        val audioMusicCue: String
    )

    data class VideoStoryboard(
        val title: String,
        val totalDurationSeconds: Int,
        val brandTagline: String,
        val scenes: List<SceneShot>
    )

    fun generateStoryboard(productOrTopic: String, brandName: String = "Heritage"): VideoStoryboard {
        val topic = productOrTopic.trim().ifEmpty { "Heritage Saree Collection" }
        val brand = brandName.trim().ifEmpty { "Rangrez Chandni" }

        val scenes = listOf(
            SceneShot(
                shotNumber = 1,
                timestampRange = "0.0s – 2.5s",
                cameraMotion = "Slow macro orbit with shallow depth of field",
                visualBrief = "Extreme close-up of intricate golden zari threadwork gleaming under warm studio rim-lighting. Slow panning reveal of authentic silk weave texture.",
                voiceoverScript = "Where heritage whispers in every golden thread...",
                audioMusicCue = "Soft ambient sitar swell with warm acoustic pad"
            ),
            SceneShot(
                shotNumber = 2,
                timestampRange = "2.5s – 5.5s",
                cameraMotion = "Medium tracking shot with graceful movement",
                visualBrief = "An elegant Indian woman in an aesthetic courtyard archway, wearing the signature saree. The fabric drapes gracefully with natural daylight reflections.",
                voiceoverScript = "Crafted for timeless celebrations and modern royalty.",
                audioMusicCue = "Rhythmic percussion drops in with subtle tabla cadence"
            ),
            SceneShot(
                shotNumber = 3,
                timestampRange = "5.5s – 8.0s",
                cameraMotion = "Smooth zoom out to cinematic hero frame",
                visualBrief = "Model poses proudly. Minimalist golden brand watermark logo appears in bottom corner with clean typography reveal.",
                voiceoverScript = "$brand. Rang Bhi, Shaan Bhi.",
                audioMusicCue = "Resolving melodic crescendo with soft chime finish"
            )
        )

        return VideoStoryboard(
            title = "$brand — 8s Promotional Video Storyboard",
            totalDurationSeconds = 8,
            brandTagline = "Rang Bhi, Shaan Bhi",
            scenes = scenes
        )
    }

    fun formatStoryboardMarkdown(storyboard: VideoStoryboard): String {
        val sb = StringBuilder()
        sb.append("🎬 **${storyboard.title}** (${storyboard.totalDurationSeconds}s Total Duration)\n\n")
        sb.append("🏷️ **Tagline**: \"${storyboard.brandTagline}\"\n\n")

        storyboard.scenes.forEach { s ->
            sb.append("### 🎥 Shot ${s.shotNumber} (${s.timestampRange})\n")
            sb.append("• **Camera Motion**: ${s.cameraMotion}\n")
            sb.append("• **Visual**: ${s.visualBrief}\n")
            sb.append("• **Voiceover**: 🎙️ \"${s.voiceoverScript}\"\n")
            sb.append("• **Audio/Music**: 🎵 ${s.audioMusicCue}\n\n")
        }

        sb.append("✨ *Ready for 8-second cloud video rendering or local animation sequencing.*")
        return sb.toString()
    }
}
