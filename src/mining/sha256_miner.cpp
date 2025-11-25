// sha256_miner.cpp
// bitcoin mining core using esp32 hardware sha256 acceleration

#include "mining/sha256_miner.h"

#include "mbedtls/sha256.h"  // esp32 hardware-accelerated sha256

// initialize hardware sha256 engine
void miner_init() {
  // esp32 mbedtls automatically uses hardware acceleration
  // no explicit initialization required for the sha256 peripheral
  // this function exists for future setup needs and startup confirmation

  Serial.println("[miner] hardware sha256 ready");
}

// compute double sha256 hash (sha256(sha256(data)))
// bitcoin uses double hashing for block headers and transactions
void sha256d(const uint8_t* data, size_t len, uint8_t* output) {
  // temporary buffer holds result of first hash
  uint8_t first_hash[32];

  // context structure holds internal sha256 computation state
  // mbedtls uses this pattern: init -> starts -> update -> finish -> free
  mbedtls_sha256_context ctx;

  // ----- first sha256 pass: hash the input data -----

  // initialize context to clean state
  mbedtls_sha256_init(&ctx);

  // begin sha256 computation
  // second parameter: 0 = sha256, 1 = sha224
  mbedtls_sha256_starts(&ctx, 0);

  // feed input data into the hash computation
  mbedtls_sha256_update(&ctx, data, len);

  // finalize computation and write 32-byte result
  mbedtls_sha256_finish(&ctx, first_hash);

  // release any resources held by context
  mbedtls_sha256_free(&ctx);

  // ----- second sha256 pass: hash the first result -----

  // reinitialize context for second hash operation
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts(&ctx, 0);

  // input is now the 32-byte result from first hash
  mbedtls_sha256_update(&ctx, first_hash, 32);

  // write final double-hashed result to output buffer
  mbedtls_sha256_finish(&ctx, output);

  // cleanup
  mbedtls_sha256_free(&ctx);
}

// compare hash to target with little-endian byte order
// returns true if hash < target (valid share)
bool hash_below_target(const uint8_t* hash, const uint8_t* target) {
  // bitcoin stores 256-bit numbers in little-endian format
  // byte[0] = least significant, byte[31] = most significant
  // to compare two numbers numerically we must start from the most significant byte and work down

  for (int i = 31; i >= 0; i--) {
    if (hash[i] < target[i]) {
      // hash is smaller at most significant differing byte means hash < target numerically
      return true;
    }
    if (hash[i] > target[i]) {
      // hash is larger at most significant differing byte means hash > target numerically
      return false;
    }
    // bytes are equal, continue to next less significant byte
  }

  // all 32 bytes are identical, hash == target
  // this counts as valid since we want hash <= target
  return true;
}

// mine a range of nonces looking for valid shares
// this is the core mining loop that tests candidate solutions
bool mine_nonce_range(uint8_t* header,
                      uint32_t start_nonce,
                      uint32_t nonce_count,
                      const uint8_t* target,
                      uint32_t* found_nonce,
                      uint32_t* hashes_done) {
  // buffer for hash output
  uint8_t hash[32];

  // iterate through assigned nonce range
  for (uint32_t i = 0; i < nonce_count; i++) {
    uint32_t nonce = start_nonce + i;

    // write nonce into header bytes 76-79 in little-endian format
    // bitcoin block header structure places nonce at end (bytes 76-79)
    // little-endian means least significant byte at lowest address
    header[76] = (nonce >> 0) & 0xFF;   // bits 0-7 (lsb)
    header[77] = (nonce >> 8) & 0xFF;   // bits 8-15
    header[78] = (nonce >> 16) & 0xFF;  // bits 16-23
    header[79] = (nonce >> 24) & 0xFF;  // bits 24-31 (msb)

    // compute double sha256 of complete 80-byte block header
    sha256d(header, 80, hash);

    // check if hash meets difficulty target
    if (hash_below_target(hash, target)) {
      // found valid share - hash is below target difficulty
      *found_nonce = nonce;
      *hashes_done = i + 1;  // report actual work done
      return true;
    }
  }

  // exhausted nonce range without finding valid share
  *hashes_done = nonce_count;
  return false;
}