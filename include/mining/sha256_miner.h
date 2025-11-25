// sha256_miner.h
// bitcoin mining core using esp32 hardware sha256 acceleration

#ifndef SHA256_MINER_H
#define SHA256_MINER_H

#include <Arduino.h>

// initialize hardware sha256 engine
// call this once during setup before any mining operations
void miner_init();

// compute double sha256 hash (sha256(sha256(data)))
// this is what bitcoin uses for block header hashing
// 
// parameters:
//   data: input data to hash
//   len: length of input data in bytes
//   output: buffer for result (must be 32 bytes)
void sha256d(const uint8_t* data, size_t len, uint8_t* output);

// compare hash to target accounting for little-endian byte order
// returns true if hash < target (valid share found)
//
// parameters:
//   hash: 32-byte hash result from mining
//   target: 32-byte difficulty target from pool
//
// bitcoin uses little-endian format where byte[0] is least significant
// comparison must start from most significant byte (index 31)
bool hash_below_target(const uint8_t* hash, const uint8_t* target);

// mine a range of nonces and check for valid shares
// modifies header bytes 76-79 in place with each nonce
//
// parameters:
//   header: 80-byte block header (will be modified)
//   start_nonce: first nonce value to try
//   nonce_count: how many sequential nonces to test
//   target: 32-byte difficulty target
//   found_nonce: output - set to winning nonce if found
//   hashes_done: output - set to number of hashes computed
//
// returns:
//   true if valid share found (found_nonce contains winning value)
//   false if no valid share in this range
bool mine_nonce_range(uint8_t* header,
                      uint32_t start_nonce,
                      uint32_t nonce_count,
                      const uint8_t* target,
                      uint32_t* found_nonce,
                      uint32_t* hashes_done);

#endif