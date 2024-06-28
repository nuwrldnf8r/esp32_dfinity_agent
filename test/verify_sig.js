import cbor from 'cbor';
import crypto from 'crypto';
import { Buffer } from 'buffer';

// CBOR data
const cborData = {
    "content": {
        "ingress_expiry": 1719571002000000000,
        "sender": Buffer.from('DA03995A96353C5D73C34BCA2989D378CB568CE5D598C19A', 'hex'),
        "canister_id": Buffer.from('00000000014055480101', 'hex'),
        "request_type": 'query',
        "method_name": 'test_data',
        "arg": Buffer.from('4449444C0001710B68656C6C6F20776F726C64', 'hex')
    },
    "sender_pubkey": Buffer.from('DDB3749CA986EA381409EC23AF8D5561FD5D0A9797E9AADD256B0C1F770179AC1A570B584C73793EA32C68E20C308B92189A7E765090CBA90F3FEBB48DFCE14F', 'hex'),
    "sender_sig": Buffer.from('304402201404D9AF8FEB7E6DA1FDDE2773698284D5293DC6C28546B08E581C01ECA0C15102204C1470D24A504EB067565FA425728004ADF307D9EAA13EA9C9C4FA713656E344', 'hex')
};

// Convert raw public key to DER encoded SPKI format
function convertToSPKI(rawPublicKey) {
    const spkiHeader = Buffer.from([
        0x30, 0x59, // SEQUENCE, length 89
        0x30, 0x13, // SEQUENCE, length 19
        0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02, 0x01, // OID 1.2.840.10045.2.1 (ecPublicKey)
        0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07, // OID 1.2.840.10045.3.1.7 (prime256v1 curve)
        0x03, 0x42, // BIT STRING, length 66
        0x00 // no unused bits
    ]);
    return Buffer.concat([spkiHeader, Buffer.from([0x04]), rawPublicKey]);
}

// Utility function to verify the signature
function verifySignature(pubKey, data, signature) {
    try {
        const spkiPubKey = convertToSPKI(pubKey);
        const verify = crypto.createVerify('SHA256');
        verify.update(data);
        verify.end();
        return verify.verify({ key: spkiPubKey, format: 'der', type: 'spki' }, signature);
    } catch (error) {
        console.error('Error during verification:', error);
        return false;
    }
}

// Encode the content to CBOR
const encodedContent = cbor.encode(cborData.content);

// Verify the signature
const isValidSignature = verifySignature(cborData.sender_pubkey, encodedContent, cborData.sender_sig);

console.log(`Is the signature valid? ${isValidSignature}`);
