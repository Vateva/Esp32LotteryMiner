// stratum_client.h
// stratum protocol client for bitcoin pool communication

#ifndef STRATUM_CLIENT_H
#define STRATUM_CLIENT_H

#include <Arduino.h>
#include <WiFiClient.h>

// buffer sizes for stratum communication
#define STRATUM_RECV_BUFFER_SIZE 1024   // incoming message buffer
#define STRATUM_MAX_MERKLE_BRANCHES 16  // max merkle tree depth

// job data received from pool via mining.notify
struct stratum_job_t {
    char job_id[64];                    // pool's identifier for this job
    uint8_t prev_hash[32];              // hash of previous block
    char coinbase1[256];                // first part of coinbase transaction (hex)
    char coinbase2[128];                // second part of coinbase transaction (hex)
    uint8_t merkle_branches[STRATUM_MAX_MERKLE_BRANCHES][32];  // merkle tree branches
    uint8_t merkle_branch_count;        // how many branches we received
    uint32_t version;                   // block version
    uint32_t nbits;                     // difficulty bits (compact format)
    uint32_t ntime;                     // block timestamp
    bool clean_jobs;                    // if true, discard previous work
    bool valid;                         // true if we have received work
};

class StratumClient {
public:
    StratumClient();
    
    // connection management
    bool connect(const char* host, uint16_t port);
    void disconnect();
    bool is_connected();
    
    // stratum protocol methods
    bool subscribe();
    bool authorize(const char* wallet_address, const char* worker_name);
    bool submit_share(const char* job_id, uint32_t ntime, uint32_t nonce);
    
    // work management
    bool has_work();
    void build_block_header(uint8_t* header_out);  // builds 80-byte header from current job
    void get_target(uint8_t* target_out);          // returns 32-byte difficulty target
    const char* get_current_job_id();              // returns job_id for share submission
    uint32_t get_current_ntime();                  // returns ntime for share submission
    
    // call regularly to process incoming pool messages
    void process();
    
    // stats
    uint32_t get_shares_accepted();
    uint32_t get_shares_rejected();
    
private:
    WiFiClient tcp_client;              // tcp socket connection
    
    // receive buffer for incoming data
    char recv_buffer[STRATUM_RECV_BUFFER_SIZE];
    uint16_t recv_buffer_pos;
    
    // extranonce values assigned by pool
    char extranonce1[32];               // hex string from pool
    uint8_t extranonce1_bytes[16];      // binary version
    uint8_t extranonce1_len;            // length in bytes
    uint8_t extranonce2_len;            // length we must fill
    uint32_t extranonce2_counter;       // we increment this for each job
    
    // current job from pool
    stratum_job_t current_job;
    
    // difficulty target
    double current_difficulty;
    uint8_t target[32];                 // 32-byte target computed from difficulty
    
    // message id counter for json-rpc
    uint32_t message_id;
    
    // stats counters
    uint32_t shares_accepted;
    uint32_t shares_rejected;
    
    // internal methods
    bool send_message(const char* message);
    void process_line(const char* line);
    void handle_subscribe_response(const char* result);
    void handle_authorize_response(bool success);
    void handle_submit_response(bool accepted);
    void handle_notify(const char* params);
    void handle_set_difficulty(double difficulty);
    
    // helper functions
    void compute_merkle_root(uint8_t* merkle_root_out);
    void difficulty_to_target(double difficulty, uint8_t* target_out);
    void hex_to_bytes(const char* hex, uint8_t* bytes, size_t byte_len);
    void bytes_to_hex(const uint8_t* bytes, size_t byte_len, char* hex_out);
    void reverse_bytes(uint8_t* data, size_t len);
};

#endif