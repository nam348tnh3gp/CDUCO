/*
 * Duino-Coin C Miner for Windows 32-bit (Win95+)
 * Cross‑compile with: i686-w64-mingw32-gcc -O2 -o duino.exe duino.c -lws2_32 -lwininet -static
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <wininet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "DSHA1.h"   /* của bạn */

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "wininet.lib")

typedef struct { char ip[64]; int port; } PoolInfo;

typedef struct {
    char username[64], mining_key[64], difficulty[16], rig_identifier[64];
    int thread_count, nice_level;
} Config;

static volatile int g_running = 1;
static CRITICAL_SECTION pool_cs;
static PoolInfo global_pool;
static int pool_initialized = 0;

BOOL WINAPI console_handler(DWORD ctrl_type) {
    if (ctrl_type == CTRL_C_EVENT || ctrl_type == CTRL_CLOSE_EVENT ||
        ctrl_type == CTRL_BREAK_EVENT || ctrl_type == CTRL_LOGOFF_EVENT ||
        ctrl_type == CTRL_SHUTDOWN_EVENT) {
        g_running = 0;
        Sleep(500);
        return TRUE;
    }
    return FALSE;
}

int is_safe_mining_key(const char *key) {
    if (strchr(key, ',')) return 0;
    for (const char *p = key; *p; p++)
        if (iscntrl(*p) && *p != '\t') return 0;
    return 1;
}

void set_miner_priority(int nice_level) {
    DWORD prio = NORMAL_PRIORITY_CLASS;
    if (nice_level > 0) prio = BELOW_NORMAL_PRIORITY_CLASS;
    else if (nice_level < 0) prio = HIGH_PRIORITY_CLASS;
    SetPriorityClass(GetCurrentProcess(), prio);
    printf("Priority set (nice=%d)\n", nice_level);
}

int read_config(const char *filename, Config *cfg) {
    FILE *f = fopen(filename, "r");
    if (!f) return 0;
    char line[256];
    cfg->nice_level = 0;
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '#') continue;
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = line, *val = eq + 1;
        while (isspace(*key)) key++;
        char *end = key + strlen(key) - 1;
        while (end > key && isspace(*end)) *end-- = '\0';
        while (isspace(*val)) val++;
        end = val + strlen(val) - 1;
        while (end > val && isspace(*end)) *end-- = '\0';
        if (strcmp(key, "username") == 0) strncpy(cfg->username, val, 63);
        else if (strcmp(key, "mining_key") == 0) strncpy(cfg->mining_key, val, 63);
        else if (strcmp(key, "difficulty") == 0) strncpy(cfg->difficulty, val, 15);
        else if (strcmp(key, "rig_identifier") == 0) strncpy(cfg->rig_identifier, val, 63);
        else if (strcmp(key, "thread_count") == 0) cfg->thread_count = atoi(val);
        else if (strcmp(key, "nice_level") == 0) cfg->nice_level = atoi(val);
    }
    fclose(f);
    return 1;
}

int fetch_pool_from_server(PoolInfo *pool) {
    HINTERNET hInet = InternetOpen("DuinoMiner", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInet) return 0;
    HINTERNET hUrl = InternetOpenUrl(hInet, "http://server.duinocoin.com/getPool", NULL, 0,
                                     INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!hUrl) { InternetCloseHandle(hInet); return 0; }

    char buf[2048] = {0};
    DWORD bytesRead;
    if (!InternetReadFile(hUrl, buf, sizeof(buf)-1, &bytesRead) || bytesRead == 0) {
        InternetCloseHandle(hUrl); InternetCloseHandle(hInet);
        return 0;
    }
    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInet);

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

int init_global_pool(void) {
    EnterCriticalSection(&pool_cs);
    if (!pool_initialized) {
        if (!fetch_pool_from_server(&global_pool)) {
            LeaveCriticalSection(&pool_cs);
            return 0;
        }
        pool_initialized = 1;
    }
    LeaveCriticalSection(&pool_cs);
    return 1;
}

int get_pool(PoolInfo *pool) {
    EnterCriticalSection(&pool_cs);
    if (!pool_initialized) { LeaveCriticalSection(&pool_cs); return 0; }
    memcpy(pool, &global_pool, sizeof(PoolInfo));
    LeaveCriticalSection(&pool_cs);
    return 1;
}

SOCKET tcp_connect(const char *ip, int port) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) return INVALID_SOCKET;
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);
    if (addr.sin_addr.s_addr == INADDR_NONE) {
        struct hostent *h = gethostbyname(ip);
        if (!h) { closesocket(sock); return INVALID_SOCKET; }
        memcpy(&addr.sin_addr, h->h_addr, h->h_length);
    }
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(sock); return INVALID_SOCKET;
    }
    int timeout = 5000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
    return sock;
}

int send_tcp(SOCKET sock, const char *data) {
    int len = (int)strlen(data);
    return send(sock, data, len, 0) == len;
}

int recv_line(SOCKET sock, char *buffer, int size) {
    int i = 0;
    char c;
    while (i < size - 1 && recv(sock, &c, 1, 0) > 0) {
        if (c == '\n') { buffer[i] = '\0'; return 1; }
        buffer[i++] = c;
    }
    buffer[i] = '\0';
    return i > 0;
}

static inline void sha1_string(const char *input, unsigned char *output) {
    DSHA1_CTX ctx;
    dsha1_init(&ctx);
    dsha1_write(&ctx, (const unsigned char*)input, strlen(input));
    dsha1_finalize(&ctx, output);
}

typedef struct { char base[256]; unsigned char target[20]; int diff; } Job;

static long long solve_job(const Job *job, double *elapsed_ms) {
    LARGE_INTEGER freq, start, end;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);

    char buffer[512];
    unsigned char hash[20];
    long long max_nonce = job->diff * 100LL;
    int base_len = (int)strlen(job->base);
    memcpy(buffer, job->base, base_len);

    for (long long nonce = 0; nonce <= max_nonce; nonce++) {
        if (nonce < 10) { buffer[base_len] = '0' + nonce; buffer[base_len+1] = '\0'; }
        else if (nonce < 100) {
            buffer[base_len] = '0' + nonce/10;
            buffer[base_len+1] = '0' + nonce%10;
            buffer[base_len+2] = '\0';
        } else {
            sprintf(buffer + base_len, "%lld", nonce);
        }
        sha1_string(buffer, hash);
        if (memcmp(hash, job->target, 20) == 0) {
            QueryPerformanceCounter(&end);
            *elapsed_ms = (double)(end.QuadPart - start.QuadPart) * 1000.0 / freq.QuadPart;
            return nonce;
        }
    }
    return -1;
}

static const char* format_hashrate(double h) {
    static char buf[64];
    if (h >= 1e9) sprintf(buf, "%.2f GH/s", h/1e9);
    else if (h >= 1e6) sprintf(buf, "%.2f MH/s", h/1e6);
    else if (h >= 1e3) sprintf(buf, "%.2f kH/s", h/1e3);
    else sprintf(buf, "%.2f H/s", h);
    return buf;
}

typedef struct {
    int id;
    Config cfg;
    unsigned int mtid;
} WorkerArgs;

DWORD WINAPI worker_thread(LPVOID param) {
    WorkerArgs *args = (WorkerArgs*)param;
    int id = args->id;
    Config cfg = args->cfg;
    unsigned int mtid = args->mtid;

    if (id > 0) Sleep(min(id*100, 5000));

    while (g_running) {
        PoolInfo pool;
        if (!get_pool(&pool)) { Sleep(1000); continue; }
        SOCKET sock = tcp_connect(pool.ip, pool.port);
        if (sock == INVALID_SOCKET) { Sleep(5000); continue; }

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
            if (strlen(target_hex) != 40) continue;
            for (int i = 0; i < 20; i++) sscanf(target_hex + i*2, "%2hhx", &job.target[i]);
            job.diff = atoi(diff_str);

            double elapsed;
            long long nonce = solve_job(&job, &elapsed);
            if (nonce >= 0) {
                double hashrate = (nonce * 1000.0) / elapsed;
                char result[256];
                sprintf(result, "%lld,%.2f,CMiner,%s,,%u\n", nonce, hashrate, cfg.rig_identifier, mtid);
                if (!send_tcp(sock, result)) break;

                char feedback[128];
                if (!recv_line(sock, feedback, sizeof(feedback))) break;

                if (strcmp(feedback, "GOOD") == 0) {
                    accepted++;
                    printf("[w%d] ✅ %s | good: %d\n", id, format_hashrate(hashrate), accepted);
                } else if (strncmp(feedback, "BAD,", 4) == 0) {
                    rejected++;
                    printf("[w%d] ❌ %s (rej=%d)\n", id, feedback+4, rejected);
                } else if (strcmp(feedback, "BLOCK") == 0)
                    printf("[w%d] ⛓️ BLOCK!\n", id);
                else printf("[w%d] %s\n", id, feedback);

                time_t now = time(NULL);
                if (now - last_stats >= 30) {
                    printf("[w%d] Stats: %d good / %d bad | Uptime: %.0fs\n",
                           id, accepted, rejected, difftime(now, t0));
                    last_stats = now;
                }
            }
        }
        closesocket(sock);
        if (g_running) Sleep(2000);
    }
    return 0;
}

int main(void) {
    SetConsoleCtrlHandler(console_handler, TRUE);

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa)) return 1;

    InitializeCriticalSection(&pool_cs);

    Config cfg;
    if (!read_config("config.txt", &cfg)) {
        strcpy(cfg.username, "Nam2010");
        strcpy(cfg.mining_key, "258013");
        strcpy(cfg.difficulty, "LOW");
        strcpy(cfg.rig_identifier, "Win32");
        cfg.thread_count = 2;
        cfg.nice_level = 0;
    }
    if (cfg.thread_count < 1) cfg.thread_count = 1;
    else if (cfg.thread_count > 32) cfg.thread_count = 32;

    printf("Duino-Coin C Miner Win32\n");
    printf("User: %s  Diff: %s  Threads: %d\n", cfg.username, cfg.difficulty, cfg.thread_count);

    if (!is_safe_mining_key(cfg.mining_key)) { WSACleanup(); return 1; }

    set_miner_priority(cfg.nice_level);

    if (!init_global_pool()) { printf("Failed to get pool.\n"); WSACleanup(); return 1; }

    srand((unsigned)time(NULL));
    unsigned int mtid = rand() % 90000 + 10000;

    HANDLE *threads = malloc(sizeof(HANDLE) * cfg.thread_count);
    WorkerArgs *args = malloc(sizeof(WorkerArgs) * cfg.thread_count);

    for (int i = 0; i < cfg.thread_count; i++) {
        args[i].id = i;
        args[i].cfg = cfg;
        args[i].mtid = mtid;
        threads[i] = CreateThread(NULL, 0, worker_thread, &args[i], 0, NULL);
    }

    WaitForMultipleObjects(cfg.thread_count, threads, TRUE, INFINITE);

    for (int i = 0; i < cfg.thread_count; i++) CloseHandle(threads[i]);
    free(threads); free(args);
    DeleteCriticalSection(&pool_cs);
    WSACleanup();
    return 0;
}
