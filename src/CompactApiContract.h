#ifndef COMPACT_API_CONTRACT_H
#define COMPACT_API_CONTRACT_H

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

namespace CompactApi
{
constexpr size_t BODY_SIZE = 512;

inline int errorStatus(const char *code)
{
  if (!strcmp(code, "storage") || !strcmp(code, "response_too_large")) return 500;
  if (!strcmp(code, "body_too_large")) return 413;
  if (!strcmp(code, "busy")) return 503;
  if (!strcmp(code, "bad_body") || !strcmp(code, "version")) return 400;
  return 422;
}

// Normalize in place: UTF-8 digits shrink to ASCII, so no extra buffer is needed.
inline void normalizeNumber(char *text)
{
  unsigned char *read = reinterpret_cast<unsigned char *>(text);
  char *write = text;
  while (*read)
  {
    const unsigned char base = *read == 0xD9 ? 0xA0 : 0xB0;
    if ((*read == 0xD9 || *read == 0xDB) &&
        read[1] >= base && read[1] <= base + 9)
    {
      *write++ = '0' + read[1] - base;
      read += 2;
    }
    else if (*read == 0xD9 && (read[1] == 0xAB || read[1] == 0xAC))
    {
      *write++ = '.';
      read += 2;
    }
    else
    {
      *write++ = *read == ',' ? '.' : *read;
      ++read;
    }
  }
  *write = '\0';
}

inline bool parseInteger(char *text, long minValue, long maxValue, long &value)
{
  normalizeNumber(text);
  char *end = nullptr;
  errno = 0;
  const long parsed = strtol(text, &end, 10);
  if (end == text || errno == ERANGE)
    return false;
  while (isspace(static_cast<unsigned char>(*end))) ++end;
  if (*end || parsed < minValue || parsed > maxValue)
    return false;
  value = parsed;
  return true;
}

inline bool parseDecimal(char *text, float minValue, float maxValue, float &value)
{
  normalizeNumber(text);
  char *end = nullptr;
  errno = 0;
  const double parsed = strtod(text, &end);
  if (end == text || errno == ERANGE || !isfinite(parsed))
    return false;
  while (isspace(static_cast<unsigned char>(*end))) ++end;
  if (*end || parsed < minValue || parsed > maxValue)
    return false;
  value = static_cast<float>(parsed);
  return true;
}

inline bool validBody(const char *body)
{
  if (!body[0])
    return true;
  // Keep room for descriptive API names such as daylightSavingTime (18 chars).
  // 20 bytes includes the terminating NUL and preserves the 16-key limit.
  char keys[16][20];
  uint8_t keyCount = 0;
  const char *line = body;
  while (*line)
  {
    const char *end = strchr(line, '\n');
    if (!end)
      end = line + strlen(line);
    const char *eq = (const char *)memchr(line, '=', end - line);
    size_t keyLen = eq ? (size_t)(eq - line) : 0;
    if (!eq || keyLen == 0 || keyLen >= sizeof(keys[0]) || keyCount >= 16)
      return false;
    for (size_t i = 0; i < keyLen; ++i)
      if (!isalnum((unsigned char)line[i]) && line[i] != '_')
        return false;
    for (uint8_t i = 0; i < keyCount; ++i)
      if (strlen(keys[i]) == keyLen && !memcmp(keys[i], line, keyLen))
        return false;
    memcpy(keys[keyCount], line, keyLen);
    keys[keyCount++][keyLen] = '\0';
    for (const char *p = eq + 1; p < end; ++p)
      if (*p == '\\' && (++p >= end || (*p != '\\' && *p != 'n')))
        return false;
    if (!*end)
      break;
    line = end + 1;
    if (!*line)
      return false;
  }
  return true;
}

// Validate the body first. 0=absent, 1=complete, -1=overflow.
inline int findValue(const char *body, const char *key, char *out, size_t outSize)
{
  const size_t wanted = strlen(key);
  const char *line = body;
  while (*line)
  {
    const char *end = strchr(line, '\n');
    if (!end)
      end = line + strlen(line);
    const char *eq = (const char *)memchr(line, '=', end - line);
    if (eq && (size_t)(eq - line) == wanted && !memcmp(line, key, wanted))
    {
      size_t len = 0;
      for (const char *p = eq + 1; p < end; ++p)
      {
        if (*p == '\\')
          ++p;
        ++len;
      }
      if (len >= outSize)
        return -1;
      len = 0;
      for (const char *p = eq + 1; p < end; ++p)
        out[len++] = *p == '\\' ? (*++p == 'n' ? '\n' : '\\') : *p;
      out[len] = '\0';
      return 1;
    }
    line = *end ? end + 1 : end;
  }
  return 0;
}

}
#endif
