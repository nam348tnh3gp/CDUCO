#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>

// Sử dụng DSHA1.h C version
#include "DSHA1.h"

// -------------------- Cấu hình --------------------
typedef struct {
    char username[64];
    char mining_key[64];
    char difficulty[16];
    char rig_identifier[64];
    int thread_count;
} Config;

static volatile int g_running = 1;

void signal_handler(int sig) {
    (void)sig;
    printf("\n🛑 Đang dừng miner...\n");
    g_running = 0;
}

// Đọc file config dạng key=value
int read_config(const char *filename, Config *cfg) {
    FILE *f = fopen(filename, "r");
    if (!f) return 0;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '#') continue;
        char *key = strtok(line, "=");
        char *val = strtok(NULL, "\n");
        if (!key || !val) continue;
        while (isspace(*key)) key++;
        char *end = key + strlen(key) - 1;
        while (end > key && isspace(*end)) *end-- = '\0';
        while (isspace(*val)) val++;
        end = val + strlen(val) - 1;
        while (end > val && isspace(*end)) *end-- = '\0';

        if (strcmp(key, "username") == 0) strcpy(cfg->username, val);
        else if (strcmp(key, "mining_key") == 0) strcpy(cfg->mining_key, val);
        else if (strcmp(key, "difficulty") == 0) strcpy(cfg->difficulty, val);
        else if (strcmp(key, "rig_identifier") == 0) strcpy(cfg->rig_identifier, val);
        else if (strcmp(key, "thread_count") == 0) cfg->thread_count = atoi(val);
    }
    fclose(f);
    return 1;
}

// -------------------- Lấy pool từ server --------------------
typedef struct {
    char ip[64];
    int port;
} PoolInfo;

int get_pool(PoolInfo *pool) {
    FILE *fp;
    char buf[1024];
    
    printf("🌐 Đang lấy pool từ server...\n");
    
    // Dùng link getPool mới
    fp = popen("curl -s --max-time 10 https://ducofaucet.nam348tnh.workers.dev/getPool 2>/dev/null", "r");
    if (!fp) {
        printf("❌ Không thể kết nối đến server getPool\n");
        return 0;
    }
    
    if (fgets(buf, sizeof(buf), fp) == NULL) {
        pclose(fp);
        printf("❌ Không nhận được dữ liệu từ server\n");
        return 0;
    }
    pclose(fp);
    
    printf("📡 Response: %s\n", buf);
    
    // Parse JSON response
    char *ip_start = strstr(buf, "\"ip\":\"");
    if (!ip_start) {
        printf("❌ Không tìm thấy IP trong response\n");
        return 0;
    }
    
    ip_start += 6;
    char *ip_end = strchr(ip_start, '"');
    if (!ip_end) {
        printf("❌ Không tìm thấy kết thúc IP\n");
        return 0;
    }
    
    int len = ip_end - ip_start;
    if (len >= (int)sizeof(pool->ip)) len = sizeof(pool->ip) - 1;
    strncpy(pool->ip, ip_start, len);
    pool->ip[len] = '\0';
    
    char *port_start = strstr(buf, "\"port\":");
    if (!port_start) {
        printf("❌ Không tìm thấy PORT trong response\n");
        return 0;
    }
    
    port_start += 7;
    pool->port = atoi(port_start);
    
    // Lấy tên pool nếu có
    char *name_start = strstr(buf, "\"name\":\"");
    if (name_start) {
        name_start += 8;
        char *name_end = strchr(name_start, '"');
        if (name_end) {
            int name_len = name_end - name_start;
            char pool_name[64];
            if (name_len >= (int)sizeof(pool_name)) name_len = sizeof(pool_name) - 1;
            strncpy(pool_name, name_start, name_len);
            pool_name[name_len] = '\0';
            printf("✅ Lấy pool thành công: %s:%d (%s)\n", pool->ip, pool->port, pool_name);
            return 1;
        }
    }
    
    printf("✅ Lấy pool thành công: %s:%d\n", pool->ip, pool->port);
    return 1;
}

// -------------------- Hàm tính SHA1 sử dụng DSHA1 (C version) --------------------
static inline void sha1_string(const char *input, unsigned char *output) {
    DSHA1_CTX ctx;
    dsha1_init(&ctx);
    dsha1_write(&ctx, (const unsigned char*)input, strlen(input));
    dsha1_finalize(&ctx, output);
}

// -------------------- Giải job --------------------
typedef struct {
    char base[256];
    unsigned char target[20];
    int diff;
} Job;

// Tìm nonce thỏa mãn
static inline long long solve_job(const Job *job, double *elapsed_ms) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    char nonce_str[16];
    unsigned char hash[20];
    long long max_nonce = job->diff * 100;
    char buffer[512];
    int base_len = strlen(job->base);
    
    memcpy(buffer, job->base, base_len);
    const unsigned char *target = job->target;
    
    for (long long nonce = 0; nonce <= max_nonce; nonce++) {
        if (nonce < 10) {
            buffer[base_len] = '0' + nonce;
            buffer[base_len + 1] = '\0';
        } else if (nonce < 100) {
            buffer[base_len] = '0' + (nonce / 10);
            buffer[base_len + 1] = '0' + (nonce % 10);
            buffer[base_len + 2] = '\0';
        } else {
            sprintf(nonce_str, "%lld", nonce);
            int nonce_len = strlen(nonce_str);
            memcpy(buffer + base_len, nonce_str, nonce_len);
            buffer[base_len + nonce_len] = '\0';
        }
        
        sha1_string(buffer, hash);
        
        if (memcmp(hash, target, 20) == 0) {
            clock_gettime(CLOCK_MONOTONIC, &end);
            *elapsed_ms = (end.tv_sec - start.tv_sec) * 1000.0 +
                          (end.tv_nsec - start.tv_nsec) / 1e6;
            return nonce;
        }
    }
    return -1;
}

// Hàm định dạng hashrate
static inline const char* format_hashrate(double h) {
    static char buf[64];
    if (h >= 1e9)
        snprintf(buf, sizeof(buf), "%.2f GH/s", h / 1e9);
    else if (h >= 1e6)
        snprintf(buf, sizeof(buf), "%.2f MH/s", h / 1e6);
    else if (h >= 1e3)
        snprintf(buf, sizeof(buf), "%.2f kH/s", h / 1e3);
    else
        snprintf(buf, sizeof(buf), "%.2f H/s", h);
    return buf;
}

// Hàm gửi request qua curl
static char* curl_request(const char *url, const char *data, char *response, size_t response_size) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "curl -s --max-time 10 -X POST -d \"%s\" \"%s\" 2>/dev/null", data, url);
    
    FILE *fp = popen(cmd, "r");
    if (!fp) return NULL;
    
    if (fgets(response, response_size, fp) == NULL) {
        pclose(fp);
        return NULL;
    }
    pclose(fp);
    
    // Xóa ký tự newline
    char *newline = strchr(response, '\n');
    if (newline) *newline = '\0';
    
    return response;
}

// -------------------- Worker thread --------------------
typedef struct {
    int id;
    Config cfg;
    unsigned int multithread_id;
} WorkerArgs;

void *worker_thread(void *arg) {
    WorkerArgs *args = (WorkerArgs*)arg;
    int id = args->id;
    Config cfg = args->cfg;
    unsigned int mtid = args->multithread_id;

    while (g_running) {
        PoolInfo pool;
        
        // Lấy pool từ server
        if (!get_pool(&pool)) {
            fprintf(stderr, "[worker%d] ❌ Không lấy được pool, thử lại sau 10s\n", id);
            sleep(10);
            continue;
        }
        
        char url[128];
        snprintf(url, sizeof(url), "http://%s:%d", pool.ip, pool.port);
        printf("[worker%d] 🔌 Kết nối tới %s\n", id, url);

        char response[1024];
        int accepted = 0, rejected = 0;
        time_t t0 = time(NULL);
        time_t last_stats = t0;
        int consecutive_failures = 0;

        while (g_running) {
            // Gửi request JOB
            char req[256];
            snprintf(req, sizeof(req), "JOB,%s,%s,%s", cfg.username, cfg.difficulty, cfg.mining_key);
            
            if (!curl_request(url, req, response, sizeof(response))) {
                consecutive_failures++;
                if (consecutive_failures >= 3) {
                    printf("[worker%d] ⚠️ Mất kết nối sau %d lần thất bại\n", id, consecutive_failures);
                    break;
                }
                sleep(1);
                continue;
            }
            
            consecutive_failures = 0;
            
            // Parse response
            char *base = strtok(response, ",");
            char *target_hex = strtok(NULL, ",");
            char *diff_str = strtok(NULL, ",");
            
            if (!base || !target_hex || !diff_str) {
                printf("[worker%d] ⚠️ Response lỗi: %s\n", id, response);
                continue;
            }

            Job job;
            strcpy(job.base, base);
            if (strlen(target_hex) != 40) continue;
            for (int i = 0; i < 20; i++) {
                sscanf(target_hex + i*2, "%2hhx", &job.target[i]);
            }
            job.diff = atoi(diff_str);

            double elapsed_ms;
            long long nonce = solve_job(&job, &elapsed_ms);
            
            if (nonce >= 0) {
                double hashrate = 1e6 * nonce / (elapsed_ms * 1000.0);
                
                // Gửi kết quả
                char result[256];
                snprintf(result, sizeof(result), "%lld,%.2f,CMiner,%s,%u", 
                         nonce, hashrate, cfg.rig_identifier, mtid);
                
                if (curl_request(url, result, response, sizeof(response))) {
                    if (strcmp(response, "GOOD") == 0) {
                        accepted++;
                        printf("[worker%d] ✅ Share accepted | %s | Total: %d\n",
                               id, format_hashrate(hashrate), accepted);
                    } else if (strncmp(response, "BAD,", 4) == 0) {
                        rejected++;
                        printf("[worker%d] ❌ Rejected: %s (rej=%d)\n",
                               id, response+4, rejected);
                    } else if (strcmp(response, "BLOCK") == 0) {
                        printf("[worker%d] ⛓️ NEW BLOCK FOUND!\n", id);
                        accepted++;
                    } else {
                        printf("[worker%d] ⚠️ Unknown response: %s\n", id, response);
                    }
                } else {
                    printf("[worker%d] ⚠️ Không gửi được kết quả\n", id);
                    break;
                }

                time_t now = time(NULL);
                if (now - last_stats >= 30) {
                    double uptime = difftime(now, t0);
                    double accept_rate = accepted + rejected > 0 ? 
                                         (accepted * 100.0 / (accepted + rejected)) : 0;
                    printf("[worker%d] 📊 Stats: %d good / %d bad | Uptime: %.0fs | Rate: %.1f%%\n",
                           id, accepted, rejected, uptime, accept_rate);
                    last_stats = now;
                }
            }
        }
        
        if (g_running) {
            fprintf(stderr, "[worker%d] ⚠️ Mất kết nối, reconnect sau 5s...\n", id);
            sleep(5);
        }
    }
    return NULL;
}

// -------------------- Main --------------------
int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    Config cfg;
    if (!read_config("config.txt", &cfg)) {
        fprintf(stderr, "Không đọc được config.txt, dùng mặc định\n");
        strcpy(cfg.username, "Nam2010");
        strcpy(cfg.mining_key, "258013");
        strcpy(cfg.difficulty, "LOW");
        strcpy(cfg.rig_identifier, "iPhoneRig");
        cfg.thread_count = 2;
    }
    
    if (cfg.thread_count < 1) cfg.thread_count = 1;
    if (cfg.thread_count > 4) cfg.thread_count = 4;

    printf("\n========================================\n");
    printf("   🦀 Duino-Coin C Miner v2.0\n");
    printf("========================================\n");
    printf("Username:   %s\n", cfg.username);
    printf("Difficulty: %s\n", cfg.difficulty);
    printf("Rig:        %s\n", cfg.rig_identifier);
    printf("Threads:    %d\n", cfg.thread_count);
    printf("========================================\n");
    printf("GetPool URL: https://ducofaucet.nam348tnh.workers.dev/getPool\n");
    printf("========================================\n\n");

    srand(time(NULL));
    unsigned int mtid = rand() % 90000 + 10000;

    pthread_t threads[cfg.thread_count];
    WorkerArgs args[cfg.thread_count];

    for (int i = 0; i < cfg.thread_count; i++) {
        args[i].id = i;
        args[i].cfg = cfg;
        args[i].multithread_id = mtid;
        pthread_create(&threads[i], NULL, worker_thread, &args[i]);
    }

    printf("✅ Miner started! Press Ctrl+C to stop.\n\n");

    for (int i = 0; i < cfg.thread_count; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("\n🛑 Miner stopped. Goodbye!\n");
    return 0;
}
