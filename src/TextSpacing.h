#ifndef TEXT_SPACING_H
#define TEXT_SPACING_H

inline bool needsTextGap(unsigned char a, unsigned char b)
{
  const bool letters = ((a >= 'A' && a <= 'Z') || (a >= 'a' && a <= 'z')) &&
                       ((b >= 'A' && b <= 'Z') || (b >= 'a' && b <= 'z'));
  return letters || (a >= '0' && a <= '9' && b >= '0' && b <= '9');
}

#endif
