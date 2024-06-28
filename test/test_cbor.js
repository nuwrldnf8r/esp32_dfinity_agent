import cbor from 'cbor';
import crypto from 'crypto';
import { Buffer } from 'buffer';

// Sample CBOR data
const cborData = {
    "content": {
        "ingress_expiry": 1719569498000000000,
        "sender": Buffer.from('BF37754D6190CEB9CEBA959924282DD9D6971ACEFE2022C7', 'hex'),
        "canister_id": Buffer.from('00000000014055480101', 'hex'),
        "request_type": "query",
        "method_name": "test_data",
        "arg": Buffer.from('4449444C0001710B68656C6C6F20776F726C64', 'hex')
    },
    "sender_pubkey": Buffer.from('88F462167DE229E342593A44DBD78F6551B0F119549B0E8BA60AE254CD2CC20BE567BE4A328FA3BC1E0AB2D60641EDB274BEFCA1D597D73816ED422AD4A44327', 'hex'),
    "sender_sig": Buffer.from('30440220F35059B987D63791F287F648BCD317BA6E7E066347AEE0966A3204CF844E93EF022053D81B88F68A672C1CAA84F240D67DFD6C18349567609105225E5FEF41D6EE1F', 'hex')
};

// Utility function to verify the signature
function verifySignature(pubKey, data, signature) {
    const verify = crypto.createVerify('SHA256');
    verify.update(data);
    verify.end();
    return verify.verify({ key: pubKey, format: 'der', type: 'spki' }, signature);
}

// Utility function to encode content to CBOR
function encodeContentToCBOR(content) {
    return cbor.encode(content);
}

// Test cases
async function runTests() {
    console.log('Running tests...');

    const chai = await import('chai');
    const { expect } = chai;

    // Test 1: Check structure
    try {
        expect(cborData).to.have.property('content');
        expect(cborData).to.have.property('sender_pubkey');
        expect(cborData).to.have.property('sender_sig');
        
        const content = cborData.content;
        expect(content).to.have.property('ingress_expiry');
        expect(content).to.have.property('sender');
        expect(content).to.have.property('canister_id');
        expect(content).to.have.property('request_type');
        expect(content).to.have.property('method_name');
        expect(content).to.have.property('arg');
        console.log('Test 1 passed: Structure is valid.');
    } catch (err) {
        console.error('Test 1 failed:', err.message);
    }

    // Test 2: Check request type
    try {
        expect(cborData.content.request_type).to.equal('query');
        console.log('Test 2 passed: Request type is valid.');
    } catch (err) {
        console.error('Test 2 failed:', err.message);
    }

    // Test 3: Check canister ID
    try {
        const canisterId = cborData.content.canister_id;
        expect(canisterId).to.be.instanceOf(Buffer);
        expect(canisterId.length).to.be.above(0);
        console.log('Test 3 passed: Canister ID is valid.');
    } catch (err) {
        console.error('Test 3 failed:', err.message);
    }

    // Test 4: Check sender
    try {
        const sender = cborData.content.sender;
        expect(sender).to.be.instanceOf(Buffer);
        expect(sender.length).to.be.above(0);
        console.log('Test 4 passed: Sender is valid.');
    } catch (err) {
        console.error('Test 4 failed:', err.message);
    }

    // Test 5: Verify the signature
    try {
        const content = cborData.content;
        const encodedContent = encodeContentToCBOR(content);
        const isValid = verifySignature(cborData.sender_pubkey, encodedContent, cborData.sender_sig);
        expect(isValid).to.be.true;
        console.log('Test 5 passed: Signature is valid.');
    } catch (err) {
        console.error('Test 5 failed:', err.message);
    }

    // Test 6: Decode the argument correctly
    try {
        const arg = cborData.content.arg;
        const decodedArg = cbor.decodeAllSync(arg)[0];
        // Verify the decoded argument
        expect(decodedArg).to.be.instanceOf(Buffer);
        expect(decodedArg.toString()).to.equal('DIDL\x00\x01q\x0Bhello world');
        console.log('Test 6 passed: Argument is decoded correctly.');
    } catch (err) {
        console.error('Test 6 failed:', err.message);
    }

    console.log('All tests completed.');
}

// Run the tests
runTests();
