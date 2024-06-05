#include "agent.h"

#define MAX_BYTES 8
#define MAX_CBOR_SIZE 9

/*
{"content":
  { "ingress_expiry" : 12345678901234567890
  , "nonce": h'01'
  , "sender":h'04'
  , "canister_id":h'1234'
  , "request_type":"query"
  , "method_name":"function"
  , "arg":'parameter'
  }
  , sender_pubkey (blob)
  , sender_delegation (blob)//can be left out
  , sender_sig (blob)
  }

    A public key can authenticate a principal if the latter is a self-authenticating id derived from that public key (see Special forms of Principals).

    The fields sender_pubkey, sender_sig, and sender_delegation must be omitted if the sender field is the anonymous principal. The fields sender_pubkey and sender_sig must be set if the sender field is not the anonymous principal.
    https://internetcomputer.org/docs/current/references/ic-interface-spec/#authentication
*/

//constexpr size_t PrincipalSize = 29; // Define the size of the Principal
//using IcpPrincipal = std::array<uint8_t, PrincipalSize>;

//uint8_t* dfinityAgent::CBOREncode(uint64_t ingress_expiry,const IcpPrincipal& sender,const IcpPrincipal& canister_id,method_name,arg) {

//}


uint8_t* dfinityAgent::createQuery(canister_id,method_name,arg) {

}

/*
uint8_t* dfinityAgent::createQuery(sender,canister_id,method_name,arg) {

}


void dfinityAgent::createUpdate(sender,canister_id,method_name,arg) {

}
*/







