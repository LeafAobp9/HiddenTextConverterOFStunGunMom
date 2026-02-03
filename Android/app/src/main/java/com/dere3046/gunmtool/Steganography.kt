package com.dere3046.gunmtool

import android.util.Base64
import java.nio.charset.StandardCharsets

object Steganography {
    private const val ZERO_WIDTH_ZERO = '\u200B'   // Zero-width space
    private const val ZERO_WIDTH_ONE = '\u200C'    // Zero-width non-joiner
    private const val ZERO_WIDTH_SEP = '\u200D'    // Zero-width joiner
    private const val ZERO_WIDTH_START = '\uFEFF'  // Zero-width no-break space (start marker)
    private const val ZERO_WIDTH_END = '\u2060'    // Word joiner (end marker)

    // Convert text to hidden zero-width characters
    fun hideText(text: String): String {
        if (text.isBlank()) return ""

        // Convert text to Base64
        val base64 = Base64.encodeToString(
            text.toByteArray(StandardCharsets.UTF_8),
            Base64.DEFAULT
        ).trim()

        // Convert Base64 to binary string
        val binary = StringBuilder()
        for (char in base64) {
            val charCode = char.code
            binary.append(charCode.toString(2).padStart(8, '0'))
        }

        val hidden = StringBuilder(ZERO_WIDTH_START.toString())
        
        for (i in binary.indices) {
            hidden.append(if (binary[i] == '0') ZERO_WIDTH_ZERO else ZERO_WIDTH_ONE)
            
            // Add separator every 8 bits (except the last byte)
            if ((i + 1) % 8 == 0 && i + 1 < binary.length) {
                hidden.append(ZERO_WIDTH_SEP)
            }
        }
        
        hidden.append(ZERO_WIDTH_END)
        return hidden.toString()
    }

    // Extract text from zero-width characters
    fun revealText(hiddenText: String): String {
        if (hiddenText.isBlank()) return ""

        var processed = hiddenText.trim()
        
        // Remove start and end markers if present
        if (processed.startsWith(ZERO_WIDTH_START)) {
            processed = processed.substring(1)
        }
        if (processed.endsWith(ZERO_WIDTH_END)) {
            processed = processed.substring(0, processed.length - 1)
        }

        // Remove separators
        processed = processed.replace(ZERO_WIDTH_SEP.toString(), "")

        val binary = StringBuilder()
        for (char in processed) {
            when (char) {
                ZERO_WIDTH_ZERO -> binary.append('0')
                ZERO_WIDTH_ONE -> binary.append('1')
            }
        }

        val bytes = mutableListOf<Byte>()
        for (i in 0 until binary.length step 8) {
            if (i + 8 <= binary.length) {
                val byteStr = binary.substring(i, i + 8)
                bytes.add(byteStr.toInt(2).toByte())
            }
        }

        val base64 = String(bytes.toByteArray(), StandardCharsets.UTF_8)
        return try {
            val decodedBytes = Base64.decode(base64, Base64.DEFAULT)
            String(decodedBytes, StandardCharsets.UTF_8)
        } catch (e: Exception) {
            "" // Return empty string if decoding fails
        }
    }
}