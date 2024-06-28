import crypto from 'crypto';
import base32 from 'base32.js';

// Utility functions
function sha256(data) {
    return crypto.createHash('sha256').update(data).digest();
}

function leb128Encode(n) {
    const result = [];
    do {
        let byte = n & 0x7F;
        n >>= 7;
        if (n !== 0) {
            byte |= 0x80;
        }
        result.push(byte);
    } while (n !== 0);
    return Buffer.from(result);
}

function cborHash(value) {
    if (typeof value === 'string') {
        return sha256(Buffer.from(value));
    } else if (Buffer.isBuffer(value)) {
        return sha256(value);
    } else if (typeof value === 'number') {
        return sha256(leb128Encode(value));
    } else if (Array.isArray(value)) {
        const concatenated = Buffer.concat(value.map(item => cborHash(item)));
        return sha256(concatenated);
    } else if (typeof value === 'object' && value !== null) {
        const hashPairs = Object.entries(value).map(([key, val]) => {
            return Buffer.concat([cborHash(Buffer.from(key)), cborHash(val)]);
        });
        hashPairs.sort(Buffer.compare);
        const concatenated = Buffer.concat(hashPairs);
        return sha256(concatenated);
    }
    throw new Error('Unsupported value type for CBOR hash');
}

// Simulate `uncook` function for sender field
function uncook(input) {
    
    const decoded = base32.decode.asBytes(input.replace(/-/g, ''));
    return decoded.slice(4); // Discard the first 4 bytes (CRC)
}

// Generate request ID
function generateRequestId(content) {
    const contentMap = {};

    // Fill contentMap with the hashed key-value pairs of the content
    contentMap[cborHash('ingress_expiry')] = cborHash(content.ingress_expiry);
    contentMap[cborHash('sender')] = cborHash(content.sender === '' ? Buffer.from([0x04]) : uncook(content.sender));
    contentMap[cborHash('canister_id')] = cborHash(content.canister_id);
    contentMap[cborHash('request_type')] = cborHash(content.request_type);
    contentMap[cborHash('method_name')] = cborHash(content.method_name);
    contentMap[cborHash('arg')] = cborHash(content.arg);

    // Compute the hash of the entire content map
    return cborHash(contentMap);
}

// Example content
const content = {
    ingress_expiry: 1719571002000000000,
    sender: 'da03995a96353c5d73c34bca2989d378cb568ce5d598c19a',
    canister_id: '00000000014055480101',
    request_type: 'query',
    method_name: 'test_data',
    arg: Buffer.from('4449444C0001710B68656C6C6F20776F726C64', 'hex')
};

// Generate request ID
const requestId = generateRequestId(content);
console.log('Request ID:', requestId.toString('hex'));
