// ArabicShaper - UTF-8 logical Arabic -> shaped visual-order glyph codes.
// Mirrors tools/gen_font.py::shape() (reference implementation).
// See docs/FONT_SPEC.md section 5.
#ifndef ARABIC_SHAPER_H
#define ARABIC_SHAPER_H

#include <stddef.h>
#include <stdint.h>

namespace ArabicShaper {
// Shape utf8 into out (NUL-terminated). Output length <= input length.
// Returns number of bytes written (excluding NUL). Harakat/tatweel dropped,
// invalid sequences become '?', ASCII passes through untouched.
size_t shape(const char *utf8, char *out, size_t outSize);
} // namespace ArabicShaper

#endif
