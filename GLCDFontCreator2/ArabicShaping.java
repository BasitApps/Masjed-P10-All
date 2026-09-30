package com.basitapps.masjedclockwifi.classes;

import java.util.HashMap;
import java.util.Map;

/**
 * ArabicShaping handles the conversion of Arabic characters to their contextual forms
 * (Initial, Medial, Final, Isolated) for correct display on LED matrices.
 * Optimized for performance using O(1) lookups.
 */
public final class ArabicShaping {

    private ArabicShaping() {
        // Prevent instantiation
    }

    /**
     * Reshapes the input text for the LED clock.
     * @param originalTxt The raw Arabic text.
     * @return The reshaped and reversed text suitable for the LED display.
     */
    public static String myReshape(String originalTxt) {
        if (originalTxt == null || originalTxt.isEmpty()) return "";

        StringBuilder result = new StringBuilder();
        String[] words = originalTxt.split("\\s+");

        for (int i = 0; i < words.length; i++) {
            String word = words[i];
            StringBuilder reshapedWord = new StringBuilder();

            for (int x = 0; x < word.length(); x++) {
                char current = word.charAt(x);
                GlyphData glyph = GLYPH_MAP.get(current);

                if (glyph == null) {
                    reshapedWord.append(current); // Append non-arabic chars as is
                    continue;
                }

                if (word.length() == 1) {
                    reshapedWord.append(glyph.isolated);
                } else {
                    if (x == 0) {
                        // First letter: use Initial form
                        reshapedWord.append(glyph.initial);
                    } else if (x < word.length() - 1) {
                        // Middle letter: depends on previous letter's connection property
                        char prevChar = word.charAt(x - 1);
                        GlyphData prevGlyph = GLYPH_MAP.get(prevChar);
                        
                        if (prevGlyph != null && prevGlyph.connectionType == 2) {
                            reshapedWord.append(glyph.initial); // Prev char doesn't connect forward
                        } else {
                            reshapedWord.append(glyph.medial);
                        }
                    } else {
                        // Last letter: depends on previous letter's connection property
                        char prevChar = word.charAt(x - 1);
                        GlyphData prevGlyph = GLYPH_MAP.get(prevChar);

                        if (prevGlyph != null && prevGlyph.connectionType == 2) {
                            reshapedWord.append(glyph.isolated);
                        } else {
                            reshapedWord.append(glyph.finalForm);
                        }
                    }
                }
            }
            result.append(reshapedWord);
            if (i < words.length - 1) result.append(" ");
        }

        return reverse(lamAlf(result.toString()));
    }

    /**
     * Handles specific character replacements required by the clock hardware.
     */
    public static String lamAlf(String original) {
        // Optimization: Single pass replacement if needed, but keeping logic consistent
        String str = original.replace("wa", "}").replace("Wa", "}")
                 .replace(">a", "}").replace("wA", "}")
                 .replace("WA", "}").replace(">A", "}");
        
        return str.replace("\"", "\"" + "\\");
    }

    /**
     * Reverses the text while keeping number segments intact for RTL display.
     */
    private static String reverse(String input) {
        if (input == null) return "";
        
        // Split into parts of non-digits and digits
        String[] parts = input.split("(?<=\\D)(?=\\d)|(?<=\\d)(?=\\D)");
        StringBuilder sb = new StringBuilder();
        
        for (int z = parts.length - 1; z >= 0; z--) {
            String part = parts[z];
            if (!part.isEmpty() && !Character.isDigit(part.charAt(0))) {
                // Reverse only non-digit segments (letters/symbols)
                sb.append(new StringBuilder(part).reverse());
            } else {
                // Keep numbers in their original order
                sb.append(part);
            }
        }
        return sb.toString();
    }

    private static class GlyphData {
        char isolated, initial, medial, finalForm;
        int connectionType;

        GlyphData(char isolated, char initial, char medial, char finalForm, int type) {
            this.isolated = isolated;
            this.initial = initial;
            this.medial = medial;
            this.finalForm = finalForm;
            this.connectionType = type;
        }
    }

    private static final Map<Character, GlyphData> GLYPH_MAP = new HashMap<>();

    static {
        // Arabic Digits (Standard and Arabic-Indic)
        addGlyph('0', '0', '0', '0', '0', 2);
        addGlyph('1', '1', '1', '1', '1', 2);
        addGlyph('2', '2', '2', '2', '2', 2);
        addGlyph('3', '3', '3', '3', '3', 2);
        addGlyph('4', '4', '4', '4', '4', 2);
        addGlyph('5', '5', '5', '5', '5', 2);
        addGlyph('6', '6', '6', '6', '6', 2);
        addGlyph('7', '7', '7', '7', '7', 2);
        addGlyph('8', '8', '8', '8', '8', 2);
        addGlyph('9', '9', '9', '9', '9', 2);
        addGlyph('٠', '0', '0', '0', '0', 2);
        addGlyph('١', '1', '1', '1', '1', 2);
        addGlyph('٢', '2', '2', '2', '2', 2);
        addGlyph('٣', '3', '3', '3', '3', 2);
        addGlyph('٤', '4', '4', '4', '4', 2);
        addGlyph('٥', '5', '5', '5', '5', 2);
        addGlyph('٦', '6', '6', '6', '6', 2);
        addGlyph('٧', '7', '7', '7', '7', 2);
        addGlyph('٨', '8', '8', '8', '8', 2);
        addGlyph('٩', '9', '9', '9', '9', 2);

        // Symbols
        addGlyph(']', ']', ']', ']', ']', 2);
        addGlyph('[', '[', '[', '[', '[', 2);
        addGlyph('|', '|', '|', '|', '|', 2);
        addGlyph(':', ':', ':', ':', ':', 2);

        // Arabic Alphabet
        addGlyph('ء', '\'', '\'', '\'', '\'', 2);
        addGlyph('آ', 'A', 'A', 'a', 'a', 2);
        addGlyph('أ', 'A', 'A', 'a', 'a', 2);
        addGlyph('ؤ', ',', ',', ',', ',', 2);
        addGlyph('إ', 'A', 'A', 'a', 'a', 2);
        addGlyph('ئ', '{', '{', '{', '{', 4);
        addGlyph('ا', 'A', 'A', 'a', 'a', 2);
        addGlyph('ب', '!', 'B', 'b', '!', 4);
        addGlyph('ة', '_', 'Z', 'z', '_', 2);
        addGlyph('ت', '\"', 'C', 'c', '\"', 4);
        addGlyph('ث', '#', 'D', 'd', '#', 4);
        addGlyph('ج', '$', 'E', 'e', '$', 4);
        addGlyph('ح', '%', 'F', 'f', '%', 4);
        addGlyph('خ', '&', 'G', 'g', '&', 4);
        addGlyph('د', 'H', 'H', 'H', 'H', 2);
        addGlyph('ذ', 'I', 'I', 'I', 'I', 2);
        addGlyph('ر', 'J', 'J', 'J', 'J', 2);
        addGlyph('ز', 'K', 'K', 'K', 'K', 2);
        addGlyph('س', '(', 'L', 'l', '(', 4);
        addGlyph('ش', ')', 'M', 'm', ')', 4);
        addGlyph('ص', '*', 'N', 'n', '*', 4);
        addGlyph('ض', '+', 'O', 'o', '+', 4);
        addGlyph('ط', 'P', 'P', 'p', 'P', 4);
        addGlyph('ظ', 'Q', 'Q', 'q', 'Q', 4);
        addGlyph('ع', '/', 'R', 'r', '/', 4);
        addGlyph('غ', '`', 'S', 's', '`', 4);
        addGlyph('ف', ';', 'T', 't', ';', 4);
        addGlyph('ق', '<', 'U', 'u', '<', 4);
        addGlyph('ك', '=', 'V', 'v', '=', 4);
        addGlyph('ل', '>', 'W', 'w', '>', 4);
        addGlyph('م', '?', 'X', 'x', '?', 4);
        addGlyph('ن', '^', 'Y', 'y', '^', 4);
        addGlyph('ه', '_', 'Z', 'z', '_', 4);
        addGlyph('و', 'h', 'h', 'h', 'h', 2);
        addGlyph('ى', 'i', 'j', 'k', 'i', 2);
        addGlyph('ي', 'i', 'j', 'k', 'i', 4);
        
        // Special Forms
        addGlyph('ٱ', 'ٱ', 'ٱ', 'ﭑ', 'ﭑ', 2);
        addGlyph('ٮ', 'ﯤ', 'ﯨ', 'ﯩ', 'ﯥ', 4);
        addGlyph('ڪ', 'ﮎ', 'ﮐ', 'ﮑ', 'ﮏ', 4);
        addGlyph('ہ', 'ﮦ', 'ﮨ', 'ﮩ', 'ﮧ', 4);
        addGlyph('ۤ', 'ۤ', 'ۤ', 'ۤ', 'ﻮ', 2);
    }

    private static void addGlyph(char code, char isolated, char initial, char medial, char finalForm, int type) {
        GLYPH_MAP.put(code, new GlyphData(isolated, initial, medial, finalForm, type));
    }
}
