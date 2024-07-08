#include "eeprom_vector_storage.h"

EEPROMVectorStorage::EEPROMVectorStorage(size_t eepromSize) : eepromSize(eepromSize) {}

void EEPROMVectorStorage::begin() {
    EEPROM.begin(eepromSize);
}

void EEPROMVectorStorage::writeVector(uint8_t index, const std::vector<uint8_t>& vec) {
    if (index >= totalVectors || vec.size() > maxVectorSize) return;

    uint16_t size = vec.size();
    size_t address = vectorStartAddress(index);

    writeData(address, &size, headerSize);
    writeData(address + headerSize, vec.data(), size);
    EEPROM.commit();
}

std::vector<uint8_t> EEPROMVectorStorage::readVector(uint8_t index) {
    std::vector<uint8_t> vec;
    if (index >= totalVectors) return vec;

    uint16_t size;
    size_t address = vectorStartAddress(index);

    readData(address, &size, headerSize);
    if (size > maxVectorSize) size = maxVectorSize;

    vec.resize(size);
    readData(address + headerSize, vec.data(), size);

    return vec;
}

size_t EEPROMVectorStorage::vectorStartAddress(uint8_t index) {
    return index * (headerSize + maxVectorSize);
}

void EEPROMVectorStorage::writeData(size_t address, const void* data, size_t len) {
    const uint8_t* byteData = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < len; ++i) {
        EEPROM.write(address + i, byteData[i]);
    }
}

void EEPROMVectorStorage::readData(size_t address, void* data, size_t len) {
    uint8_t* byteData = static_cast<uint8_t*>(data);
    for (size_t i = 0; i < len; ++i) {
        byteData[i] = EEPROM.read(address + i);
    }
}

void EEPROMVectorStorage::clearEEPROM() {
    for (size_t i = 0; i < eepromSize; ++i) {
        EEPROM.write(i, 0);
    }
    EEPROM.commit();
}
