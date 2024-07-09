#ifndef RESPONSE_H
#define RESPONSE_H


#include <string>
#include <vector>
#include <cstdint>


class Signature {
public:
    uint64_t timestamp;
    std::string signature;
    std::string identity;
};

class Reply {
public:
    std::vector<uint8_t> arg;
};

class Response {
public:
    Response();
    Response(const std::vector<uint8_t>& data);
    std::string status;
    Reply reply;
    std::vector<Signature> signatures;
};

#endif // RESPONSE_H