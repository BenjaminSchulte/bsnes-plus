// -----------------------------------------------------------------------------
int64_t MemoryContext::readBits(uint32_t address, uint32_t numBits, bool isSigned) const {
  if (!SNES::cartridge.loaded()) {
    return 0;
  }

  SNES::debugger.bus_access = true;
  uint64_t result = 0;
  uint32_t currentShift = 0;
  uint32_t bitsRemain = numBits;

  while (bitsRemain > 0) {
    uint32_t thisBits = bitsRemain > 8 ? 8 : bitsRemain;
    result |= SNES::debugger.read(memorySource(), address++) << currentShift;
    currentShift += 8;
    bitsRemain -= thisBits;
  }

  if (isSigned && (result & (1 << (numBits - 1)))) {
    result |= 0xFFFFFFFFFFFFFFFFl << numBits;
  }
  
  SNES::debugger.bus_access = false;

  return result;
}

// -----------------------------------------------------------------------------
int64_t MemoryFlagsContext::readBits(uint32_t readAddress, uint32_t readBits, bool isSigned) const {
  if (!SNES::cartridge.loaded()) {
    return 0;
  }

  //qDebug() << "readAddress" << readAddress;

  uint32_t thisOffset = offset;
  uint32_t readSize = (bits + 7 / 8) * 8;
  while (thisOffset >= 8) {
    thisOffset -= 8;
    readAddress++;
  }

  //qDebug() << "readAddress" << readAddress;
  //qDebug() << "offset" << offset;
  //qDebug() << "thisOffset" << thisOffset;
  uint64_t originalBitMask = (~(0xFFFFFFFFFFFFFFFFl << bits));
  //qDebug() << "originalBitMask" << originalBitMask;
  int64_t readValue = MemoryContext::readBits(readAddress, readSize, false);
  //qDebug() << "readValue" << readValue;
  bool isNegative = readValue < 0;
  uint64_t absValue = isNegative ? -readValue : readValue;
  //qDebug() << "absValue" << absValue;
  uint64_t calcValue = (absValue >> thisOffset) & originalBitMask;
  //qDebug() << "calcValue" << calcValue;
  int64_t result = calcValue;
  
  if (isNegative) {
    result = -result;
  }

  return result;
}

// -----------------------------------------------------------------------------
MemoryVariablePtr MemoryStructContext::resolveMember(const QString &id, uint32_t) {
  return currentStruct->resolveMember(id, sharedFromThis(), address);
}
