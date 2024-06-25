#include "candid.h"

const std::vector<uint8_t> didl_prefix = {'D', 'I', 'D', 'L'};
const std::vector<uint8_t> didl_empty = {0, 0};

void Parameter::encodeLeb128(int64_t value) {
    bool more = true;
    while (more) {
        uint8_t byte = value & 0x7F;
        value >>= 7;
        if ((value == 0 && !(byte & 0x40)) || (value == -1 && (byte & 0x40))) {
            more = false;
        } else {
            byte |= 0x80;
        }
        _value.push_back(byte);
    }
}

Parameter::Parameter() {
    _value.clear();
    _value = didl_empty;
    _type = "empty";
}

Parameter::Parameter(const bool value) {
    printf("bool\n");
    _value.clear();
    _type = "bool";
    _value.push_back(0x7d);
    _value.push_back(value ? 0x01 : 0x00);
}

Parameter::Parameter(const int64_t value) {
    _value.clear();
    _type = "int";
    _value.push_back(0x7c);
    encodeLeb128(value);
}

Parameter::Parameter(const std::string& value) {
    printf("text\n");
    _value.clear();
    _type = "text";
    _value.push_back(0x71);
    encodeLeb128(value.size());
    _value.insert(_value.end(), value.begin(), value.end());
}

Parameter::Parameter(const std::vector<uint8_t>& value) {
    _value.clear();
    _type = "blob";
    _value.push_back(0x68);
    encodeLeb128(value.size());
    _value.insert(_value.end(), value.begin(), value.end());
}

bool Parameter::parseBool() const {
    if (_type != "bool" || _value.size() != 2 || _value[1] > 0x01) {
        throw std::runtime_error("Invalid boolean parameter");
    }
    return _value[1] == 0x01;
}

int64_t Parameter::parseInt() const {
    if (_type != "int") {
        throw std::runtime_error("Invalid integer parameter");
    }
    size_t pos = 1; // Skip type indicator
    int64_t result = 0;
    int shift = 0;
    uint8_t byte;
    do {
        byte = _value[pos++];
        result |= (int64_t(byte & 0x7F) << shift);
        shift += 7;
    } while (byte & 0x80);
    return result;
}

std::string Parameter::parseText() const {
    if (_type != "text") {
        throw std::runtime_error("Invalid text parameter");
    }
    size_t pos = 1; // Skip type indicator
    int64_t length = 0;
    int shift = 0;
    uint8_t byte;
    do {
        byte = _value[pos++];
        length |= (int64_t(byte & 0x7F) << shift);
        shift += 7;
    } while (byte & 0x80);
    return std::string(_value.begin() + pos, _value.begin() + pos + length);
}

std::vector<uint8_t> Parameter::parseBlob() const {
    if (_type != "blob") {
        throw std::runtime_error("Invalid blob parameter");
    }
    size_t pos = 1; // Skip type indicator
    int64_t length = 0;
    int shift = 0;
    uint8_t byte;
    do {
        byte = _value[pos++];
        length |= (int64_t(byte & 0x7F) << shift);
        shift += 7;
    } while (byte & 0x80);
    return std::vector<uint8_t>(_value.begin() + pos, _value.begin() + pos + length);
}

Candid::Candid() : position_(0) {}

Candid::Candid(const std::vector<uint8_t>& data) : data_(data), position_(0) {}

Candid::Candid(const std::vector<Parameter>& args){
    data_ = encode(args);
    position_ = 0;
}

uint8_t Candid::readByte() {
    if (position_ >= data_.size()) {
        throw std::runtime_error("Out of bounds read");
    }
    return data_[position_++];
}

int64_t Candid::leb128Decode() {
    int64_t result = 0;
    int shift = 0;
    uint8_t byte;
    do {
        byte = readByte();
        result |= (int64_t(byte & 0x7F) << shift);
        shift += 7;
    } while (byte & 0x80);
    return result;
}

std::vector<Parameter> Candid::decode() {
    std::vector<Parameter> parameters;
    if (data_.size() < 4 || std::string(data_.begin(), data_.begin() + 4) != "DIDL") {
        throw std::runtime_error("Invalid Candid encoding");
    }
    position_ += 4; // Skip DIDL prefix

    while (position_ < data_.size()) {
        uint8_t type = readByte();
        switch (type) {
            case 0x7d: // bool
                parameters.push_back(Parameter(readByte() == 0x01));
                break;
            case 0x7c: // int
                parameters.push_back(Parameter(leb128Decode()));
                break;
            case 0x71: // text
                {
                    int64_t length = leb128Decode();
                    std::string value;
                    for (int64_t i = 0; i < length; ++i) {
                        value += char(readByte());
                    }
                    parameters.push_back(Parameter(value));
                }
                break;
            case 0x68: // blob
                {
                    int64_t length = leb128Decode();
                    std::vector<uint8_t> value(length);
                    for (int64_t i = 0; i < length; ++i) {
                        value[i] = readByte();
                    }
                    parameters.push_back(Parameter(value));
                }
                break;
            default:
                throw std::runtime_error("Unknown type indicator");
        }
    }
    return parameters;
}

std::vector<uint8_t> Candid::encode(const std::vector<Parameter>& args) {
    std::vector<uint8_t> result = didl_prefix;
    for (const Parameter& arg : args) {
        printf("Encoding type: %s\n", arg.getType().c_str());
        std::vector<uint8_t> value_ = arg.getValue(); // Copy the value to avoid const issues
        result.insert(result.end(), value_.begin(), value_.end());
    }
    return result;
}
