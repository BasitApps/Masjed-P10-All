/*
 * constants.h - System Constants
 * Centralized configuration for easy tuning
 */
#ifndef CONSTANTS_H
#define CONSTANTS_H

// ============ DEBUG ============
#define DEBUG 0

// ============ VERSION ============
#define EEPROM_VERSION 3
#define MIN_VALID_YEAR 2025

// ============ LANGUAGE ============
#define LANG_AR 0
#define LANG_EN 1

// ============ DISPLAY ============
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 16
#define DISPLAYS_WIDE 4
#define DISPLAYS_HIGH 1
#define SMALL_FONT_Y_POS 8

// ============ TIMING (ms) ============
#define MS_SECOND 1000UL
#define MS_RTC_SYNC 300000UL
#define MS_SPLASH 10000UL
#define MS_RESTART_DELAY 2000UL
#define MS_WIFI_YIELD 50UL
#define MS_CARD_DURATION 5000UL
#define MS_WIFI_TIMEOUT 600000UL // 10 minutes

// ============ BUFFERS ============
// UTF-8 Arabic: shaped output <= logical input length.
// BUF_SMALL: time display "H : MM : SS" (~12 chars) + headroom
#define BUF_SMALL 48
// BUF_LARGE: runText - must hold config.message (240) + headroom
#define BUF_LARGE 256
// BUF_TEXT: SmartText internal (logical cache + shaped), matches message
#define BUF_TEXT 256

// ============ WIFI ============
#define AP_SSID_PREFIX "Basit-Masjed-"
#define AP_PASSWORD "12881288"

// ============ CARDS ============
#define CARD_COUNT 12

// ============ TEST MODE ============
#define TEST_BRIGHTNESS 1

// ============ DEBUG MACROS ============
#if DEBUG
#define DBG(x) Serial.print(x)
#define DBGLN(x) Serial.println(x)
#else
#define DBG(x)
#define DBGLN(x)
#endif

#endif
