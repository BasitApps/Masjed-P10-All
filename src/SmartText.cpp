/*
 * SmartText.cpp - Flicker-Free Text Engine for DMDESP
 */
#include "SmartText.h"
#include "ArabicShaper.h"

static inline bool englishLetter(unsigned char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static inline bool needsEnglishGap(unsigned char a, unsigned char b) {
  return englishLetter(a) && englishLetter(b);
}

static inline bool asciiDigit(unsigned char c) {
  return c >= '0' && c <= '9';
}

SmartText::SmartText(DMDESP &d, int16_t w)
    : disp(d), width(w), textW(0), scrollX(0), lastMs(0), flashMs(0), speed(50),
      flashSpeed(1000), areaY(0), areaH(16), drawY(0), isNew(true),
      flashVisible(true), shaping(true), rightToLeft(false), dirty(false) {
  text[0] = '\0';
  srcCache[0] = '\0';
}

bool SmartText::consumeDirty() {
  const bool changed = dirty;
  dirty = false;
  return changed;
}

void SmartText::reset() {
  text[0] = '\0';
  srcCache[0] = '\0';
  textW = scrollX = 0;
  isNew = true;
  flashVisible = true;
  flashMs = 0;
}

inline void SmartText::clearArea() {
  dirty = true;
  disp.drawFilledRect(0, areaY, width - 1, areaY + areaH - 1, Bitmap::Black);
}

void SmartText::drawTextFast(int x, int y, const char *str) {
  dirty = true;
  uint8_t charCount = 0;
  while (*str && x < width) {
    const unsigned char current = static_cast<unsigned char>(*str);
    int charWidth = disp.drawChar(x, y, *str);
    if (charWidth <= 0)
      charWidth = 1;
    x += charWidth;
    const unsigned char next = static_cast<unsigned char>(str[1]);
    if ((rightToLeft && needsEnglishGap(current, next)) ||
        (!rightToLeft && asciiDigit(current) && asciiDigit(next)))
    {
      disp.drawFilledRect(x, areaY, x, areaY + areaH - 1, Bitmap::Black);
      ++x;
    }
    ++str;
    if (++charCount >= 8) { // Yield every 8 characters
      yield();
      disp.loop();
      charCount = 0;
    }
  }
  if (charCount > 0)
    disp.loop(); // Final update
}

inline void SmartText::drawCentered() {
  drawTextFast((width - textW) >> 1, drawY, text);
}

bool SmartText::updateText(const char *src, int8_t y) {
  if (!src || !src[0]) {
    if (text[0]) {
      clearArea();
      reset();
    }
    return false;
  }
  drawY = y;
  if (strcmp(srcCache, src) != 0) {
    strncpy(srcCache, src, BUF_TEXT - 1);
    srcCache[BUF_TEXT - 1] = '\0';
    if (shaping) {
      ArabicShaper::shape(srcCache, text, BUF_TEXT);
    } else {
      strncpy(text, srcCache, BUF_TEXT - 1);
      text[BUF_TEXT - 1] = '\0';
    }
    textW = disp.textWidth(text);
    for (size_t i = 1; text[i]; ++i) {
      const unsigned char previous = static_cast<unsigned char>(text[i - 1]);
      const unsigned char current = static_cast<unsigned char>(text[i]);
      if ((rightToLeft && needsEnglishGap(previous, current)) ||
          (!rightToLeft && asciiDigit(previous) && asciiDigit(current)))
        ++textW;
    }
    isNew = true;
    clearArea();
    return true;
  }
  return false;
}

bool SmartText::draw(const char *src, int8_t y, unsigned long now) {
  if (updateText(src, y) || !src) {
    scrollX = rightToLeft ? width : -textW;
    lastMs = now;
    if (!src)
      return false;
  }

  // Static text - draw once
  if (textW <= width) {
    if (isNew) {
      drawCentered();
      isNew = false;
    }
    return false;
  }

  // Scrolling text
  if (isNew) {
    // First draw: clear area once, then draw
    clearArea();
    drawTextFast(scrollX, drawY, text);
    isNew = false;
    lastMs = now;
    return false;
  }

  if (now - lastMs < speed)
    return false;
  lastMs = now;

  if (rightToLeft) {
    if (--scrollX <= -textW) {
      scrollX = width;
      return true;
    }
  } else if (++scrollX >= width) {
    scrollX = -textW;
    return true;
  }

  // For scrolling: clear only the trailing edge (where text is scrolling out)
  // This is more efficient than clearing the entire area every frame
  int16_t clearX = rightToLeft ? scrollX + textW : scrollX - 1;
  if (clearX >= 0 && clearX < width) {
    // Clear the column that just scrolled out of view
    disp.drawFilledRect(clearX, areaY, clearX, areaY + areaH - 1,
                        Bitmap::Black);
  }

  drawTextFast(scrollX, drawY, text);
  return false;
}

bool SmartText::drawFlash(const char *src, int8_t y, unsigned long now) {
  updateText(src, y);
  if (!src || !src[0])
    return false;

  bool visible = ((now / 1000) % 2) == 0;

  if (visible != flashVisible) {
    flashVisible = visible;
    clearArea();
    if (flashVisible)
      drawCentered();
    isNew = false;
  } else if (isNew && flashVisible) {
    drawCentered();
    isNew = false;
  }
  return false;
}
