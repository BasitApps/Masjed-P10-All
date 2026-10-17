#ifndef CLOCK_STORAGE_CONTRACT_H
#define CLOCK_STORAGE_CONTRACT_H

#include <LittleFS.h>
#include <flash_hal.h>

namespace ClockStorage
{
inline bool partitionErased()
{
  const uint32_t start = FS_PHYS_ADDR, size = FS_PHYS_SIZE;
  if (!size || size % 256 || start > ESP.getFlashChipRealSize() ||
      size > ESP.getFlashChipRealSize() - start)
    return false;
  uint32_t words[64];
  for (uint32_t offset = 0; offset < size; offset += sizeof(words))
  {
    if (!ESP.flashRead(start + offset, words, sizeof(words))) return false;
    for (uint32_t word : words)
      if (word != UINT32_MAX) return false;
    yield();
  }
  return true;
}

inline bool begin()
{
  LittleFS.setConfig(LittleFSConfig(false));
  if (LittleFS.begin()) return true;
  // Only an entirely erased partition is a new unit, not damaged user data.
  return partitionErased() && LittleFS.format() && LittleFS.begin();
}
}
#endif
