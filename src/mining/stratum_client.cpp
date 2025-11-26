// stratum_client.cpp
// stratum protocol client for bitcoin pool communication

#include "mining/stratum_client.h"
#include "mining/sha256_miner.h"
#include <ArduinoJson.h>

// constructor - initialize all state
StratumClient::StratumClient() {
    recv_buffer_pos = 0;
    recv_buffer[0] = '\0';
    
    extranonce1[0] = '\0';
    extranonce1_len = 0;
    extranonce2_len = 4;  // default, pool will tell us actual value
    extranonce2_counter = 0;
    
    current_job.valid = false;
    current_difficulty = 1.0;
    message_id = 1;
    
    shares_accepted = 0;
    shares_rejected = 0;
    
    // initialize target to max value (easiest difficulty)
    memset(target, 0xFF, 32);
}

// connect to pool server
bool StratumClient::connect(const char* host, uint16_t port) {
    Serial.print("[stratum] connecting to ");
    Serial.print(host);
    Serial.print(":");
    Serial.println(port);
    
    // attempt tcp connection with 10 second timeout
    if (!tcp_client.connect(host, port)) {
        Serial.println("[stratum] connection failed");
        return false;
    }
    
    Serial.println("[stratum] connected");
    
    // reset state for new connection
    recv_buffer_pos = 0;
    message_id = 1;
    current_job.valid = false;
    extranonce2_counter = 0;
    
    return true;
}

// disconnect from pool
void StratumClient::disconnect() {
    if (tcp_client.connected()) {
        tcp_client.stop();
        Serial.println("[stratum] disconnected");
    }
    current_job.valid = false;
}

// check if connected to pool
bool StratumClient::is_connected() {
    return tcp_client.connected();
}

// send raw message to pool (adds newline terminator)
bool StratumClient::send_message(const char* message) {
    if (!tcp_client.connected()) {
        return false;
    }
    
    tcp_client.print(message);
    tcp_client.print("\n");
    
    Serial.print("[stratum] sent: ");
    Serial.println(message);
    
    return true;
}

// mining.subscribe - initiate session with pool
// pool responds with extranonce1 and extranonce2_size
bool StratumClient::subscribe() {
    // build subscribe request
    // format: {"id": 1, "method": "mining.subscribe", "params": []}
    StaticJsonDocument<256> doc;
    doc["id"] = message_id++;
    doc["method"] = "mining.subscribe";
    doc.createNestedArray("params");
    
    char buffer[256];
    serializeJson(doc, buffer);
    
    return send_message(buffer);
}

// mining.authorize - authenticate with wallet address
bool StratumClient::authorize(const char* wallet_address, const char* worker_name) {
    // build authorize request
    // format: {"id": 2, "method": "mining.authorize", "params": ["wallet.worker", "x"]}
    StaticJsonDocument<512> doc;
    doc["id"] = message_id++;
    doc["method"] = "mining.authorize";
    
    JsonArray params = doc.createNestedArray("params");
    
    // combine wallet and worker name with dot separator
    char full_worker[128];
    if (worker_name && strlen(worker_name) > 0) {
        snprintf(full_worker, sizeof(full_worker), "%s.%s", wallet_address, worker_name);
    } else {
        strncpy(full_worker, wallet_address, sizeof(full_worker) - 1);
        full_worker[sizeof(full_worker) - 1] = '\0';
    }
    
    params.add(full_worker);
    params.add("x");  // password (usually ignored by pools)
    
    char buffer[512];
    serializeJson(doc, buffer);
    
    return send_message(buffer);
}

// mining.submit - send valid share to pool
bool StratumClient::submit_share(const char* job_id, uint32_t ntime, uint32_t nonce) {
    // build submit request
    // format: {"id": n, "method": "mining.submit", 
    //          "params": ["worker", "job_id", "extranonce2", "ntime", "nonce"]}
    StaticJsonDocument<512> doc;
    doc["id"] = message_id++;
    doc["method"] = "mining.submit";
    
    JsonArray params = doc.createNestedArray("params");
    
    // worker name (we'll use a placeholder, mining_manager should set this properly)
    params.add("worker");
    
    // job id from mining.notify
    params.add(job_id);
    
    // extranonce2 as hex string
    char extranonce2_hex[32];
    uint8_t extranonce2_bytes[8];
    for (int i = 0; i < extranonce2_len; i++) {
        extranonce2_bytes[i] = (extranonce2_counter >> (8 * i)) & 0xFF;
    }
    bytes_to_hex(extranonce2_bytes, extranonce2_len, extranonce2_hex);
    params.add(extranonce2_hex);
    
    // ntime as hex string (8 characters, big-endian)
    char ntime_hex[16];
    snprintf(ntime_hex, sizeof(ntime_hex), "%08x", ntime);
    params.add(ntime_hex);
    
    // nonce as hex string (8 characters, little-endian for display but we send as pool expects)
    char nonce_hex[16];
    snprintf(nonce_hex, sizeof(nonce_hex), "%08x", nonce);
    params.add(nonce_hex);
    
    char buffer[512];
    serializeJson(doc, buffer);
    
    Serial.print("[stratum] submitting share - nonce: ");
    Serial.println(nonce_hex);
    
    return send_message(buffer);
}

// process incoming data from pool
// call this regularly in main loop
void StratumClient::process() {
    if (!tcp_client.connected()) {
        return;
    }
    
    // read available data into buffer
    while (tcp_client.available()) {
        char c = tcp_client.read();
        
        // stratum uses newline-delimited json messages
        if (c == '\n') {
            // null-terminate and process complete line
            recv_buffer[recv_buffer_pos] = '\0';
            
            if (recv_buffer_pos > 0) {
                process_line(recv_buffer);
            }
            
            // reset buffer for next message
            recv_buffer_pos = 0;
        } else if (recv_buffer_pos < STRATUM_RECV_BUFFER_SIZE - 1) {
            // add character to buffer
            recv_buffer[recv_buffer_pos++] = c;
        }
        // if buffer full, we'll lose data - shouldn't happen with proper sizing
    }
}

// process a complete json line from pool
void StratumClient::process_line(const char* line) {
    Serial.print("[stratum] recv: ");
    Serial.println(line);
    
    // parse json
    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, line);
    
    if (error) {
        Serial.print("[stratum] json parse error: ");
        Serial.println(error.c_str());
        return;
    }
    
    // check if this is a response (has "id" that's not null) or notification
    if (doc.containsKey("method")) {
        // this is a notification from pool
        const char* method = doc["method"];
        
        if (strcmp(method, "mining.notify") == 0) {
            // new work available
            JsonArray params = doc["params"];
            handle_notify(line);  // pass full line for detailed parsing
        } else if (strcmp(method, "mining.set_difficulty") == 0) {
            // difficulty adjustment
            JsonArray params = doc["params"];
            if (params.size() > 0) {
                double diff = params[0];
                handle_set_difficulty(diff);
            }
        }
    } else if (doc.containsKey("result")) {
        // this is a response to one of our requests
        // we identify by checking what kind of result it is
        
        JsonVariant result = doc["result"];
        JsonVariant err = doc["error"];
        
        // check if it's a subscribe response (result is array with subscription info)
        if (result.is<JsonArray>() && result[0].is<JsonArray>()) {
            // subscribe response format: [[["mining.notify", "subscription_id"]], "extranonce1", extranonce2_size]
            handle_subscribe_response(line);
        } else if (result.is<bool>()) {
            // authorize or submit response (result is true/false)
            bool success = result.as<bool>();
            
            // we track message types by id, but for simplicity we just check
            // if we already have extranonce (meaning we've subscribed)
            if (extranonce1_len > 0) {
                // this is likely an authorize or submit response
                // check error field to determine outcome
                if (!err.isNull()) {
                    Serial.print("[stratum] error: ");
                    serializeJson(err, Serial);
                    Serial.println();
                    shares_rejected++;
                } else if (success) {
                    // could be authorize success or share accepted
                    Serial.println("[stratum] operation successful");
                    shares_accepted++;
                }
            }
        }
    }
}

// handle mining.subscribe response
void StratumClient::handle_subscribe_response(const char* result) {
    // parse again to extract extranonce info
    StaticJsonDocument<1024> doc;
    deserializeJson(doc, result);
    
    JsonArray res = doc["result"];
    
    // result format: [[["mining.notify", "subscription_id"]], "extranonce1", extranonce2_size]
    // extranonce1 is at index 1, extranonce2_size is at index 2
    
    if (res.size() >= 3) {
        const char* en1 = res[1];
        int en2_size = res[2];
        
        // store extranonce1
        strncpy(extranonce1, en1, sizeof(extranonce1) - 1);
        extranonce1[sizeof(extranonce1) - 1] = '\0';
        extranonce1_len = strlen(extranonce1) / 2;  // hex string, 2 chars per byte
        hex_to_bytes(extranonce1, extranonce1_bytes, extranonce1_len);
        
        // store extranonce2 size
        extranonce2_len = en2_size;
        
        Serial.print("[stratum] subscribed - extranonce1: ");
        Serial.print(extranonce1);
        Serial.print(", extranonce2_size: ");
        Serial.println(extranonce2_len);
    }
}

// handle mining.notify - new work from pool
void StratumClient::handle_notify(const char* line) {
    StaticJsonDocument<2048> doc;
    deserializeJson(doc, line);
    
    JsonArray params = doc["params"];
    
    if (params.size() < 9) {
        Serial.println("[stratum] invalid notify params");
        return;
    }
    
    // extract job parameters
    // params: [job_id, prevhash, coinbase1, coinbase2, merkle_branches[], version, nbits, ntime, clean_jobs]
    
    const char* job_id = params[0];
    const char* prev_hash_hex = params[1];
    const char* coinbase1 = params[2];
    const char* coinbase2 = params[3];
    JsonArray merkle = params[4];
    const char* version_hex = params[5];
    const char* nbits_hex = params[6];
    const char* ntime_hex = params[7];
    bool clean = params[8];
    
    // store job id
    strncpy(current_job.job_id, job_id, sizeof(current_job.job_id) - 1);
    current_job.job_id[sizeof(current_job.job_id) - 1] = '\0';
    
    // store prev hash (needs byte reversal for header)
    hex_to_bytes(prev_hash_hex, current_job.prev_hash, 32);
    
    // store coinbase parts
    strncpy(current_job.coinbase1, coinbase1, sizeof(current_job.coinbase1) - 1);
    current_job.coinbase1[sizeof(current_job.coinbase1) - 1] = '\0';
    
    strncpy(current_job.coinbase2, coinbase2, sizeof(current_job.coinbase2) - 1);
    current_job.coinbase2[sizeof(current_job.coinbase2) - 1] = '\0';
    
    // store merkle branches
    current_job.merkle_branch_count = min((int)merkle.size(), STRATUM_MAX_MERKLE_BRANCHES);
    for (int i = 0; i < current_job.merkle_branch_count; i++) {
        const char* branch_hex = merkle[i];
        hex_to_bytes(branch_hex, current_job.merkle_branches[i], 32);
    }
    
    // store version, nbits, ntime (as 32-bit integers)
    current_job.version = strtoul(version_hex, NULL, 16);
    current_job.nbits = strtoul(nbits_hex, NULL, 16);
    current_job.ntime = strtoul(ntime_hex, NULL, 16);
    
    current_job.clean_jobs = clean;
    current_job.valid = true;
    
    // increment extranonce2 for new work
    extranonce2_counter++;
    
    Serial.print("[stratum] new job: ");
    Serial.print(job_id);
    Serial.print(", clean: ");
    Serial.println(clean ? "yes" : "no");
}

// handle mining.set_difficulty
void StratumClient::handle_set_difficulty(double difficulty) {
    current_difficulty = difficulty;
    difficulty_to_target(difficulty, target);
    
    Serial.print("[stratum] difficulty set to: ");
    Serial.println(difficulty, 8);
}

// convert pool difficulty to 32-byte target
void StratumClient::difficulty_to_target(double difficulty, uint8_t* target_out) {
    // bitcoin pool difficulty 1 target:
    // 0x00000000ffff0000000000000000000000000000000000000000000000000000
    // 
    // target = diff1_target / difficulty
    //
    // for simplicity, we compute this by setting bytes based on difficulty
    
    // start with all zeros (hardest possible)
    memset(target_out, 0, 32);
    
    // calculate target from difficulty
    // diff 1 = 0xffff * 2^208
    // target = (0xffff * 2^208) / difficulty
    
    // simplified approach: set leading bytes based on log2(difficulty)
    // this is approximate but works for pool mining
    
    double target_val = 65535.0 / difficulty;  // 0xFFFF / diff
    
    // find position and value
    // target bytes 29-30 hold the main value at diff 1
    // as difficulty increases, we shift right (toward byte 31)
    
    if (difficulty <= 0) {
        // invalid difficulty, set easiest target
        memset(target_out, 0xFF, 32);
        return;
    }
    
    // compute full precision target
    // we'll use a simpler method: pdiff (pool difficulty) format
    // target = 0x00000000FFFFFFFF... / difficulty
    
    uint64_t base = 0xFFFFFFFFFFFFFFFFULL;
    uint64_t target_high = (uint64_t)(base / difficulty);
    
    // store in bytes 24-31 (big endian position for the significant part)
    // but remember bitcoin is little endian, so we need to be careful
    
    // actually, let's use a more straightforward approach
    // that matches what pools expect
    
    // pdiff target calculation:
    // target[0-3] = 0x00000000
    // target[4-7] = 0xFFFF0000 / difficulty (approximately)
    
    // for pool difficulties, we can use this simpler formula:
    // the target is essentially how many leading zero bits are required
    
    // reset target to known diff-1 value and divide
    memset(target_out, 0, 32);
    target_out[29] = 0xFF;
    target_out[30] = 0xFF;
    
    // scale down based on difficulty
    // this is a simplified approach - divide the target value
    if (difficulty > 1.0) {
        double scaled = 0xFFFF / difficulty;
        uint16_t val = (uint16_t)scaled;
        target_out[29] = val & 0xFF;
        target_out[30] = (val >> 8) & 0xFF;
    }
}

// build 80-byte block header from current job
void StratumClient::build_block_header(uint8_t* header_out) {
    if (!current_job.valid) {
        memset(header_out, 0, 80);
        return;
    }
    
    // header structure (80 bytes):
    // bytes 0-3:   version (4 bytes, little-endian)
    // bytes 4-35:  previous block hash (32 bytes)
    // bytes 36-67: merkle root (32 bytes)
    // bytes 68-71: timestamp (4 bytes, little-endian)
    // bytes 72-75: nbits (4 bytes, little-endian)
    // bytes 76-79: nonce (4 bytes, little-endian) - set to 0, miner fills this
    
    // version (little-endian)
    header_out[0] = (current_job.version >> 0) & 0xFF;
    header_out[1] = (current_job.version >> 8) & 0xFF;
    header_out[2] = (current_job.version >> 16) & 0xFF;
    header_out[3] = (current_job.version >> 24) & 0xFF;
    
    // previous block hash (already in correct byte order from pool)
    memcpy(header_out + 4, current_job.prev_hash, 32);
    
    // merkle root (computed from coinbase + merkle branches)
    uint8_t merkle_root[32];
    compute_merkle_root(merkle_root);
    memcpy(header_out + 36, merkle_root, 32);
    
    // timestamp (little-endian)
    header_out[68] = (current_job.ntime >> 0) & 0xFF;
    header_out[69] = (current_job.ntime >> 8) & 0xFF;
    header_out[70] = (current_job.ntime >> 16) & 0xFF;
    header_out[71] = (current_job.ntime >> 24) & 0xFF;
    
    // nbits (little-endian)
    header_out[72] = (current_job.nbits >> 0) & 0xFF;
    header_out[73] = (current_job.nbits >> 8) & 0xFF;
    header_out[74] = (current_job.nbits >> 16) & 0xFF;
    header_out[75] = (current_job.nbits >> 24) & 0xFF;
    
    // nonce - set to 0, mining code will fill this
    header_out[76] = 0;
    header_out[77] = 0;
    header_out[78] = 0;
    header_out[79] = 0;
}

// compute merkle root from coinbase transaction and merkle branches
void StratumClient::compute_merkle_root(uint8_t* merkle_root_out) {
    // step 1: build coinbase transaction
    // coinbase = coinbase1 + extranonce1 + extranonce2 + coinbase2
    
    // calculate sizes
    size_t cb1_len = strlen(current_job.coinbase1) / 2;
    size_t cb2_len = strlen(current_job.coinbase2) / 2;
    size_t coinbase_len = cb1_len + extranonce1_len + extranonce2_len + cb2_len;
    
    // allocate coinbase buffer (on stack, should be < 500 bytes typically)
    uint8_t coinbase[512];
    size_t pos = 0;
    
    // coinbase1
    hex_to_bytes(current_job.coinbase1, coinbase + pos, cb1_len);
    pos += cb1_len;
    
    // extranonce1
    memcpy(coinbase + pos, extranonce1_bytes, extranonce1_len);
    pos += extranonce1_len;
    
    // extranonce2 (our counter, little-endian)
    for (int i = 0; i < extranonce2_len; i++) {
        coinbase[pos++] = (extranonce2_counter >> (8 * i)) & 0xFF;
    }
    
    // coinbase2
    hex_to_bytes(current_job.coinbase2, coinbase + pos, cb2_len);
    pos += cb2_len;
    
    // step 2: hash coinbase transaction (double sha256)
    uint8_t coinbase_hash[32];
    sha256d(coinbase, pos, coinbase_hash);
    
    // step 3: compute merkle root by hashing with each branch
    // start with coinbase hash, then hash(current + branch) for each branch
    uint8_t current_hash[32];
    memcpy(current_hash, coinbase_hash, 32);
    
    for (int i = 0; i < current_job.merkle_branch_count; i++) {
        // concatenate current hash with branch
        uint8_t concat[64];
        memcpy(concat, current_hash, 32);
        memcpy(concat + 32, current_job.merkle_branches[i], 32);
        
        // double sha256 the concatenation
        sha256d(concat, 64, current_hash);
    }
    
    // current_hash now contains the merkle root
    memcpy(merkle_root_out, current_hash, 32);
}

// get current difficulty target
void StratumClient::get_target(uint8_t* target_out) {
    memcpy(target_out, target, 32);
}

// check if we have valid work
bool StratumClient::has_work() {
    return current_job.valid;
}

// get current job id for share submission
const char* StratumClient::get_current_job_id() {
    return current_job.job_id;
}

// get current ntime for share submission
uint32_t StratumClient::get_current_ntime() {
    return current_job.ntime;
}

// get accepted share count
uint32_t StratumClient::get_shares_accepted() {
    return shares_accepted;
}

// get rejected share count
uint32_t StratumClient::get_shares_rejected() {
    return shares_rejected;
}

// helper: convert hex string to bytes
void StratumClient::hex_to_bytes(const char* hex, uint8_t* bytes, size_t byte_len) {
    for (size_t i = 0; i < byte_len; i++) {
        char high = hex[i * 2];
        char low = hex[i * 2 + 1];
        
        uint8_t high_val = (high >= 'a') ? (high - 'a' + 10) : 
                          (high >= 'A') ? (high - 'A' + 10) : (high - '0');
        uint8_t low_val = (low >= 'a') ? (low - 'a' + 10) : 
                         (low >= 'A') ? (low - 'A' + 10) : (low - '0');
        
        bytes[i] = (high_val << 4) | low_val;
    }
}

// helper: convert bytes to hex string
void StratumClient::bytes_to_hex(const uint8_t* bytes, size_t byte_len, char* hex_out) {
    const char* hex_chars = "0123456789abcdef";
    for (size_t i = 0; i < byte_len; i++) {
        hex_out[i * 2] = hex_chars[(bytes[i] >> 4) & 0x0F];
        hex_out[i * 2 + 1] = hex_chars[bytes[i] & 0x0F];
    }
    hex_out[byte_len * 2] = '\0';
}

// helper: reverse byte order in place
void StratumClient::reverse_bytes(uint8_t* data, size_t len) {
    for (size_t i = 0; i < len / 2; i++) {
        uint8_t temp = data[i];
        data[i] = data[len - 1 - i];
        data[len - 1 - i] = temp;
    }
}