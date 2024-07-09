#include "candid.h"
#include <algorithm>
#include "utils.h"

const std::vector<uint8_t> didl_prefix = {'D', 'I', 'D', 'L'};
const std::vector<uint8_t> didl_empty = {0, 0};


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
    Utils::leb128_encode(value);
}

Parameter::Parameter(const std::string& value) {
    printf("text\n");
    _value.clear();
    _type = "text";
    _value.push_back(0x71);
    std::vector<uint8_t> encoded_size = Utils::leb128_encode(value.size());
    _value.insert(_value.end(), encoded_size.begin(), encoded_size.end());
    _value.insert(_value.end(), value.begin(), value.end());
}

Parameter::Parameter(const std::vector<uint8_t>& value) {
    _value.clear();
    _type = "blob";
    _value.push_back(0x68);
    std::vector<uint8_t> encoded_size = Utils::leb128_encode(value.size());
    _value.insert(_value.end(), encoded_size.begin(), encoded_size.end());
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
    std::vector<Parameter> result;

    // Ensure the prefix matches
    for (size_t i = 0; i < didl_prefix.size(); ++i) {
        if (readByte() != didl_prefix[i]) {
            throw std::runtime_error("Invalid Candid prefix");
        }
    }

    uint8_t num_types = readByte();  // This should be the number of type descriptions
    if (num_types != 0x00) {
        throw std::runtime_error("Invalid number of type descriptions");
    }

    uint8_t num_values = readByte();  // This should be the number of values
    for (uint8_t i = 0; i < num_values; ++i) {
        uint8_t type = readByte();

        switch (type) {
            case 0x71:  // Text type
            {
                uint64_t length = leb128Decode();
                std::string value;
                for (uint64_t j = 0; j < length; ++j) {
                    value += char(readByte());
                }
                result.push_back(Parameter(value));
                break;
            }
            case 0x7c:  // Int type
            {
                int64_t value = leb128Decode();
                result.push_back(Parameter(value));
                break;
            }
            case 0x7d:  // Bool type
            {
                bool value = readByte() != 0x00;
                result.push_back(Parameter(value));
                break;
            }
            case 0x7f:  // Blob type
            {
                uint64_t length = leb128Decode();
                std::vector<uint8_t> value(length);
                for (uint64_t j = 0; j < length; ++j) {
                    value[j] = readByte();
                }
                result.push_back(Parameter(value));
                break;
            }
            default:
                throw std::runtime_error("Unknown Candid type");
        }
    }

    return result;
}



std::vector<uint8_t> Candid::encode(const std::vector<Parameter>& args) {
    if(args.empty()) {
        return encodeEmpty();
    }
    std::vector<uint8_t> result = didl_prefix;
    //count number of types
    std::vector<std::string> types;
    for (const Parameter& arg : args) {
        auto idx = std::find(types.begin(), types.end(), arg.getType());
        if (idx == types.end()) {
            types.push_back(arg.getType());
        }
    }
    result.push_back(types.size()-1);
    result.push_back(args.size());


    for (const Parameter& arg : args) {
        printf("Encoding type: %s\n", arg.getType().c_str());
        std::vector<uint8_t> value_ = arg.getValue(); // Copy the value to avoid const issues
        result.insert(result.end(), value_.begin(), value_.end());
    }
    return result;
}


std::vector<uint8_t> Candid::encodeEmpty() {
    std::vector<uint8_t> result = didl_prefix;
    result.insert(result.end(), didl_empty.begin(), didl_empty.end());
    return result;
}   