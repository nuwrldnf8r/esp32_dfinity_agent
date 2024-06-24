const { HttpAgent, Actor } = require('@dfinity/agent');
const { Principal } = require('@dfinity/principal');
const cbor = require('cbor');
const nodeFetch = require('node-fetch');

// Replace with your canister ID
const canisterId = 'trr4d-jiaaa-aaaak-akvea-cai';

// Define the IDL of the canister
const idlFactory = ({ IDL }) => {
  return IDL.Service({
    'get_data': IDL.Func([], [IDL.Text], ['query']),
  });
};

// Save the original fetch method
const originalFetch = global.fetch;

// Override the fetch method
global.fetch = async (input, init) => {
  if (init && init.body) {
    // Convert the Uint8Array to a Buffer
    const buffer = Buffer.from(init.body);
    // Decode the CBOR data
    const decodedData = cbor.decode(buffer);
    // Print out the CBOR data being sent
    console.log('CBOR data sent:', decodedData);
  }

  const response = await originalFetch(input, init);

  return response;
};

const agent = new HttpAgent({ host: 'https://ic0.app' });

const canisterActor = Actor.createActor(idlFactory, {
  agent,
  canisterId: Principal.fromText(canisterId),
});

// Function to call the get_data method
async function callGetData() {
  try {
    // Call the get_data method
    const result = await canisterActor.get_data();

    // Log the result
    console.log('Result:', result);
  } catch (error) {
    console.error('Error calling get_data:', error);
  }
}

// Call the function
callGetData();