/*
 * SmartText.cpp - Flicker-Free Text Engine for DMDESP
 */
#include "SmartText.h"
#include "ArabicShaper.h"
#include "TextSpacing.h"

SmartText::SmartText(DMDESP &d, int16_t w)
    : disp(d), width(w), textW(0), scrollX(0), lastMs(0), flashMs(0), speed(50),
      flashSpeed(1000), areaY(0), areaH(16), drawY(0), isNew(true),
      flashVisible(true), shaping(true), rightToLeft(false), dirty(false),
      preserveScrollOnUpdate(false), colonHidden(false) {
  text[0] = '\0';
  srcCache[0] = '\0';
}

bool SmartText::consumeDirty() {
  const bool changed = dirty;
  dirty = false;
  return changed;
}

void SmartText::reset() {
#if SMARTTEXT_PIXEL_SCROLL
  columnChar = nullptr;
#endif
  text[0] = '\0';
  srcCache[0] = '\0';
  textW = scrollX = 0;
  isNew = true;
  flashVisible = true;
  flashMs = 0;
  preserveScrollOnUpdate = false;
  colonHidden = false;
}

inline void SmartText::clearArea() {
  dirty = true;
#if SMARTTEXT_PIXEL_SCROLL
  if (width > 0 && width == disp.width() && !(width & 7) && areaY >= 0 &&
      areaH > 0 && areaY + areaH <= disp.height()) {
    // The framebuffer uses 1 for black; clear only complete rows in this area.
    memset(disp.data() + areaY * disp.stride(), 0xFF, areaH * disp.stride());
    return;
  }
#endif
  disp.drawFilledRect(0, areaY, width - 1, areaY + areaH - 1, Bitmap::Black);
}

void SmartText::drawTextFast(int x, int y, const char *str) {
  dirty = true;
  uint8_t charCount = 0;
  while (*str && x < width) {
    const unsigned char current = static_cast<unsigned char>(*str);
#if SMARTTEXT_PIXEL_SCROLL
    int charWidth = disp.charWidth(*str);
    if (charWidth > 0 && x + charWidth > 0)
      charWidth = disp.drawChar(x, y, *str);
#else
    int charWidth = disp.drawChar(x, y, *str);
#endif
    if (charWidth <= 0)
      charWidth = 1;
    x += charWidth;
    const unsigned char next = static_cast<unsigned char>(str[1]);
    if (needsTextGap(current, next))
    {
      if (x >= 0 && x < width)
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

int16_t SmartText::colonX() const {
  const char *colon = strchr(text, ':');
  if (!colon)
    return INT16_MIN;
  int16_t x = textW <= width ? (width - textW) >> 1 : scrollX;
  for (const char *p = text; p < colon; ++p) {
    int glyphWidth = disp.charWidth(*p);
    if (glyphWidth <= 0)
      glyphWidth = 1;
    x += glyphWidth;
    if (needsTextGap(static_cast<unsigned char>(*p),
                     static_cast<unsigned char>(p[1])))
      ++x;
  }
  return x;
}

void SmartText::applyColon(bool hide) {
  const int16_t x = colonX();
  if (x == INT16_MIN)
    return;
  int colonWidth = disp.charWidth(':');
  if (colonWidth <= 0)
    colonWidth = 1;
  if (hide) {
    const int16_t first = x > 0 ? x : 0;
    const int16_t last = x + colonWidth - 1 < width
                             ? x + colonWidth - 1
                             : width - 1;
    if (first <= last)
      disp.drawFilledRect(first, drawY, last, drawY + disp.textHeight() - 1,
                          Bitmap::Black);
  } else {
    disp.drawChar(x, drawY, ':');
  }
  dirty = true;
}

#if SMARTTEXT_PIXEL_SCROLL
bool SmartText::shiftTextPixels() {
  if (width <= 0 || width != disp.width() || (width & 7) || areaY < 0 ||
      areaH <= 0 || areaY + areaH > disp.height() || !disp.font() ||
      drawY < areaY || drawY + disp.textHeight() > areaY + areaH)
    return false;
  const int stride = disp.stride();
  // Packed bits are inverted: insert 1 (black) at the uncovered edge.
  for (int y = areaY; y < areaY + areaH; ++y) {
    uint8_t *row = disp.data() + y * stride;
    if (rightToLeft) {
      for (int x = 0; x < stride; ++x)
        row[x] = (row[x] << 1) | (x + 1 < stride ? row[x + 1] >> 7 : 1);
    } else {
      for (int x = stride - 1; x >= 0; --x)
        row[x] = (row[x] >> 1) | (x ? row[x - 1] << 7 : 0x80);
    }
  }
  dirty = true;
  drawIncomingColumn(rightToLeft ? width - 1 : 0);
  disp.loop();
  return true;
}

void SmartText::drawIncomingColumn(int16_t x) {
  int16_t column = x - scrollX;
  if (column < 0 || column >= textW)
    return;
  const uint8_t *font = static_cast<const uint8_t *>(disp.font());
  if (!columnChar || columnFont != font ||
      column < columnStart || column >= columnEnd) {
    columnFont = font;
    columnChar = nullptr;
    columnGlyph = nullptr;
    int16_t start = 0;
    for (const char *p = text; *p; ++p) {
      int glyphWidth = disp.charWidth(*p);
      if (glyphWidth <= 0)
        glyphWidth = 1;
      const unsigned char current = static_cast<unsigned char>(*p);
      const unsigned char next = static_cast<unsigned char>(p[1]);
      const bool gap = needsTextGap(current, next);
      const int16_t end = start + glyphWidth + gap;
      if (column < end) {
        columnChar = p;
        columnStart = start;
        columnEnd = end;
        columnWidth = glyphWidth;
        columnHeight = pgm_read_byte(font + 3);
        columnPages = (columnHeight + 7) >> 3;
        const uint8_t first = pgm_read_byte(font + 4);
        const uint8_t count = pgm_read_byte(font + 5);
        if (*p != ' ' && current >= first && current < first + count) {
          const uint8_t index = current - first;
          columnGlyph = font + 6;
          if (!pgm_read_byte(font) && !pgm_read_byte(font + 1))
            columnGlyph += index * columnPages * columnWidth;
          else {
            columnGlyph += count;
            for (uint8_t i = 0; i < index; ++i)
              columnGlyph += pgm_read_byte(font + 6 + i) * columnPages;
          }
        }
        break;
      }
      start = end;
    }
  }
  if (!columnChar)
    return;
  column -= columnStart;
  if (column >= columnWidth)
    return;
  if (*columnChar == ' ') {
    disp.drawFilledRect(x, drawY, x, drawY + columnHeight - 1, !disp.textColor());
    return;
  }
  if (!columnGlyph)
    return;
  // Match Bitmap's bottom-aligned final page for fonts taller than 8px.
  for (uint8_t page = 0; page < columnPages; ++page) {
    uint8_t bits = pgm_read_byte(columnGlyph + page * columnWidth + column);
    const int row = columnPages > 1 && page == columnPages - 1
                        ? columnHeight - 8 : page * 8;
    for (uint8_t bit = 0; bit < 8; ++bit) {
      if (row + bit >= page * 8 && row + bit <= columnHeight)
        disp.setPixel(x, drawY + row + bit,
                      bits & 1 ? disp.textColor() : !disp.textColor());
      bits >>= 1;
    }
  }
}
#endif

bool SmartText::updateText(const char *src, int8_t y) {
  preserveScrollOnUpdate = false;
  if (!src || !src[0]) {
    if (text[0]) {
      clearArea();
      reset();
    }
    return false;
  }
  drawY = y;
  if (strcmp(srcCache, src) != 0) {
    const bool wasScrolling = !isNew && textW > width;
    const int16_t oldTextW = textW;
    const int16_t oldScrollX = scrollX;
#if SMARTTEXT_PIXEL_SCROLL
    columnChar = nullptr;
#endif
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
      if (needsTextGap(previous, current))
        ++textW;
    }
    if (wasScrolling && textW > width) {
      // Keep the same relative point in the scroll cycle when a countdown
      // changes (for example 13 -> 12) instead of restarting off-screen.
      const int32_t oldCycle = static_cast<int32_t>(width) + oldTextW;
      const int32_t newCycle = static_cast<int32_t>(width) + textW;
      int32_t travelled = rightToLeft ? width - oldScrollX
                                     : oldScrollX + oldTextW;
      if (travelled < 0)
        travelled = 0;
      else if (travelled > oldCycle)
        travelled = oldCycle;
      const int32_t newTravelled =
          (travelled * newCycle + oldCycle / 2) / oldCycle;
      scrollX = rightToLeft ? width - newTravelled
                            : newTravelled - textW;
      preserveScrollOnUpdate = true;
    }
    isNew = true;
    clearArea();
    return true;
  }
  return false;
}

bool SmartText::draw(const char *src, int8_t y, unsigned long now) {
  if (updateText(src, y) || !src) {
    if (!preserveScrollOnUpdate)
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
#if !SMARTTEXT_PIXEL_SCROLL
    clearArea();
#endif
    // updateText already cleared the new text's area.
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

#if SMARTTEXT_PIXEL_SCROLL
  if (shiftTextPixels())
    return false;
#endif

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

bool SmartText::drawClock(const char *src, int8_t y, unsigned long now,
                          bool hideColon) {
  const bool wasHidden = colonHidden;
  const int16_t oldColonX = colonX();
  const bool done = draw(src, y, now);
  if (textW > width) {
    // The clock colon blinks only while the complete clock fits statically.
    colonHidden = false;
    return done;
  }
  const int16_t newColonX = colonX();

  if (hideColon) {
    if (!wasHidden || dirty || oldColonX != newColonX)
      applyColon(true);
  } else if (wasHidden && !dirty) {
    applyColon(false);
  }
  colonHidden = hideColon;
  return done;
}

bool SmartText::drawFlash(const char *src, int8_t y, unsigned long now) {
  if (updateText(src, y)) {
    if (!preserveScrollOnUpdate)
      scrollX = rightToLeft ? width : -textW;
    lastMs = now;
  }
  if (!src || !src[0])
    return false;

  // Long alerts scroll continuously so blinking cannot hide their letters.
  if (textW > width)
    return draw(src, y, now);

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
