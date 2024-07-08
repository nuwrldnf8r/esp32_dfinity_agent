#ifndef EEPROM_VECTOR_STORAGE_H
#define EEPROM_VECTOR_STORAGE_H

#include <EEPROM.h>
#include <vector>

class EEPROMVectorStorage {
public:
    EEPROMVectorStorage(size_t eepromSize);
    void begin();
    void writeVector(uint8_t index, const std::vector<uint8_t>& vec);
    std::vector<uint8_t> readVector(uint8_t index);
    void clearEEPROM();

private:
    const size_t maxVectorSize = 100;
    const size_t headerSize = sizeof(uint16_t); // To store the size of the vector
    const size_t totalVectors = 4;
    size_t eepromSize;
    size_t vectorStartAddress(uint8_t index);

    void writeData(size_t address, const void* data, size_t len);
    void readData(size_t address, void* data, size_t len);
};

#endif
