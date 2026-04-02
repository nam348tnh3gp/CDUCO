#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

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

// Hàm kiểm tra mining key có an toàn không
int is_safe_mining_key(const char *key) {
    if (strchr(key, ',') != NULL) {
        fprintf(stderr, "❌ Lỗi: Mining key chứa dấu phẩy (','), ký tự này không được phép.\n");
        return 0;
    }
    if (strchr(key, '\n') != NULL || strchr(key, '\r') != NULL) {
        fprintf(stderr, "❌ Lỗi: Mining key chứa ký tự xuống dòng.\n");
        return 0;
    }
    for (const char *p = key; *p; p++) {
        if (iscntrl((unsigned char)*p) && *p != '\t') {
            fprintf(stderr, "❌ Lỗi: Mining key chứa ký tự điều khiển (mã %d).\n", *p);
            return 0;
        }
    }
    return 1;
}

// Đọc file config dạng key=value
int read_config(const char *filename, Config *cfg) {
    FILE *f = fopen(filename, "r");
    if (!f) return 0;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '#') continue;
        
        char *eq = strchr(line, '=');
        if (!eq) continue;
        
        *eq = '\0';
        char *key = line;
        char *val = eq + 1;
        
        while (isspace((unsigned char)*key)) key++;
        char *end = key + strlen(key) - 1;
        while (end > key && isspace((unsigned char)*end)) *end-- = '\0';
        
        while (isspace((unsigned char)*val)) val++;
        end = val + strlen(val) - 1;
        while (end > val && isspace((unsigned char)*end)) *end-- = '\0';

        if (strcmp(key, "username") == 0) {
            strncpy(cfg->username, val, sizeof(cfg->username) - 1);
            cfg->username[sizeof(cfg->username) - 1] = '\0';
        } else if (strcmp(key, "mining_key") == 0) {
            strncpy(cfg->mining_key, val, sizeof(cfg->mining_key) - 1);
            cfg->mining_key[sizeof(cfg->mining_key) - 1] = '\0';
        } else if (strcmp(key, "difficulty") == 0) {
            strncpy(cfg->difficulty, val, sizeof(cfg->difficulty) - 1);
            cfg->difficulty[sizeof(cfg->difficulty) - 1] = '\0';
        } else if (strcmp(key, "rig_identifier") == 0) {
            strncpy(cfg->rig_identifier, val, sizeof(cfg->rig_identifier) - 1);
            cfg->rig_identifier[sizeof(cfg->rig_identifier) - 1] = '\0';
        } else if (strcmp(key, "thread_count") == 0) {
            cfg->thread_count = atoi(val);
        }
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
    
    printf("✅ Lấy pool thành công: %s:%d\n", pool->ip, pool->port);
    return 1;
}

// -------------------- Tạo kết nối TCP --------------------
int tcp_connect(const char *ip, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);
    
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }
    
    return sock;
}

// -------------------- Gửi và nhận dữ liệu qua TCP --------------------
int send_tcp(int sock, const char *data) {
    ssize_t len = strlen(data);
    ssize_t sent = send(sock, data, len, 0);
    return sent == len ? 1 : 0;
}

int recv_line(int sock, char *buffer, size_t size) {
    size_t i = 0;
    char c;
    while (i < size - 1 && recv(sock, &c, 1, 0) > 0) {
        if (c == '\n') {
            buffer[i] = '\0';
            return 1;
        }
        buffer[i++] = c;
    }
    buffer[i] = '\0';
    return i > 0 ? 1 : 0;
}

// -------------------- Hàm tính SHA1 --------------------
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
    
    // Kiểm tra mining key
    if (!is_safe_mining_key(cfg.mining_key)) {
        fprintf(stderr, "[worker%d] ❌ Mining key không an toàn, thread sẽ dừng.\n", id);
        return NULL;
    }

    while (g_running) {
        PoolInfo pool;
        
        if (!get_pool(&pool)) {
            fprintf(stderr, "[worker%d] ❌ Không lấy được pool, thử lại sau 10s\n", id);
            sleep(10);
            continue;
        }
        
        printf("[worker%d] 🔌 Kết nối TCP tới %s:%d\n", id, pool.ip, pool.port);
        
        int sock = tcp_connect(pool.ip, pool.port);
        if (sock < 0) {
            fprintf(stderr, "[worker%d] ❌ Không thể kết nối, thử lại sau 5s\n", id);
            sleep(5);
            continue;
        }
        
        char server_version[128];
        if (recv_line(sock, server_version, sizeof(server_version))) {
            printf("[worker%d] ✅ Kết nối thành công (server v%s)\n", id, server_version);
        }
        
        int accepted = 0;
        int rejected = 0;
        time_t t0 = time(NULL);
        time_t last_stats = t0;
        
        while (g_running) {
            // ========== SỬA LỖI 1: Đúng format JOB (có dấu phẩy cuối) ==========
            // Format: JOB,username,difficulty,mining_key,
            char req[256];
            snprintf(req, sizeof(req), "JOB,%s,%s,%s,\n", 
                     cfg.username, cfg.difficulty, cfg.mining_key);
            
            if (!send_tcp(sock, req)) {
                printf("[worker%d] ⚠️ Gửi request thất bại\n", id);
                break;
            }
            
            char jobline[1024];
            if (!recv_line(sock, jobline, sizeof(jobline))) {
                printf("[worker%d] ⚠️ Không nhận được job\n", id);
                break;
            }
            
            char *base = strtok(jobline, ",");
            char *target_hex = strtok(NULL, ",");
            char *diff_str = strtok(NULL, ",");
            
            if (!base || !target_hex || !diff_str) {
                printf("[worker%d] ⚠️ Job format lỗi: %s\n", id, jobline);
                continue;
            }
            
            Job job;
            strncpy(job.base, base, sizeof(job.base) - 1);
            job.base[sizeof(job.base) - 1] = '\0';
            if (strlen(target_hex) != 40) continue;
            for (int i = 0; i < 20; i++) {
                sscanf(target_hex + i*2, "%2hhx", &job.target[i]);
            }
            job.diff = atoi(diff_str);
            
            double elapsed_ms;
            long long nonce = solve_job(&job, &elapsed_ms);
            
            if (nonce >= 0) {
                double hashrate = (nonce * 1000.0) / elapsed_ms;
                
                // ========== SỬA LỖI 2: Đúng format result (2 dấu phẩy liên tiếp) ==========
                // Format: nonce,hashrate,CMiner,rig_identifier,,thread_id
                char result[256];
                snprintf(result, sizeof(result), "%lld,%.2f,CMiner,%s,,%u\n", 
                         nonce, hashrate, cfg.rig_identifier, mtid);
                
                if (!send_tcp(sock, result)) {
                    printf("[worker%d] ⚠️ Gửi kết quả thất bại\n", id);
                    break;
                }
                
                char feedback[128];
                if (!recv_line(sock, feedback, sizeof(feedback))) {
                    printf("[worker%d] ⚠️ Không nhận được feedback\n", id);
                    break;
                }
                
                if (strcmp(feedback, "GOOD") == 0) {
                    accepted++;
                    printf("[worker%d] ✅ Share accepted | %s | Total: %d\n",
                           id, format_hashrate(hashrate), accepted);
                } else if (strncmp(feedback, "BAD,", 4) == 0) {
                    rejected++;
                    printf("[worker%d] ❌ Rejected: %s (rej=%d)\n",
                           id, feedback+4, rejected);
                } else if (strcmp(feedback, "BLOCK") == 0) {
                    printf("[worker%d] ⛓️ NEW BLOCK FOUND!\n", id);
                } else {
                    printf("[worker%d] ℹ️ %s\n", id, feedback);
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
        
        close(sock);
        
        if (g_running) {
            fprintf(stderr, "[worker%d] ⚠️ Mất kết nối, reconnect sau 2s...\n", id);
            sleep(2);
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
    printf("   🦀 Duino-Coin C Miner v2.0 (FIXED)\n");
    printf("========================================\n");
    printf("Username:   %s\n", cfg.username);
    printf("Difficulty: %s\n", cfg.difficulty);
    printf("Rig:        %s\n", cfg.rig_identifier);
    printf("Threads:    %d\n", cfg.thread_count);
    printf("========================================\n\n");
    
    if (!is_safe_mining_key(cfg.mining_key)) {
        fprintf(stderr, "\n❌ MINING KEY KHÔNG HỢP LỆ!\n");
        fprintf(stderr, "Vui lòng sửa lại mining key trong file config.txt\n");
        fprintf(stderr, "Lưu ý: Mining key KHÔNG được chứa dấu phẩy (,) hoặc ký tự xuống dòng.\n");
        return 1;
    }
    
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
