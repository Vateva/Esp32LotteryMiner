// mining_manager.cpp
// coordinates mining operations between stratum client and sha256 miner

#include <WiFi.h>
#include "mining/mining_manager.h"
#include "esp_task_wdt.h"

// how many nonces to try per batch before checking for new work/stop signal
#define NONCES_PER_BATCH 10000

// how often to update hashrate calculation (milliseconds)
#define HASHRATE_UPDATE_INTERVAL 2000

// stack size for mining task (bytes)
#define MINING_TASK_STACK_SIZE 4096

// mining task priority (higher = more priority)
#define MINING_TASK_PRIORITY 1

// nvs namespace used by config screens
#define NVS_NAMESPACE "esp32btcminer"

// number of wallet/pool slots
#define CONFIG_SLOTS 4

// global instance
MiningManager mining_manager;

// constructor - initialize all state
MiningManager::MiningManager() {
    pool_host[0] = '\0';
    pool_port = 0;
    wallet_address[0] = '\0';
    strcpy(worker_name, "esp32");  // default worker name
    
    current_state = mining_state_t::STOPPED;
    error_message[0] = '\0';
    manually_stopped = false;
    
    mining_task_handle = NULL;
    mining_active = false;
    hashes_this_batch = 0;
    share_found = false;
    found_nonce = 0;
    
    total_hashes = 0;
    shares_found_count = 0;
    mining_start_time = 0;
    last_hashrate_update = 0;
    current_hashrate = 0.0f;
    
    current_nonce = 0;
    
    memset(block_header, 0, 80);
    memset(target, 0xFF, 32);
}

// parse pool address string "host:port" into separate components
// returns true on success, false if format is invalid
bool MiningManager::parse_pool_address(const char* address, char* host_out, uint16_t* port_out) {
    // find the colon separator
    const char* colon = strchr(address, ':');
    
    if (colon == NULL) {
        // no colon found, invalid format
        Serial.println("[mining] invalid pool address format (no port)");
        return false;
    }
    
    // calculate host length (everything before colon)
    size_t host_len = colon - address;
    
    if (host_len == 0 || host_len >= 64) {
        // host is empty or too long
        Serial.println("[mining] invalid pool host length");
        return false;
    }
    
    // copy host portion
    strncpy(host_out, address, host_len);
    host_out[host_len] = '\0';
    
    // parse port (everything after colon)
    const char* port_str = colon + 1;
    
    if (strlen(port_str) == 0) {
        // no port number after colon
        Serial.println("[mining] missing port number");
        return false;
    }
    
    // convert port string to integer
    int port = atoi(port_str);
    
    if (port <= 0 || port > 65535) {
        // invalid port number
        Serial.println("[mining] invalid port number");
        return false;
    }
    
    *port_out = (uint16_t)port;
    
    return true;
}

// load active pool and wallet configuration from nvs
// returns true if valid configuration found, false otherwise
bool MiningManager::load_config_from_nvs() {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);  // read-only mode
    
    bool found_wallet = false;
    bool found_pool = false;
    
    // search for active wallet
    for (int i = 0; i < CONFIG_SLOTS; i++) {
        char active_key[16];
        char config_key[16];
        char addr_key[16];
        
        sprintf(active_key, "wallet%d_act", i);
        sprintf(config_key, "wallet%d_cfg", i);
        sprintf(addr_key, "wallet%d_addr", i);
        
        bool is_active = prefs.getBool(active_key, false);
        bool is_configured = prefs.getBool(config_key, false);
        
        if (is_active && is_configured) {
            // found active wallet, load address
            String addr = prefs.getString(addr_key, "");
            
            if (addr.length() > 0) {
                strncpy(wallet_address, addr.c_str(), sizeof(wallet_address) - 1);
                wallet_address[sizeof(wallet_address) - 1] = '\0';
                found_wallet = true;
                
                Serial.print("[mining] loaded wallet: ");
                Serial.println(wallet_address);
            }
            break;  // only need one active wallet
        }
    }
    
    // search for active pool
    for (int i = 0; i < CONFIG_SLOTS; i++) {
        char active_key[16];
        char config_key[16];
        char addr_key[16];
        
        sprintf(active_key, "pool%d_act", i);
        sprintf(config_key, "pool%d_cfg", i);
        sprintf(addr_key, "pool%d_addr", i);
        
        bool is_active = prefs.getBool(active_key, false);
        bool is_configured = prefs.getBool(config_key, false);
        
        if (is_active && is_configured) {
            // found active pool, load address
            String addr = prefs.getString(addr_key, "");
            
            if (addr.length() > 0) {
                // parse "host:port" format
                if (parse_pool_address(addr.c_str(), pool_host, &pool_port)) {
                    found_pool = true;
                    
                    Serial.print("[mining] loaded pool: ");
                    Serial.print(pool_host);
                    Serial.print(":");
                    Serial.println(pool_port);
                }
            }
            break;  // only need one active pool
        }
    }
    
    prefs.end();
    
    return found_wallet && found_pool;
}

// check if we have valid configuration to start mining
bool MiningManager::is_configured() {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);  // read-only mode
    
    bool has_wallet = false;
    bool has_pool = false;
    
    // check for active configured wallet
    for (int i = 0; i < CONFIG_SLOTS; i++) {
        char active_key[16];
        char config_key[16];
        
        sprintf(active_key, "wallet%d_act", i);
        sprintf(config_key, "wallet%d_cfg", i);
        
        if (prefs.getBool(active_key, false) && prefs.getBool(config_key, false)) {
            has_wallet = true;
            break;
        }
    }
    
    // check for active configured pool
    for (int i = 0; i < CONFIG_SLOTS; i++) {
        char active_key[16];
        char config_key[16];
        
        sprintf(active_key, "pool%d_act", i);
        sprintf(config_key, "pool%d_cfg", i);
        
        if (prefs.getBool(active_key, false) && prefs.getBool(config_key, false)) {
            has_pool = true;
            break;
        }
    }
    
    prefs.end();
    
    // also need wifi to be connected
    bool has_wifi = (WiFi.status() == WL_CONNECTED);
    
    return has_wallet && has_pool && has_wifi;
}

// start mining - load config, connect to pool and launch mining task
bool MiningManager::start_mining() {
    // check if already mining
    if (current_state == mining_state_t::MINING) {
        Serial.println("[mining] already mining");
        return true;
    }
    
    // check wifi connection first
    if (WiFi.status() != WL_CONNECTED) {
        strcpy(error_message, "WiFi not connected");
        current_state = mining_state_t::ERROR;
        Serial.println("[mining] error: wifi not connected");
        return false;
    }
    
    // load configuration from nvs
    if (!load_config_from_nvs()) {
        strcpy(error_message, "Config not found");
        current_state = mining_state_t::ERROR;
        Serial.println("[mining] error: no valid wallet/pool configured");
        return false;
    }
    
    // validate loaded configuration
    if (strlen(pool_host) == 0 || pool_port == 0) {
        strcpy(error_message, "Pool not configured");
        current_state = mining_state_t::ERROR;
        Serial.println("[mining] error: pool not configured");
        return false;
    }
    
    if (strlen(wallet_address) == 0) {
        strcpy(error_message, "Wallet not configured");
        current_state = mining_state_t::ERROR;
        Serial.println("[mining] error: wallet not configured");
        return false;
    }
    
    // connect to pool
    current_state = mining_state_t::CONNECTING;
    if (!connect_to_pool()) {
        return false;
    }
    
    // reset stats for new session
    total_hashes = 0;
    shares_found_count = 0;
    current_nonce = 0;
    mining_start_time = millis();
    last_hashrate_update = millis();
    current_hashrate = 0.0f;
    manually_stopped = false;
    
    // get initial work
    update_work();
    
    // set mining active flag before creating task
    mining_active = true;
    share_found = false;
    
    // create mining task pinned to core 0
    // core 1 is used by arduino loop() for ui and network
    BaseType_t result = xTaskCreatePinnedToCore(
        mining_task_function,       // task function
        "MiningTask",               // task name
        MINING_TASK_STACK_SIZE,     // stack size
        this,                       // parameter (pass this pointer)
        MINING_TASK_PRIORITY,       // priority
        &mining_task_handle,        // task handle
        0                           // core 0
    );
    
    if (result != pdPASS) {
        strcpy(error_message, "Failed to create task");
        current_state = mining_state_t::ERROR;
        mining_active = false;
        Serial.println("[mining] error: failed to create mining task");
        return false;
    }
    
    current_state = mining_state_t::MINING;
    Serial.println("[mining] started mining on core 0");
    
    return true;
}

// stop mining - kill task and disconnect
void MiningManager::stop_mining() {
    Serial.println("[mining] stopping...");
    
    // signal mining task to stop
    mining_active = false;
    
    // wait a bit for task to finish current batch
    delay(100);
    
    // delete task if it exists
    if (mining_task_handle != NULL) {
        vTaskDelete(mining_task_handle);
        mining_task_handle = NULL;
        Serial.println("[mining] task deleted");
    }
    
    // disconnect from pool
    disconnect_from_pool();
    
    // update state
    current_state = mining_state_t::STOPPED;
    
    Serial.println("[mining] stopped");
}

// check if mining is active
bool MiningManager::is_mining() {
    return current_state == mining_state_t::MINING;
}

// process function - call regularly from main loop
// handles pool communication and share submission
void MiningManager::process() {
    // always process stratum messages if connected
    if (stratum.is_connected()) {
        stratum.process();
    }
    
    // if not mining, nothing else to do
    if (current_state != mining_state_t::MINING) {
        return;
    }
    
    // check if pool connection dropped
    if (!stratum.is_connected()) {
        Serial.println("[mining] pool connection lost, attempting reconnect...");
        
        // try to reconnect
        if (!connect_to_pool()) {
            // reconnect failed, stop mining
            stop_mining();
            strcpy(error_message, "Pool disconnected");
            current_state = mining_state_t::ERROR;
            return;
        }
        
        // reconnected, get fresh work
        update_work();
    }
    
    // check if mining task found a share
    if (share_found) {
        share_found = false;  // clear flag
        shares_found_count++;
        
        // submit to pool
        submit_share(found_nonce);
    }
    
    // check if we have new work from pool
    // stratum client sets clean_jobs flag when new work arrives
    // for simplicity, we update work periodically anyway
    if (stratum.has_work()) {
        update_work();
    }
    
    // update stats periodically
    update_stats();
}

// connect to pool and perform handshake
bool MiningManager::connect_to_pool() {
    Serial.println("[mining] connecting to pool...");
    
    // tcp connect
    if (!stratum.connect(pool_host, pool_port)) {
        strcpy(error_message, "Connection failed");
        current_state = mining_state_t::ERROR;
        return false;
    }
    
    // subscribe
    if (!stratum.subscribe()) {
        strcpy(error_message, "Subscribe failed");
        current_state = mining_state_t::ERROR;
        stratum.disconnect();
        return false;
    }
    
    // wait for subscribe response
    unsigned long start = millis();
    while (millis() - start < 5000) {
        stratum.process();
        delay(100);
        
        // check if we got extranonce (means subscribe succeeded)
        // we can check by trying to build header - if valid, we're ready
        if (stratum.has_work()) {
            break;
        }
    }
    
    // authorize with wallet
    if (!stratum.authorize(wallet_address, worker_name)) {
        strcpy(error_message, "Authorize failed");
        current_state = mining_state_t::ERROR;
        stratum.disconnect();
        return false;
    }
    
    // wait for authorize response and initial work
    start = millis();
    while (millis() - start < 5000) {
        stratum.process();
        delay(100);
        
        if (stratum.has_work()) {
            Serial.println("[mining] received work from pool");
            break;
        }
    }
    
    if (!stratum.has_work()) {
        strcpy(error_message, "No work received");
        current_state = mining_state_t::ERROR;
        stratum.disconnect();
        return false;
    }
    
    Serial.println("[mining] pool connection established");
    return true;
}

// disconnect from pool
void MiningManager::disconnect_from_pool() {
    stratum.disconnect();
}

// submit a found share to the pool
void MiningManager::submit_share(uint32_t nonce) {
    Serial.print("[mining] submitting share with nonce: ");
    Serial.println(nonce, HEX);
    
    stratum.submit_share(
        stratum.get_current_job_id(),
        stratum.get_current_ntime(),
        nonce
    );
}

// update work data from stratum client
void MiningManager::update_work() {
    // build block header from current job
    stratum.build_block_header(block_header);
    
    // get current target
    stratum.get_target(target);
    
    // reset nonce for new work
    current_nonce = 0;
}

// update hashrate and other stats
void MiningManager::update_stats() {
    unsigned long now = millis();
    
    // only update hashrate every HASHRATE_UPDATE_INTERVAL
    if (now - last_hashrate_update >= HASHRATE_UPDATE_INTERVAL) {
        // calculate hashrate from hashes done since last update
        float elapsed_seconds = (now - last_hashrate_update) / 1000.0f;
        
        if (elapsed_seconds > 0) {
            current_hashrate = hashes_this_batch / elapsed_seconds;
            total_hashes += hashes_this_batch;
            hashes_this_batch = 0;
        }
        
        last_hashrate_update = now;
    }
}

// get current mining state
mining_state_t MiningManager::get_state() {
    return current_state;
}

// get current stats
mining_stats_t MiningManager::get_stats() {
    mining_stats_t stats;
    
    stats.hashrate = current_hashrate;
    stats.hashes_total = total_hashes + hashes_this_batch;
    stats.shares_found = shares_found_count;
    stats.shares_accepted = stratum.get_shares_accepted();
    stats.shares_rejected = stratum.get_shares_rejected();
    
    // calculate uptime
    if (mining_start_time > 0 && current_state == mining_state_t::MINING) {
        stats.uptime_seconds = (millis() - mining_start_time) / 1000;
    } else {
        stats.uptime_seconds = 0;
    }
    
    stats.pool_connected = stratum.is_connected();
    
    // difficulty would come from stratum client
    // for now just use 1.0 as placeholder
    stats.current_difficulty = 1.0;
    
    return stats;
}

// get error message
const char* MiningManager::get_error_message() {
    return error_message;
}

// set manually stopped flag
void MiningManager::set_manually_stopped(bool stopped) {
    manually_stopped = stopped;
}

// check if manually stopped
bool MiningManager::is_manually_stopped() {
    return manually_stopped;
}

// static mining task function - runs on core 0
// this is the tight loop that does actual sha256 hashing
void MiningManager::mining_task_function(void* parameter) {
    // get pointer to MiningManager instance
    MiningManager* manager = (MiningManager*)parameter;
    
    Serial.println("[mining] task started on core 0");
    
    // disable watchdog for this core
    // mining is intentionally a tight loop that doesn't yield
    disableCore0WDT();
    
    // local copies of work data (avoid accessing shared memory in tight loop)
    uint8_t header[80];
    uint8_t target[32];
    uint32_t nonce = 0;
    uint32_t found = 0;
    uint32_t hashes = 0;
    
    // main mining loop
    while (manager->mining_active) {
        // copy current work data
        // this is the only place we read from shared memory in the loop
        memcpy(header, manager->block_header, 80);
        memcpy(target, manager->target, 32);
        nonce = manager->current_nonce;
        
        // mine a batch of nonces
        bool found_share = mine_nonce_range(
            header,
            nonce,
            NONCES_PER_BATCH,
            target,
            &found,
            &hashes
        );
        
        // update shared state
        manager->hashes_this_batch += hashes;
        manager->current_nonce = nonce + NONCES_PER_BATCH;
        
        // check if we found a valid share
        if (found_share) {
            Serial.print("[mining] share found! nonce: ");
            Serial.println(found, HEX);
            
            manager->found_nonce = found;
            manager->share_found = true;
            
            // small delay to let main thread process the share
            // before we potentially find another one
            delay(10);
        }
        
        // check for nonce wraparound
        // if we've exhausted all 4 billion nonces, we need new work
        if (manager->current_nonce < nonce) {
            Serial.println("[mining] nonce range exhausted, waiting for new work");
            
            // wait for new work from pool
            // main thread will update block_header when new work arrives
            while (manager->mining_active && manager->current_nonce < nonce) {
                delay(100);
            }
        }
    }
    
    Serial.println("[mining] task stopping");
    
    // task cleanup - will be deleted by stop_mining()
    vTaskDelete(NULL);
}