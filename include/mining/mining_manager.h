// mining_manager.h
// coordinates mining operations between stratum client and sha256 miner

#ifndef MINING_MANAGER_H
#define MINING_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include "stratum_client.h"
#include "sha256_miner.h"

// mining state enumeration
enum class mining_state_t {
    STOPPED,        // not mining
    CONNECTING,     // connecting to pool
    MINING,         // actively mining
    ERROR           // error state (connection failed, etc)
};

// statistics structure - updated by mining task, read by UI
struct mining_stats_t {
    float hashrate;             // hashes per second
    uint32_t hashes_total;      // total hashes computed this session
    uint32_t shares_found;      // shares we found
    uint32_t shares_accepted;   // shares pool accepted
    uint32_t shares_rejected;   // shares pool rejected
    uint32_t uptime_seconds;    // how long we've been mining
    double current_difficulty;  // pool difficulty
    bool pool_connected;        // pool connection status
};

class MiningManager {
public:
    MiningManager();
    
    // mining control
    bool start_mining();        // load config from nvs, connect to pool, start mining task
    void stop_mining();         // stop mining task and disconnect
    bool is_mining();           // check if currently mining
    
    // check if we have valid configuration to start mining
    bool is_configured();
    
    // call regularly from main loop - handles pool communication
    void process();
    
    // state and stats
    mining_state_t get_state();
    mining_stats_t get_stats();
    const char* get_error_message();
    
    // manual stop flag (for home screen start/stop button)
    void set_manually_stopped(bool stopped);
    bool is_manually_stopped();
    
private:
    // pool configuration (loaded from nvs)
    char pool_host[64];
    uint16_t pool_port;
    char wallet_address[128];
    char worker_name[32];
    
    // stratum client instance
    StratumClient stratum;
    
    // mining state
    mining_state_t current_state;
    char error_message[64];
    bool manually_stopped;      // user pressed stop button
    
    // mining task handle
    TaskHandle_t mining_task_handle;
    
    // shared data between cores (mining task writes, main thread reads)
    volatile bool mining_active;        // flag to signal mining task to stop
    volatile uint32_t hashes_this_batch;// hashes done in current batch
    volatile bool share_found;          // flag when share is found
    volatile uint32_t found_nonce;      // the winning nonce
    
    // stats tracking
    uint32_t total_hashes;
    uint32_t shares_found_count;
    unsigned long mining_start_time;
    unsigned long last_hashrate_update;
    float current_hashrate;
    
    // work data for mining task
    uint8_t block_header[80];
    uint8_t target[32];
    uint32_t current_nonce;
    
    // internal methods
    bool load_config_from_nvs();        // load active pool and wallet from nvs
    bool parse_pool_address(const char* address, char* host_out, uint16_t* port_out);
    bool connect_to_pool();
    void disconnect_from_pool();
    void submit_share(uint32_t nonce);
    void update_stats();
    void update_work();
    
    // static task function (FreeRTOS requires static)
    static void mining_task_function(void* parameter);
};

// global instance (singleton pattern for easy access)
extern MiningManager mining_manager;

#endif