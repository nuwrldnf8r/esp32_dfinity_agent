#ifndef CANDID_H
#define CANDID_H

#include <vector>
#include <string>
#include <cstdint>
#include <iostream>
#include <sstream>

class Parameter {
public:
    Parameter();
    Parameter(const bool value);
    Parameter(const int64_t value);
    Parameter(const std::string& value);
    Parameter(const std::vector<uint8_t>& value); 
    const std::vector<uint8_t>& getValue() const { return _value; }
    std::string getType() const { return _type; }
    bool parseBool() const;
    int64_t parseInt() const;
    std::string parseText() const;
    std::vector<uint8_t> parseBlob() const;
private:
    std::vector<uint8_t> _value;
    std::string _type;
};

class Candid {
public:
    Candid();
    Candid(const std::vector<uint8_t>& data);
    Candid(const std::vector<Parameter>& args);
    std::string decodeText();
    int64_t decodeInt();
    bool decodeBool();
    std::vector<uint8_t> decodeBlob();
    std::vector<Parameter> decode();
    std::vector<uint8_t> encode() const { return data_; }
    std::vector<uint8_t> encode(const std::vector<Parameter>& args);
    std::vector<uint8_t> encodeEmpty();

private:
    std::vector<uint8_t> data_;
    size_t position_;
    uint8_t readByte();
    int64_t leb128Decode();
};

#endif // CANDID_H
