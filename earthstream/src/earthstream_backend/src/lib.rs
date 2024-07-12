use std::cell::RefCell;
use std::collections::BTreeMap;
use std::vec::Vec;
use sha2::{Digest, Sha256};
use ic_cdk::caller;
use std::string::String;
use candid::Principal;
use ic_cdk::api;
use candid::CandidType;
use serde::Deserialize;
use libsecp256k1::*;
//use util::make_unsafe_rng;



//use secp256k1::{Secp256k1, PublicKey, Message};


static FIELD_TEMP:u8 = 0x01;
static FIELD_HUMIDITY:u8 = 0x02;
static FIELD_LIGHT_INTENSITY:u8 = 0x03;
static FIELD_LATITUDE:u8 = 0x04;
static FIELD_LONGITUDE:u8 = 0x05;
static FIELD_ALTITUDE:u8 = 0x06;
static FIELD_PUBLIC_KEY:u8 = 0x07;
static FIELD_SIGNATURE:u8 = 0x08;

#[derive(CandidType, Deserialize)]
struct Data{
    ts: u64,
    data: String,
}

#[derive(CandidType, Deserialize)]
struct ParsedData{
    ts: u64,
    temp: String,
    humidity: String,
    light_intensity: String,
    latitude: String,
    longitude: String,
    altitude: String,
    public_key: String,
    signature: String,
}

struct SensorData {
    owner: Principal,
    data: Vec<u8>,
}



thread_local! {
    static GATEWAY_OWNER_MAP: RefCell<BTreeMap<u128, Vec<Vec<u8>>>> = RefCell::new(BTreeMap::new());
    static GATEWAY_MAP: RefCell<BTreeMap<u128, Vec<u8>>> = RefCell::new(BTreeMap::new());
    static DATA_MAP: RefCell<BTreeMap<u64, SensorData>> = RefCell::new(BTreeMap::new());
}

#[ic_cdk_macros::init]
fn init() {
    ic_cdk::setup();
}

fn hex_to_string(byte_data: &[u8]) -> String{
    let byte_data = byte_data.to_vec();
    match String::from_utf8(byte_data){
        Ok(s) => s,
        Err(_) => panic!("Invalid UTF-8 sequence")
    }
}

#[ic_cdk::query]
fn parse_data(ts: u64, data: String) -> ParsedData {
    let mut map: BTreeMap<u8,String> = BTreeMap::new();
    let _data = hex::decode(data).unwrap();
    let _data = _data[4..].to_vec();
    let mut i = 0;
    while i < _data.len() {
        let field = _data[i];
        let ln = _data[i+1];
        if field==FIELD_SIGNATURE || field==FIELD_PUBLIC_KEY {
            let value = hex::encode(&_data[i+2..i+2+ln as usize]);
            map.insert(field, value);
        } else if field==FIELD_TEMP || 
            field==FIELD_HUMIDITY || field==FIELD_LIGHT_INTENSITY || 
            field==FIELD_LATITUDE || field==FIELD_LONGITUDE || field==FIELD_ALTITUDE{
            let value = hex_to_string(&_data[i+2..i+2+ln as usize]);
            map.insert(field, value);
        } else {
            panic!("Invalid field");
        }
        i += 2+ln as usize;
    }
    
    return ParsedData{
        ts:ts, 
        temp:           map.get(&FIELD_TEMP).unwrap().clone(), 
        humidity:       map.get(&FIELD_HUMIDITY).unwrap().clone(), 
        light_intensity:map.get(&FIELD_LIGHT_INTENSITY).unwrap().clone(), 
        latitude:       map.get(&FIELD_LATITUDE).unwrap().clone(),    
        longitude:      map.get(&FIELD_LONGITUDE).unwrap().clone(), 
        altitude:       map.get(&FIELD_ALTITUDE).unwrap().clone(), 
        public_key:     map.get(&FIELD_PUBLIC_KEY).unwrap().clone(), 
        signature:      map.get(&FIELD_SIGNATURE).unwrap().clone()
    };
}

fn get_id(blob: &[u8]) -> u128 {
    let mut hasher = Sha256::new();
    hasher.update(blob);
    let hash_result = hasher.finalize();
    let mut result = 0u128;
    for i in 0..16 {
        result |= (hash_result[i] as u128) << (8 * i);
    }
    result
}

#[ic_cdk::update]
fn register(owner : String) {
    let gateway = caller();
    let gateway_id = get_id(gateway.as_slice());
    let owner_principal: Principal;    
    match Principal::from_text(owner.clone()){
        Ok(op) => owner_principal = op,
        Err(_) => panic!("Owner is an invalid Principal.")
    }
    let owner_id = get_id(owner.as_bytes());
    GATEWAY_OWNER_MAP.with(|map| {
        let mut map_mut = map.borrow_mut();
        let owner_gateways = map_mut.entry(owner_id).or_insert(Vec::new());
        owner_gateways.push(gateway.as_slice().to_vec());
    });
    GATEWAY_MAP.with(|map| {
        let mut map_mut = map.borrow_mut();
        map_mut.insert(gateway_id, owner_principal.as_slice().to_vec());
    });
}

#[ic_cdk::query]
fn is_registered() -> bool {
    let gateway = caller();
    let gateway_id = get_id(gateway.as_slice());
    GATEWAY_MAP.with(|map| {
        let map = map.borrow();
        map.contains_key(&gateway_id)
    })
}   

#[ic_cdk::query]
fn get_owner() -> String {
    let gateway = caller();
    let gateway_id = get_id(gateway.as_slice());
    GATEWAY_MAP.with(|map| {
        let map = map.borrow();
        match map.get(&gateway_id) {
            Some(owner) => Principal::from_slice(owner).to_text(),
            None => panic!("Gateway not registered")
        }   
    })
}


#[ic_cdk::query]
fn check_valid(data: String) -> bool{
    let data = hex::decode(data).unwrap();
    return is_valid(data);
}

fn check_sig(_public_key: &[u8], _signature: &[u8], _data: &[u8]) -> bool {

    let mut pub_key = [0u8; 65];
    pub_key[0] = 0x04;
    let mut msg: [u8; 32] = [0; 32];
    let mut sig: [u8; 64] = [0; 64];
    pub_key[1..].copy_from_slice(_public_key);
    msg.copy_from_slice(_data);
    sig.copy_from_slice(_signature);
    let pubkey: PublicKey = PublicKey::parse(&pub_key).unwrap();
    let message: Message = Message::parse(&msg);
    let sig: Signature = Signature::parse_standard(&sig).unwrap();
    libsecp256k1::verify(&message, &sig, &pubkey)
    

    /*
    TODO: Implement random: https://docs.rs/getrandom/latest/getrandom/fn.getrandom.html
    https://docs.rs/getrandom/latest/getrandom/#webassembly-support - see custom implementations
    https://docs.rs/libsecp256k1/latest/libsecp256k1/index.html
     */
    //true
}

fn is_valid(data: Vec<u8>) -> bool{
    //get first 4 bytes
    let chk = &data[0..4];
    //calculate checksum (first 4 bytes of sha256 of the rest of the data)
    let mut hasher = Sha256::new();
    hasher.update(&data[4..]);
    let hash_result = hasher.finalize();
    if chk != &hash_result[0..4] {
        return false;
    }
    
    //get signature 
    let data = data[4..].to_vec();
    if data[0]!=FIELD_SIGNATURE {
        panic!("Signature must be first field");
    }
    let ln = data[1];
    let sig = &data[2..2+ln as usize];
    let data = &data[2+ln as usize..];
    if data[0]!=FIELD_PUBLIC_KEY {
        panic!("Public key must be second field");
    }

    let ln = data[1];
    let ln = ln as usize;

    let public_key = &data[2..2+ln];

    //get message to validate with signature and DER public key
    let mut hasher = Sha256::new();
    hasher.update(data);
    let hash_result = hasher.finalize();

    if !check_sig(public_key, sig, &hash_result) {
        return false;
    }
    
    return true;
}

fn get_timestamp() -> u64 {
    let duration = std::time::Duration::from_nanos(api::time());
    duration.as_secs()
}

#[ic_cdk::update]
fn upload_data(data: String){
    if !check_valid(data.clone()){
        panic!("Invalid data");
    }
    
    let ts = get_timestamp();
    let data = hex::decode(data).unwrap();
    let sensor_data = SensorData {
        owner: caller(),
        data: data,
    };
    DATA_MAP.with(|map| {
        let mut map_mut = map.borrow_mut();
        map_mut.insert(ts, sensor_data);
    });
}

#[ic_cdk::query]
fn get_all_data() -> std::vec::Vec<Data>  {
    let mut result = Vec::new();
    DATA_MAP.with(|map| {
        let map = map.borrow();
        for (ts, data) in map.iter() {
            let data_hex = hex::encode(data.data.clone());
            result.push(Data{ts:ts.clone(),data:data_hex});
        }
    });
    result
}   

#[ic_cdk::query]
fn get_data(from: u64) -> std::vec::Vec<Data>  {
    let mut result = Vec::new();
    let to = get_timestamp();
    DATA_MAP.with(|map| {
        let map = map.borrow();
        for (ts, data) in map.range(from..=to) {
            let data_hex = hex::encode(data.data.clone());
            result.push(Data{ts:ts.clone(),data:data_hex});
        }
    });
    result
}

#[ic_cdk::query]
fn get_parsed_data(from: u64) -> std::vec::Vec<ParsedData>  {
    let mut result = Vec::new();
    let to = get_timestamp();
    DATA_MAP.with(|map| {
        let map = map.borrow();
        for (ts, data) in map.range(from..=to) {
            let data_hex = hex::encode(data.data.clone());
            let parsed_data = parse_data(ts.clone(), data_hex);
            result.push(parsed_data);
        }
    });
    result
}

#[ic_cdk::query]
fn get_data_by_owner(owner: Principal, from: u64) -> std::vec::Vec<Data>  {
    let mut result = Vec::new();
    let to = get_timestamp();
    DATA_MAP.with(|map| {
        let map = map.borrow();
        for (ts, data) in map.range(from..to) {
            
            if data.owner != owner {
                continue;
            }

            let data_hex = hex::encode(data.data.clone());
            result.push(Data{ts:ts.clone(),data:data_hex});
        }
    });
    result
}

