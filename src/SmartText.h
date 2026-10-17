/*
 * SmartText.h - Optimized Text Engine for DMDESP
 */
#ifndef SMART_TEXT_H
#define SMART_TEXT_H

#include "constants.h"
#include <Arduino.h>
#include <DMDESP.h>

class SmartText {
public:
  SmartText(DMDESP &d, int16_t w);

  bool draw(const char *text, int8_t y, unsigned long now);
  bool drawFlash(const char *text, int8_t y, unsigned long now);

  void setSpeed(uint16_t ms) { speed = ms ? ms : 50; }

  void setArea(int8_t y, int8_t h) {
    areaY = y;
    areaH = h;
  }
  void setShaping(bool on) {
    if (shaping != on) {
      shaping = on;
      reset();
    }
  }
  void setRightToLeft(bool on) {
    if (rightToLeft != on) {
      rightToLeft = on;
      reset();
    }
  }
  void reset();
  bool consumeDirty();

  bool isStatic() const { return textW <= width; }

private:
  bool updateText(const char *src, int8_t y);
  void drawTextFast(int x, int y, const char *str);
  void clearArea();
  void drawCentered();
#if SMARTTEXT_PIXEL_SCROLL
  bool shiftTextPixels();
  void drawIncomingColumn(int16_t x);
  const char *columnChar = nullptr;
  const uint8_t *columnFont = nullptr, *columnGlyph = nullptr;
  int16_t columnStart = 0, columnEnd = 0;
  uint8_t columnWidth = 0, columnHeight = 0, columnPages = 0;
#endif

  DMDESP &disp;
  int16_t width, textW, scrollX;
  unsigned long lastMs, flashMs;
  uint16_t speed, flashSpeed;
  int8_t areaY, areaH, drawY;
  char text[BUF_TEXT];
  char srcCache[BUF_TEXT];
  bool isNew, flashVisible, shaping, rightToLeft, dirty,
      preserveScrollOnUpdate;
};

#endif
