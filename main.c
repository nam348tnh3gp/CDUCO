#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>

#include <openssl/sha.h>

// -------------------- Cấu hình --------------------
typedef struct {
    char username[64];
    char mining_key[64];
    char difficulty[16];
    char rig_identifier[64];
    int thread_count;
} Config;

// Đọc file config dạng key=value
int read_config(const char *filename, Config *cfg) {
    FILE *f = fopen(filename, "r");
    if (!f) return 0;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        // Bỏ dòng trống và comment
        if (line[0] == '\n' || line[0] == '#') continue;
        char *key = strtok(line, "=");
        char *val = strtok(NULL, "\n");
        if (!key || !val) continue;
        // Xóa khoảng trắng đầu cuối
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
    // Dùng lệnh curl để lấy JSON (có thể dùng libcurl, nhưng đơn giản dùng popen)
    FILE *fp = popen("curl -s https://server.duinocoin.com/getPool", "r");
    if (!fp) return 0;
    char buf[256];
    if (!fgets(buf, sizeof(buf), fp)) {
        pclose(fp);
        return 0;
    }
    pclose(fp);
    // Parse JSON thủ công: {"ip":"xxx","port":yyy}
    char *ip_start = strstr(buf, "\"ip\":\"");
    if (!ip_start) return 0;
    ip_start += 6;
    char *ip_end = strchr(ip_start, '"');
    if (!ip_end) return 0;
    int len = ip_end - ip_start;
    if (len >= (int)sizeof(pool->ip)) len = sizeof(pool->ip) - 1;
    strncpy(pool->ip, ip_start, len);
    pool->ip[len] = '\0';

    char *port_start = strstr(buf, "\"port\":");
    if (!port_start) return 0;
    port_start += 7;
    pool->port = atoi(port_start);
    return 1;
}

// -------------------- Hàm tính SHA1 --------------------
void sha1_string(const char *input, unsigned char *output) {
    SHA_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, input, strlen(input));
    SHA1_Final(output, &ctx);
}

// -------------------- Giải job --------------------
typedef struct {
    char base[256];
    unsigned char target[20];   // SHA1 là 20 byte
    int diff;
} Job;

// Tìm nonce thỏa mãn
long long solve_job(const Job *job, double *elapsed_ms) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    char nonce_str[32];
    unsigned char hash[20];
    long long max_nonce = job->diff * 100;
    // Dùng buffer để nối base + nonce
    char buffer[512];
    int base_len = strlen(job->base);
    memcpy(buffer, job->base, base_len);
    for (long long nonce = 0; nonce <= max_nonce; nonce++) {
        sprintf(nonce_str, "%lld", nonce);
        int nonce_len = strlen(nonce_str);
        memcpy(buffer + base_len, nonce_str, nonce_len);
        buffer[base_len + nonce_len] = '\0';

        sha1_string(buffer, hash);
        if (memcmp(hash, job->target, 20) == 0) {
            clock_gettime(CLOCK_MONOTONIC, &end);
            *elapsed_ms = (end.tv_sec - start.tv_sec) * 1000.0 +
                          (end.tv_nsec - start.tv_nsec) / 1e6;
            return nonce;
        }
    }
    return -1;
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

    while (1) {
        // Lấy pool
        PoolInfo pool;
        if (!get_pool(&pool)) {
            fprintf(stderr, "[worker%d] Không lấy được pool, thử lại sau 5s\n", id);
            sleep(5);
            continue;
        }
        printf("[worker%d] Kết nối tới %s:%d\n", id, pool.ip, pool.port);

        // Tạo socket
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            perror("[worker%d] socket");
            sleep(3);
            continue;
        }
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(pool.port);
        if (inet_pton(AF_INET, pool.ip, &addr.sin_addr) <= 0) {
            perror("[worker%d] inet_pton");
            close(sock);
            sleep(3);
            continue;
        }
        if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("[worker%d] connect");
            close(sock);
            sleep(3);
            continue;
        }

        // Đọc dòng server version
        char buf[1024];
        int n = recv(sock, buf, sizeof(buf)-1, 0);
        if (n <= 0) {
            close(sock);
            continue;
        }
        buf[n] = '\0';
        char *end = strchr(buf, '\n');
        if (end) *end = '\0';
        printf("[worker%d] Connected (server v%s)\n", id, buf);

        int accepted = 0, rejected = 0;
        time_t t0 = time(NULL);

        // Vòng lặp nhận job và xử lý
        while (1) {
            // Gửi yêu cầu job
            char req[256];
            snprintf(req, sizeof(req), "JOB,%s,%s,%s\n",
                     cfg.username, cfg.difficulty, cfg.mining_key);
            if (send(sock, req, strlen(req), 0) < 0) break;

            // Nhận job
            n = recv(sock, buf, sizeof(buf)-1, 0);
            if (n <= 0) break;
            buf[n] = '\0';
            // Xóa ký tự xuống dòng
            char *newline = strchr(buf, '\n');
            if (newline) *newline = '\0';

            // Parse job: base,target_hex,diff
            char *base = strtok(buf, ",");
            char *target_hex = strtok(NULL, ",");
            char *diff_str = strtok(NULL, ",");
            if (!base || !target_hex || !diff_str) continue;

            Job job;
            strcpy(job.base, base);
            // Giải mã target_hex thành 20 byte
            if (strlen(target_hex) != 40) continue; // SHA1 hex = 40 ký tự
            for (int i = 0; i < 20; i++) {
                sscanf(target_hex + i*2, "%2hhx", &job.target[i]);
            }
            job.diff = atoi(diff_str);

            // Giải bài toán
            double elapsed_ms;
            long long nonce = solve_job(&job, &elapsed_ms);
            if (nonce >= 0) {
                double hashrate = 1e6 * nonce / (elapsed_ms * 1000.0); // H/s
                char msg[256];
                snprintf(msg, sizeof(msg), "%lld,%.2f,RustMiner,%s,%u\n",
                         nonce, hashrate, cfg.rig_identifier, mtid);
                if (send(sock, msg, strlen(msg), 0) < 0) break;

                // Nhận feedback
                n = recv(sock, buf, sizeof(buf)-1, 0);
                if (n <= 0) break;
                buf[n] = '\0';
                char *fb = strtok(buf, "\n");
                if (!fb) continue;

                if (strcmp(fb, "GOOD") == 0) {
                    accepted++;
                    printf("[worker%d] ✅ Share accepted | %s | %d\n",
                           id, format_hashrate(hashrate), accepted);
                } else if (strncmp(fb, "BAD,", 4) == 0) {
                    rejected++;
                    printf("[worker%d] ❌ Rejected: %s (rej=%d)\n",
                           id, fb+4, rejected);
                } else if (strcmp(fb, "BLOCK") == 0) {
                    printf("[worker%d] ⛓️ New block\n", id);
                } else {
                    printf("[worker%d] ℹ️ %s\n", id, fb);
                }

                // In thống kê mỗi 10 share
                if ((accepted + rejected) % 10 == 0) {
                    time_t now = time(NULL);
                    double uptime = difftime(now, t0);
                    printf("[worker%d] 📊 Shares: %d good / %d bad | Uptime %.1fs\n",
                           id, accepted, rejected, uptime);
                    t0 = now;
                }
            }
        }
        close(sock);
        fprintf(stderr, "[worker%d] ⚠️ Mất kết nối, kết nối lại...\n", id);
        sleep(2);
    }
    return NULL;
}

// Hàm định dạng hashrate
const char* format_hashrate(double h) {
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

// -------------------- Main --------------------
int main() {
    Config cfg;
    if (!read_config("config.txt", &cfg)) {
        fprintf(stderr, "Không đọc được config.txt, dùng mặc định\n");
        strcpy(cfg.username, "Nam2010");
        strcpy(cfg.mining_key, "258013");
        strcpy(cfg.difficulty, "LOW");
        strcpy(cfg.rig_identifier, "RustRig");
        cfg.thread_count = 4;
    }

    printf("Duino-Coin C Miner\n");
    printf("Username: %s\n", cfg.username);
    printf("Difficulty: %s\n", cfg.difficulty);
    printf("Rig: %s\n", cfg.rig_identifier);
    printf("Threads: %d\n", cfg.thread_count);

    srand(time(NULL));
    unsigned int mtid = rand() % 90000 + 10000; // giống Rust: 10000-99999

    pthread_t threads[cfg.thread_count];
    WorkerArgs args[cfg.thread_count];

    for (int i = 0; i < cfg.thread_count; i++) {
        args[i].id = i;
        args[i].cfg = cfg;
        args[i].multithread_id = mtid;
        pthread_create(&threads[i], NULL, worker_thread, &args[i]);
    }

    for (int i = 0; i < cfg.thread_count; i++) {
        pthread_join(threads[i], NULL);
    }
    return 0;
}
