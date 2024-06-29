import crypto from 'crypto';
import asn1 from 'asn1.js';

// Convert hex string to buffer
function hexToBuffer(hex) {
  return Buffer.from(hex, 'hex');
}

//3056301006072a8648ce3d020106052b8104000a0342000404a311bb4aabb1a214bd223446f1a6f351770d89a10d39a6dfc0dfa71afd01f702723f84df3732ddf37338904cb56eb20fda4591734461889ced23cbafbd012c06
//3056301006072a8648ce3d020106052b8104000a0342000404a311bb4aabb1a214bd223446f1a6f351770d89a10d39a6dfc0dfa71afd01f702723f84df3732ddf37338904cb56eb20fda4591734461889ced23cbafbd012c06
// Your data
const message = 'd3433bfd9e7f090e8d382a34c29697bc709657499b8203cdbd48ae92bdd92924';
const signature = '9a6a3dc00ccf26a9854874f902bb501b47e58814dd7b86a7651aed67a995a042a711709e1dc481e91fed6e025ee2d050d0eeb2eb857586179a0bbfc6c1a414e1';
const derPublicKey = '3057301006072a8648ce3d020106052b8104000a0343000404238997eb79f91d179b520e2dc24f14e2c51987e0e7727b4311a2abc77f9415c9f84b6e051ba596b54f0f8ca897939f49a92194db01e1cc14b0178624fe040eb6';
//                   '3057301006072a8648ce3d020106052b8104000a03420004049cd0d27879304522ab0195ec55c49202bdf7c7132315c9edd751eea836f04181e570a4cae403ed7a2a7215a2934ad2ef17819bbe3bd569ded2bfa389a9679d4c'

const actualPublicKey =                                              '238997eb79f91d179b520e2dc24f14e2c51987e0e7727b4311a2abc77f9415c9f84b6e051ba596b54f0f8ca897939f49a92194db01e1cc14b0178624fe040eb6'

// Convert inputs to buffers
const messageBuffer = hexToBuffer(message);
const signatureBuffer = hexToBuffer(signature);
const publicKeyBuffer = hexToBuffer(derPublicKey);
const actualPublicKeyBuffer = hexToBuffer(actualPublicKey);

export const SECP256K1_OID = Uint8Array.from([
    [0x30, 0x10], // SEQUENCE
    [0x06, 0x07], // OID with 7 bytes
    [0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02, 0x01], // OID ECDSA
    [0x06, 0x05], // OID with 5 bytes
    [0x2b, 0x81, 0x04, 0x00, 0x0a], // OID secp256k1
]);

const encodeLenBytes = (len) => {
    if (len <= 0x7f) {
      return 1;
    } else if (len <= 0xff) {
      return 2;
    } else if (len <= 0xffff) {
      return 3;
    } else if (len <= 0xffffff) {
      return 4;
    } else {
      throw new Error('Length too long (> 4 bytes)');
    }
};

const encodeLen = (buf, offset, len) => {
    if (len <= 0x7f) {
      buf[offset] = len;
      return 1;
    } else if (len <= 0xff) {
      buf[offset] = 0x81;
      buf[offset + 1] = len;
      return 2;
    } else if (len <= 0xffff) {
      buf[offset] = 0x82;
      buf[offset + 1] = len >> 8;
      buf[offset + 2] = len;
      return 3;
    } else if (len <= 0xffffff) {
      buf[offset] = 0x83;
      buf[offset + 1] = len >> 16;
      buf[offset + 2] = len >> 8;
      buf[offset + 3] = len;
      return 4;
    } else {
      throw new Error('Length too long (> 4 bytes)');
    }
  };

function wrapDER(payload, oid) {
    // The Bit String header needs to include the unused bit count byte in its length
    const bitStringHeaderLength = 2 + encodeLenBytes(payload.byteLength + 1);
    const len = oid.byteLength + bitStringHeaderLength + payload.byteLength;
    let offset = 0;
    const buf = new Uint8Array(1 + encodeLenBytes(len) + len);
    // Sequence
    buf[offset++] = 0x30;
    // Sequence Length
    offset += encodeLen(buf, offset, len);
  
    // OID
    buf.set(oid, offset);
    offset += oid.byteLength;
  
    // Bit String Header
    buf[offset++] = 0x03;
    offset += encodeLen(buf, offset, payload.byteLength + 1);
    // 0 padding
    buf[offset++] = 0x00;
    buf.set(new Uint8Array(payload), offset);
  
    return buf;
  }


  function ToHexString(uint8Array) {
    return Array.from(uint8Array, byte => {
      return byte.toString(16).padStart(2, '0');
    }).join('');
  }

  function run () {
    let der = wrapDER(actualPublicKeyBuffer, SECP256K1_OID)
    console.log("Created:")
    console.log(ToHexString(der))
    console.log("Expected:")
    console.log(derPublicKey)
    console.log(ToHexString(der)===derPublicKey)
  }

  run()

//3057301006072a8648ce3d020106052b8104000a03430004049cd0d27879304522ab0195ec55c49202bdf7c7132315c9edd751eea836f04181e570a4cae403ed7a2a7215a2934ad2ef17819bbe3bd569ded2bfa389a9679d4c
//                                                049cd0d27879304522ab0195ec55c49202bdf7c7132315c9edd751eea836f04181e570a4cae403ed7a2a7215a2934ad2ef17819bbe3bd569ded2bfa389a9679d4c