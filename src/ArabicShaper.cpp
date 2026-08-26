#include "ArabicShaper.h"
#include "arabic_glyph_map.h"

namespace {

constexpr uint16_t LAM = 0x0644;

bool isTransparent(uint16_t cp) {
  return (cp >= 0x064B && cp <= 0x065F) || cp == 0x0670 || cp == 0x0640;
}

bool isAlefVariant(uint16_t cp) {
  return cp == 0x0627 || cp == 0x0623 || cp == 0x0625 || cp == 0x0622;
}

// Decode one UTF-8 codepoint; returns 0xFFFFFFFF on malformed input.
uint32_t nextCp(const char *&p) {
  uint8_t c = (uint8_t)*p++;
  if (c < 0x80)
    return c;
  if ((c & 0xE0) == 0xC0) {
    uint8_t c2 = (uint8_t)*p;
    if ((c2 & 0xC0) != 0x80)
      return 0xFFFFFFFF;
    ++p;
    return ((uint32_t)(c & 0x1F) << 6) | (c2 & 0x3F);
  }
  if ((c & 0xF0) == 0xE0) {
    uint8_t c2 = (uint8_t)*p, c3 = (uint8_t)p[1];
    if ((c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80)
      return 0xFFFFFFFF;
    p += 2;
    return ((uint32_t)(c & 0x0F) << 12) | ((uint32_t)(c2 & 0x3F) << 6) |
           (c3 & 0x3F);
  }
  if ((c & 0xF8) == 0xF0) {
    uint8_t c2 = (uint8_t)*p, c3 = (uint8_t)p[1], c4 = (uint8_t)p[2];
    if ((c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80 || (c4 & 0xC0) != 0x80)
      return 0xFFFFFFFF;
    p += 3;
    return ((uint32_t)(c & 0x07) << 18) | ((uint32_t)(c2 & 0x3F) << 12) |
           ((uint32_t)(c3 & 0x3F) << 6) | (c4 & 0x3F);
  }
  return 0xFFFFFFFF;
}

// Presentation forms (pre-shaped input) pass through as opaque glyphs.
bool presentationSlot(uint16_t cp, uint8_t &slot) {
  if (cp < 0xFE70 || cp > 0xFEFF)
    return false;
  return ArabicMap::lookupSlot(cp, slot);
}

enum FormIdx : uint8_t { F_ISO = 0, F_INI = 1, F_MED = 2, F_FIN = 3 };

} // namespace

size_t ArabicShaper::shape(const char *utf8, char *out, size_t outSize) {
  if (!out || outSize == 0)
    return 0;
  size_t o = 0;
  const char *p = utf8;
  bool prevJoinsLeft = false;

  if (utf8) {
    while (*p && o + 1 < outSize) {
      uint32_t cp = nextCp(p);
      if (cp == 0xFFFFFFFF) {
        out[o++] = '?';
        prevJoinsLeft = false;
        continue;
      }
      if (isTransparent(cp))
        continue;

      uint8_t slot;
      if (presentationSlot((uint16_t)cp, slot)) {
        out[o++] = (char)slot;
        prevJoinsLeft = false;
        continue;
      }

      if (cp == LAM) {
        const char *q = p;
        uint32_t nxt = nextCp(q);
        while (nxt != 0xFFFFFFFF && isTransparent((uint16_t)nxt) && *q)
          nxt = nextCp(q);
        if (isAlefVariant((uint16_t)nxt)) {
          for (uint8_t i = 0;
               i < sizeof(ArabicMap::LIGATURES) / sizeof(ArabicMap::LIGATURES[0]);
               ++i) {
            if (pgm_read_word(&ArabicMap::LIGATURES[i].pair[1]) == nxt) {
              slot = prevJoinsLeft ? pgm_read_byte(&ArabicMap::LIGATURES[i].fin)
                                   : pgm_read_byte(&ArabicMap::LIGATURES[i].iso);
              out[o++] = (char)slot;
              prevJoinsLeft = false;
              break;
            }
          }
          p = q;
          continue;
        }
      }

      const ArabicMap::FormRow *row =
          (cp >= 0x0621 && cp <= 0x064A) ? ArabicMap::formsByBase((uint16_t)cp)
                                         : nullptr;
      if (row) {
        bool jr = ArabicMap::joinsRight((uint16_t)cp);
        bool jl = ArabicMap::joinsLeft((uint16_t)cp);
        bool linkPrev = prevJoinsLeft && jr;
        const char *q = p;
        uint32_t nxt = nextCp(q);
        while (nxt != 0xFFFFFFFF && isTransparent((uint16_t)nxt) && *q)
          nxt = nextCp(q);
        bool nxtIsLetter = nxt >= 0x0621 && nxt <= 0x064A;
        bool linkNext = jl && nxtIsLetter && ArabicMap::joinsRight((uint16_t)nxt);
        uint8_t fi = linkPrev && linkNext ? F_MED
                     : linkPrev           ? F_FIN
                     : linkNext           ? F_INI
                                          : F_ISO;
        slot = pgm_read_byte(&row->f[fi]);
        if (!slot)
          slot = pgm_read_byte(&row->f[F_ISO]);
        out[o++] = (char)slot;
        prevJoinsLeft = jl;
        continue;
      }

      if (cp < 0x80) {
        out[o++] = (char)cp;
      } else {
        out[o++] = '?';
      }
      prevJoinsLeft = false;
    }
  }
  // Visual reorder: reverse whole buffer, then un-reverse ASCII runs.
  for (size_t i = 0; i < o / 2; ++i) {
    char t = out[i];
    out[i] = out[o - 1 - i];
    out[o - 1 - i] = t;
  }
  size_t i = 0;
  while (i < o) {
    if ((uint8_t)out[i] >= 0x80) {
      ++i;
      continue;
    }
    size_t j = i;
    while (j < o && (uint8_t)out[j] < 0x80)
      ++j;
    for (size_t a = i, b = j - 1; a < b; ++a, --b) {
      char t = out[a];
      out[a] = out[b];
      out[b] = t;
    }
    i = j;
  }
  out[o] = '\0';
  return o;
}
