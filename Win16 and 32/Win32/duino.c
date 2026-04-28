/*
 * Duino-Coin C Miner – Cross‑platform (Windows & Linux)
 *
 * Build Linux:
 *   gcc -O2 -s -o duino duino.c -lpthread
 *
 * Build Windows (cross‑compile from Linux with MinGW‑w64):
 *   i686-w64-mingw32-gcc -O2 -s -static -o duino.exe duino.c -lws2_32 -lwininet
 */

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <winsock2.h>
    #include <wininet.h>
    #pragma comment(lib, "ws2_32.lib")
    #pragma comment(lib, "wininet.lib")
#else
    #define _POSIX_C_SOURCE 199309L
    #include <unistd.h>
    #include <pthread.h>
    #include <sys/socket.h>
    #include <netdb.h>
    #include <arpa/inet.h>
    #include <signal.h>
    #include <sys/time.h>
    #include <errno.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "DSHA1.h"

/* ====================== CÁC KIỂU DỮ LIỆU CHUNG ====================== */
typedef struct {
    char ip[64];
    int port;
} PoolInfo;

typedef struct {
    char username[64];
    char mining_key[64];
    char difficulty[16];
    char rig_identifier[64];
    int thread_count;
    int nice_level;
} Config;

/* ====================== WRAPPER HỆ ĐIỀU HÀNH ====================== */
#ifdef _WIN32
    typedef HANDLE thread_t;
    typedef CRITICAL_SECTION mutex_t;
    typedef SOCKET socket_t;
    #define INVALID_SOCKET_T INVALID_SOCKET
#else
    typedef pthread_t thread_t;
    typedef pthread_mutex_t mutex_t;
    typedef int socket_t;
    #define INVALID_SOCKET_T -1
#endif

/* ====================== BIẾN TOÀN CỤC ====================== */
static volatile int g_running = 1;
static mutex_t pool_mutex;
static PoolInfo global_pool;
static int pool_initialized = 0;

/* ====================== WRAPPER FUNCTIONS ====================== */
static void sleep_ms(int ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

static double get_time_ms(void) {
#ifdef _WIN32
    return (double)GetTickCount64();
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
#endif
}

static void mutex_init_wrapper(mutex_t *m) {
#ifdef _WIN32
    InitializeCriticalSection(m);
#else
    pthread_mutex_init(m, NULL);
#endif
}

static void mutex_lock_wrapper(mutex_t *m) {
#ifdef _WIN32
    EnterCriticalSection(m);
#else
    pthread_mutex_lock(m);
#endif
}

static void mutex_unlock_wrapper(mutex_t *m) {
#ifdef _WIN32
    LeaveCriticalSection(m);
#else
    pthread_mutex_unlock(m);
#endif
}

static void mutex_destroy_wrapper(mutex_t *m) {
#ifdef _WIN32
    DeleteCriticalSection(m);
#else
    pthread_mutex_destroy(m);
#endif
}

static thread_t thread_create(void *(*start_routine)(void*), void *arg) {
#ifdef _WIN32
    return CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)start_routine, arg, 0, NULL);
#else
    pthread_t tid;
    pthread_create(&tid, NULL, start_routine, arg);
    return tid;
#endif
}

static void thread_join(thread_t thread) {
#ifdef _WIN32
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
#else
    pthread_join(thread, NULL);
#endif
}

/* signal handler chỉ dùng trên Linux */
#ifndef _WIN32
static void signal_handler(int sig) {
    (void)sig;
    g_running = 0;
}
#endif

static void setup_signal_handler(void) {
#ifdef _WIN32
    /* Không cần thiết lập trên Windows (dùng console_handler riêng nếu cần) */
#else
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
#endif
}

/* ====================== CÁC HÀM DÙNG CHUNG ====================== */
int is_safe_mining_key(const char *key) {
    if (strchr(key, ',')) return 0;
    if (strchr(key, '\n') || strchr(key, '\r')) return 0;
    for (const char *p = key; *p; p++)
        if (iscntrl((unsigned char)*p) && *p != '\t') return 0;
    return 1;
}

void read_config(const char *filename, Config *cfg) {
    FILE *f = fopen(filename, "r");
    /* mặc định */
    strcpy(cfg->username, "Nam2010");
    strcpy(cfg->mining_key, "258013");
    strcpy(cfg->difficulty, "LOW");
    strcpy(cfg->rig_identifier, "CrossMiner");
    cfg->thread_count = 2;
    cfg->nice_level = 0;

    if (!f) return;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = line, *val = eq + 1;
        while (isspace((unsigned char)*key)) key++;
        char *end = key + strlen(key) - 1;
        while (end > key && isspace((unsigned char)*end)) *end-- = '\0';
        while (isspace((unsigned char)*val)) val++;
        end = val + strlen(val) - 1;
        while (end > val && isspace((unsigned char)*end)) *end-- = '\0';
        if (strcmp(key, "username") == 0) strncpy(cfg->username, val, 63);
        else if (strcmp(key, "mining_key") == 0) strncpy(cfg->mining_key, val, 63);
        else if (strcmp(key, "difficulty") == 0) strncpy(cfg->difficulty, val, 15);
        else if (strcmp(key, "rig_identifier") == 0) strncpy(cfg->rig_identifier, val, 63);
        else if (strcmp(key, "thread_count") == 0) cfg->thread_count = atoi(val);
        else if (strcmp(key, "nice_level") == 0) cfg->nice_level = atoi(val);
    }
    fclose(f);
    if (cfg->thread_count < 1) cfg->thread_count = 1;
    if (cfg->thread_count > 64) cfg->thread_count = 64;
}

/* ====================== LẤY POOL QUA HTTP ====================== */
#ifdef _WIN32
int fetch_pool_from_server(PoolInfo *pool) {
    HINTERNET hInet = InternetOpen("DuinoMiner", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInet) return 0;
    HINTERNET hUrl = InternetOpenUrl(hInet, "http://server.duinocoin.com/getPool", NULL, 0,
                                     INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!hUrl) { InternetCloseHandle(hInet); return 0; }

    char buf[2048] = {0};
    DWORD bytesRead;
    if (!InternetReadFile(hUrl, buf, sizeof(buf) - 1, &bytesRead) || bytesRead == 0) {
        InternetCloseHandle(hUrl);
        InternetCloseHandle(hInet);
        return 0;
    }
    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInet);

    /* parse JSON */
    char *ip_start = strstr(buf, "\"ip\":\"");
    if (!ip_start) return 0;
    ip_start += 6;
    char *ip_end = strchr(ip_start, '"');
    if (!ip_end) return 0;
    int len = ip_end - ip_start;
    if (len >= 64) len = 63;
    strncpy(pool->ip, ip_start, len);
    pool->ip[len] = '\0';

    char *port_start = strstr(buf, "\"port\":");
    if (!port_start) return 0;
    port_start += 7;
    pool->port = atoi(port_start);
    printf("Pool: %s:%d\n", pool->ip, pool->port);
    return 1;
}
#else
/* Linux: dùng socket thô */
int fetch_pool_from_server(PoolInfo *pool) {
    struct hostent *hp = gethostbyname("server.duinocoin.com");
    if (!hp) return 0;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return 0;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    memcpy(&addr.sin_addr, hp->h_addr, hp->h_length);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return 0;
    }

    const char *request =
        "GET /getPool HTTP/1.0\r\n"
        "Host: server.duinocoin.com\r\n"
        "User-Agent: DuinoMinerLinux\r\n"
        "Connection: close\r\n\r\n";
    send(sock, request, strlen(request), 0);

    char response[2048];
    int total = 0;
    while (total < (int)sizeof(response) - 1) {
        int r = recv(sock, response + total, sizeof(response) - 1 - total, 0);
        if (r <= 0) break;
        total += r;
    }
    response[total] = '\0';
    close(sock);

    char *body = strstr(response, "\r\n\r\n");
    if (!body) body = response;
    else body += 4;

    char *ip_start = strstr(body, "\"ip\":\"");
    if (!ip_start) return 0;
    ip_start += 6;
    char *ip_end = strchr(ip_start, '"');
    if (!ip_end) return 0;
    int len = ip_end - ip_start;
    if (len >= 64) len = 63;
    strncpy(pool->ip, ip_start, len);
    pool->ip[len] = '\0';

    char *port_start = strstr(body, "\"port\":");
    if (!port_start) return 0;
    port_start += 7;
    pool->port = atoi(port_start);
    printf("Pool: %s:%d\n", pool->ip, pool->port);
    return 1;
}
#endif

int init_global_pool(void) {
    mutex_lock_wrapper(&pool_mutex);
    if (!pool_initialized) {
        if (!fetch_pool_from_server(&global_pool)) {
            mutex_unlock_wrapper(&pool_mutex);
            return 0;
        }
        pool_initialized = 1;
    }
    mutex_unlock_wrapper(&pool_mutex);
    return 1;
}

int get_pool(PoolInfo *pool) {
    mutex_lock_wrapper(&pool_mutex);
    if (!pool_initialized) {
        mutex_unlock_wrapper(&pool_mutex);
        return 0;
    }
    memcpy(pool, &global_pool, sizeof(PoolInfo));
    mutex_unlock_wrapper(&pool_mutex);
    return 1;
}

/* ====================== KẾT NỐI TCP ====================== */
socket_t tcp_connect(const char *ip, int port) {
    socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
#ifdef _WIN32
    if (sock == INVALID_SOCKET) return INVALID_SOCKET_T;
#else
    if (sock < 0) return -1;
#endif

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

#ifdef _WIN32
    addr.sin_addr.s_addr = inet_addr(ip);
    if (addr.sin_addr.s_addr == INADDR_NONE) {
        struct hostent *h = gethostbyname(ip);
        if (!h) { closesocket(sock); return INVALID_SOCKET_T; }
        memcpy(&addr.sin_addr, h->h_addr, h->h_length);
    }
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(sock);
        return INVALID_SOCKET_T;
    }
    int timeout = 5000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
#else
    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1) {
        struct hostent *h = gethostbyname(ip);
        if (!h) { close(sock); return -1; }
        memcpy(&addr.sin_addr, h->h_addr, h->h_length);
    }
    struct timeval tv = {5, 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }
#endif
    return sock;
}

int send_tcp(socket_t sock, const char *data) {
    int len = strlen(data);
#ifdef _WIN32
    return send(sock, data, len, 0) == len;
#else
    return send(sock, data, len, 0) == len;
#endif
}

int recv_line(socket_t sock, char *buffer, int size) {
    int i = 0;
    char c;
    while (i < size - 1) {
#ifdef _WIN32
        int r = recv(sock, &c, 1, 0);
#else
        int r = recv(sock, &c, 1, 0);
#endif
        if (r <= 0) break;
        if (c == '\n') {
            buffer[i] = '\0';
            return 1;
        }
        buffer[i++] = c;
    }
    buffer[i] = '\0';
    return i > 0;
}

/* ====================== HÀM BĂM SHA1 ====================== */
static inline void sha1_string(const char *input, unsigned char *output) {
    DSHA1_CTX ctx;
    dsha1_init(&ctx);
    dsha1_write(&ctx, (const unsigned char*)input, strlen(input));
    dsha1_finalize(&ctx, output);
}

/* ====================== GIẢI JOB ====================== */
typedef struct {
    char base[256];
    unsigned char target[20];
    int diff;
} Job;

static long long solve_job(const Job *job, double *elapsed_ms) {
    double start = get_time_ms();

    char buffer[512];
    unsigned char hash[20];
    long long max_nonce = job->diff * 100LL;
    int base_len = strlen(job->base);
    memcpy(buffer, job->base, base_len);

    for (long long nonce = 0; nonce <= max_nonce; nonce++) {
        if (nonce < 10) {
            buffer[base_len] = '0' + nonce;
            buffer[base_len + 1] = '\0';
        } else if (nonce < 100) {
            buffer[base_len] = '0' + nonce / 10;
            buffer[base_len + 1] = '0' + nonce % 10;
            buffer[base_len + 2] = '\0';
        } else {
            sprintf(buffer + base_len, "%lld", nonce);
        }

        sha1_string(buffer, hash);
        if (memcmp(hash, job->target, 20) == 0) {
            *elapsed_ms = get_time_ms() - start;
            return nonce;
        }
    }
    return -1;
}

static const char* format_hashrate(double h) {
    static char buf[64];
    if (h >= 1e9)      sprintf(buf, "%.2f GH/s", h / 1e9);
    else if (h >= 1e6) sprintf(buf, "%.2f MH/s", h / 1e6);
    else if (h >= 1e3) sprintf(buf, "%.2f kH/s", h / 1e3);
    else               sprintf(buf, "%.2f H/s", h);
    return buf;
}

/* ====================== WORKER THREAD ====================== */
typedef struct {
    int id;
    Config cfg;
    unsigned int mtid;
} WorkerArgs;

#ifdef _WIN32
DWORD WINAPI worker_thread_func(LPVOID param) {
#else
void *worker_thread_func(void *param) {
#endif
    WorkerArgs *args = (WorkerArgs*)param;
    int id = args->id;
    Config cfg = args->cfg;
    unsigned int mtid = args->mtid;

    if (id > 0) {
        int wait_ms = id * 100;
        if (wait_ms > 5000) wait_ms = 5000;
        sleep_ms(wait_ms);
    }

    while (g_running) {
        PoolInfo pool;
        if (!get_pool(&pool)) {
            sleep_ms(1000);
            continue;
        }

        socket_t sock = tcp_connect(pool.ip, pool.port);
#ifdef _WIN32
        if (sock == INVALID_SOCKET) {
#else
        if (sock < 0) {
#endif
            sleep_ms(5000);
            continue;
        }

        char ver[128];
        if (recv_line(sock, ver, sizeof(ver)))
            printf("[w%d] Server v%s\n", id, ver);

        int accepted = 0, rejected = 0;
        time_t t0 = time(NULL), last_stats = t0;

        while (g_running) {
            char req[256];
            sprintf(req, "JOB,%s,%s,%s,\n", cfg.username, cfg.difficulty, cfg.mining_key);
            if (!send_tcp(sock, req)) break;

            char jobline[1024];
            if (!recv_line(sock, jobline, sizeof(jobline))) break;

            char *base = strtok(jobline, ",");
            char *target_hex = strtok(NULL, ",");
            char *diff_str = strtok(NULL, ",");
            if (!base || !target_hex || !diff_str) continue;

            Job job;
            strncpy(job.base, base, 255);
            job.base[255] = '\0';
            if (strlen(target_hex) != 40) continue;
            for (int i = 0; i < 20; i++)
                sscanf(target_hex + i * 2, "%2hhx", &job.target[i]);
            job.diff = atoi(diff_str);

            double elapsed;
            long long nonce = solve_job(&job, &elapsed);
            if (nonce >= 0) {
                double hashrate = (nonce * 1000.0) / (elapsed > 0 ? elapsed : 1);
                char result[256];
                sprintf(result, "%lld,%.2f,CMiner,%s,,%u\n",
                        nonce, hashrate, cfg.rig_identifier, mtid);
                if (!send_tcp(sock, result)) break;

                char feedback[128];
                if (!recv_line(sock, feedback, sizeof(feedback))) break;

                if (strcmp(feedback, "GOOD") == 0) {
                    accepted++;
                    printf("[w%d] ✅ %s | good: %d\n", id, format_hashrate(hashrate), accepted);
                } else if (strncmp(feedback, "BAD,", 4) == 0) {
                    rejected++;
                    printf("[w%d] ❌ %s (rej=%d)\n", id, feedback + 4, rejected);
                } else if (strcmp(feedback, "BLOCK") == 0) {
                    printf("[w%d] ⛓️ BLOCK!\n", id);
                } else {
                    printf("[w%d] %s\n", id, feedback);
                }

                time_t now = time(NULL);
                if (now - last_stats >= 30) {
                    double uptime = difftime(now, t0);
                    printf("[w%d] Stats: %d good / %d bad | Uptime: %.0fs\n",
                           id, accepted, rejected, uptime);
                    last_stats = now;
                }
            }
        }

#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        if (g_running) sleep_ms(2000);
    }
    return 0;
}

/* ====================== ĐIỀU CHỈNH PRIORITY ====================== */
void set_miner_priority(int nice_level) {
#ifdef _WIN32
    DWORD prio = NORMAL_PRIORITY_CLASS;
    if (nice_level > 0) prio = BELOW_NORMAL_PRIORITY_CLASS;
    else if (nice_level < 0) prio = HIGH_PRIORITY_CLASS;
    SetPriorityClass(GetCurrentProcess(), prio);
#else
    if (nice_level != 0) {
        nice_level = nice_level < -20 ? -20 : (nice_level > 19 ? 19 : nice_level);
        setpriority(PRIO_PROCESS, 0, nice_level);
    }
#endif
    printf("Priority set (nice=%d)\n", nice_level);
}

/* ====================== MAIN ====================== */
int main(void) {
    setup_signal_handler();

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa)) return 1;
#endif

    mutex_init_wrapper(&pool_mutex);

    Config cfg;
    read_config("config.txt", &cfg);

    printf("\n========================================\n");
    printf("   Duino-Coin C Miner (Cross-platform)\n");
    printf("========================================\n");
    printf("Username:   %s\n", cfg.username);
    printf("Difficulty: %s\n", cfg.difficulty);
    printf("Rig:        %s\n", cfg.rig_identifier);
    printf("Threads:    %d\n", cfg.thread_count);
    printf("Nice level: %d\n", cfg.nice_level);
    printf("========================================\n\n");

    if (!is_safe_mining_key(cfg.mining_key)) {
        fprintf(stderr, "Mining key khong an toan!\n");
        return 1;
    }

    set_miner_priority(cfg.nice_level);

    if (!init_global_pool()) {
        fprintf(stderr, "Khong the lay pool info!\n");
        return 1;
    }

    srand((unsigned)time(NULL));
    unsigned int mtid = rand() % 90000 + 10000;

    thread_t *threads = malloc(sizeof(thread_t) * cfg.thread_count);
    WorkerArgs *args = malloc(sizeof(WorkerArgs) * cfg.thread_count);

    printf("Khoi dong %d thread...\n", cfg.thread_count);
    for (int i = 0; i < cfg.thread_count; i++) {
        args[i].id = i;
        args[i].cfg = cfg;
        args[i].mtid = mtid;
        threads[i] = thread_create(worker_thread_func, &args[i]);
    }

    /* Chờ Ctrl+C (Linux) hoặc chỉ cần giữ main không thoát */
    while (g_running) {
        sleep_ms(200);
    }

    printf("\nDang dung miner...\n");
    g_running = 0;

    for (int i = 0; i < cfg.thread_count; i++) {
        thread_join(threads[i]);
    }

    free(threads);
    free(args);

    mutex_destroy_wrapper(&pool_mutex);

#ifdef _WIN32
    WSACleanup();
#endif

    printf("Miner stopped.\n");
    return 0;
}
