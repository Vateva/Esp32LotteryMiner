// main.cpp
// test program for sha256_miner module

#include <Arduino.h>
#include "mining/sha256_miner.h"

// helper function to print 32 bytes as hex string
void print_hash(const uint8_t* hash) {
  for (int i = 0; i < 32; i++) {
    if (hash[i] < 0x10) {
      Serial.print("0");  // leading zero for single digit hex
    }
    Serial.print(hash[i], HEX);
  }
  Serial.println();
}

// helper function to compare two 32-byte arrays
// returns true if they match exactly
bool arrays_match(const uint8_t* a, const uint8_t* b, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (a[i] != b[i]) {
      return false;
    }
  }
  return true;
}

// test 1: verify sha256d produces correct output
// uses known test vector: sha256d("abc")
bool test_sha256d() {
  Serial.println("\n[test] sha256d with input 'abc'");
  
  // input: "abc" as bytes
  uint8_t input[] = {0x61, 0x62, 0x63};
  
  // expected output: sha256(sha256("abc"))
  // sha256("abc") = ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
  // sha256(that) = 4f8b42c22dd3729b519ba6f68d2da7cc5b2d606d05daed5ad5128cc03e6c6358
  uint8_t expected[] = {
    0x4f, 0x8b, 0x42, 0xc2, 0x2d, 0xd3, 0x72, 0x9b,
    0x51, 0x9b, 0xa6, 0xf6, 0x8d, 0x2d, 0xa7, 0xcc,
    0x5b, 0x2d, 0x60, 0x6d, 0x05, 0xda, 0xed, 0x5a,
    0xd5, 0x12, 0x8c, 0xc0, 0x3e, 0x6c, 0x63, 0x58
  };
  
  // compute sha256d
  uint8_t output[32];
  sha256d(input, 3, output);
  
  // print results
  Serial.print("  expected: ");
  print_hash(expected);
  Serial.print("  got:      ");
  print_hash(output);
  
  // verify match
  if (arrays_match(output, expected, 32)) {
    Serial.println("  [pass] sha256d output matches expected");
    return true;
  } else {
    Serial.println("  [fail] sha256d output does not match!");
    return false;
  }
}

// test 2: verify hash_below_target comparison logic
bool test_hash_below_target() {
  Serial.println("\n[test] hash_below_target comparison");
  
  bool all_passed = true;
  
  // test case 2a: hash all zeros, target all ones -> hash < target (true)
  {
    uint8_t hash[32] = {0};    // all zeros
    uint8_t target[32];
    memset(target, 0xFF, 32);  // all ones
    
    bool result = hash_below_target(hash, target);
    Serial.print("  case 2a (zeros < ones): ");
    if (result == true) {
      Serial.println("[pass]");
    } else {
      Serial.println("[fail] expected true");
      all_passed = false;
    }
  }
  
  // test case 2b: hash all ones, target all zeros -> hash > target (false)
  {
    uint8_t hash[32];
    memset(hash, 0xFF, 32);    // all ones
    uint8_t target[32] = {0};  // all zeros
    
    bool result = hash_below_target(hash, target);
    Serial.print("  case 2b (ones > zeros): ");
    if (result == false) {
      Serial.println("[pass]");
    } else {
      Serial.println("[fail] expected false");
      all_passed = false;
    }
  }
  
  // test case 2c: hash equals target -> should return true (hash <= target)
  {
    uint8_t hash[32];
    uint8_t target[32];
    memset(hash, 0x55, 32);
    memset(target, 0x55, 32);
    
    bool result = hash_below_target(hash, target);
    Serial.print("  case 2c (equal values): ");
    if (result == true) {
      Serial.println("[pass]");
    } else {
      Serial.println("[fail] expected true");
      all_passed = false;
    }
  }
  
  // test case 2d: tricky endianness case
  // hash[31] = 0x01 (msb), target[31] = 0x02 (msb)
  // hash[0] = 0xFF (lsb), target[0] = 0x00 (lsb)
  // hash < target because msb determines result
  {
    uint8_t hash[32] = {0};
    uint8_t target[32] = {0};
    
    hash[31] = 0x01;    // most significant byte
    hash[0] = 0xFF;     // least significant byte (should be ignored)
    
    target[31] = 0x02;  // most significant byte
    target[0] = 0x00;   // least significant byte
    
    bool result = hash_below_target(hash, target);
    Serial.print("  case 2d (msb determines): ");
    if (result == true) {
      Serial.println("[pass]");
    } else {
      Serial.println("[fail] expected true (hash[31]=0x01 < target[31]=0x02)");
      all_passed = false;
    }
  }
  
  // test case 2e: opposite of 2d - hash msb > target msb
  {
    uint8_t hash[32] = {0};
    uint8_t target[32] = {0};
    
    hash[31] = 0x02;    // most significant byte
    hash[0] = 0x00;     // least significant byte
    
    target[31] = 0x01;  // most significant byte
    target[0] = 0xFF;   // least significant byte (should be ignored)
    
    bool result = hash_below_target(hash, target);
    Serial.print("  case 2e (msb determines, hash > target): ");
    if (result == false) {
      Serial.println("[pass]");
    } else {
      Serial.println("[fail] expected false (hash[31]=0x02 > target[31]=0x01)");
      all_passed = false;
    }
  }
  
  if (all_passed) {
    Serial.println("  [pass] all hash_below_target cases passed");
  }
  
  return all_passed;
}

// test 3: verify mine_nonce_range can find valid nonce
bool test_mine_nonce_range() {
  Serial.println("\n[test] mine_nonce_range with easy target");
  
  // create fake block header (80 bytes)
  // fill with some arbitrary data - doesn't matter for this test
  uint8_t header[80];
  for (int i = 0; i < 80; i++) {
    header[i] = i;  // simple pattern
  }
  
  // create an easy target - almost all 0xFF means almost any hash will pass
  // we set byte 31 (most significant) to a medium value so some hashes fail
  // this tests that we actually find valid ones, not that everything passes
  uint8_t target[32];
  memset(target, 0xFF, 32);
  target[31] = 0x7F;  // msb = 0x7F, so about half of hashes should be valid
  
  // mine parameters
  uint32_t start_nonce = 0;
  uint32_t nonce_count = 10000;  // try up to 10000 nonces
  uint32_t found_nonce = 0;
  uint32_t hashes_done = 0;
  
  // measure time
  unsigned long start_time = millis();
  
  // run mining
  bool found = mine_nonce_range(header, start_nonce, nonce_count, target, &found_nonce, &hashes_done);
  
  unsigned long elapsed = millis() - start_time;
  
  // report results
  Serial.print("  nonces tried: ");
  Serial.println(hashes_done);
  Serial.print("  time elapsed: ");
  Serial.print(elapsed);
  Serial.println(" ms");
  
  if (elapsed > 0) {
    float hashrate = (float)hashes_done / ((float)elapsed / 1000.0);
    Serial.print("  hash rate: ");
    Serial.print(hashrate, 1);
    Serial.println(" h/s");
  }
  
  if (found) {
    Serial.print("  found valid nonce: ");
    Serial.println(found_nonce);
    
    // verify the found nonce actually produces valid hash
    // nonce should already be written into header[76-79]
    uint8_t verify_hash[32];
    sha256d(header, 80, verify_hash);
    
    Serial.print("  resulting hash: ");
    print_hash(verify_hash);
    
    if (hash_below_target(verify_hash, target)) {
      Serial.println("  [pass] found nonce produces valid hash");
      return true;
    } else {
      Serial.println("  [fail] found nonce does not produce valid hash!");
      return false;
    }
  } else {
    // with target[31] = 0x7F, we should find something in 10000 tries
    // if not, something is wrong
    Serial.println("  [fail] no valid nonce found in range (unexpected)");
    return false;
  }
}

// test 4: hashrate benchmark with harder target
void test_hashrate_benchmark() {
  Serial.println("\n[test] hashrate benchmark (100000 hashes)");
  
  // create fake header
  uint8_t header[80];
  for (int i = 0; i < 80; i++) {
    header[i] = i * 3;
  }
  
  // impossible target - all zeros means no hash will ever match
  // this forces full iteration through nonce_count
  uint8_t target[32] = {0};
  
  uint32_t found_nonce = 0;
  uint32_t hashes_done = 0;
  uint32_t nonce_count = 100000;
  
  // measure time for full batch
  unsigned long start_time = millis();
  
  mine_nonce_range(header, 0, nonce_count, target, &found_nonce, &hashes_done);
  
  unsigned long elapsed = millis() - start_time;
  
  Serial.print("  hashes: ");
  Serial.println(hashes_done);
  Serial.print("  time: ");
  Serial.print(elapsed);
  Serial.println(" ms");
  
  if (elapsed > 0) {
    float hashrate = (float)hashes_done / ((float)elapsed / 1000.0);
    Serial.print("  hash rate: ");
    Serial.print(hashrate / 1000.0, 2);
    Serial.println(" kh/s");
  }
}

void setup() {
  // initialize serial
  Serial.begin(115200);
  delay(2000);  // wait for serial monitor
  
  Serial.println("\n========================================");
  Serial.println("   sha256_miner test suite");
  Serial.println("========================================");
  
  // initialize miner
  miner_init();
  
  // run tests
  bool test1 = test_sha256d();
  bool test2 = test_hash_below_target();
  bool test3 = test_mine_nonce_range();
  
  // run benchmark
  test_hashrate_benchmark();
  
  // summary
  Serial.println("\n========================================");
  Serial.println("   test summary");
  Serial.println("========================================");
  Serial.print("  sha256d:           ");
  Serial.println(test1 ? "[pass]" : "[fail]");
  Serial.print("  hash_below_target: ");
  Serial.println(test2 ? "[pass]" : "[fail]");
  Serial.print("  mine_nonce_range:  ");
  Serial.println(test3 ? "[pass]" : "[fail]");
  
  if (test1 && test2 && test3) {
    Serial.println("\n  all tests passed!");
  } else {
    Serial.println("\n  some tests failed - check output above");
  }
  
  Serial.println("\n========================================");
}

void loop() {
  // nothing to do - tests run once in setup
  delay(10000);
}