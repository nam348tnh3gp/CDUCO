/*
 * DUINO-COIN C MINER – Cross‑platform: Windows 3.1 (Win16) + Linux
 *
 * Build Windows 3.1 (16-bit):
 *   cl /AM /G2 /Os duino.c dsha1.c winsock.lib
 *
 * Build Linux:
 *   gcc -O2 -s -o duino duino.c -lpthread
 */

#ifdef _WIN32
    /* ========== Win16 ========== */
    #define STRICT
    #include <windows.h>
    #include <winsock.h>
    #define sock_t SOCKET
    #define INVALID_SOCK -1          /* INVALID_SOCKET đã có trong winsock.h nhưng ta ánh xạ lại */
    #define sclose closesocket
    #define snprintf _snprintf
#else
    /* ========== Linux ========== */
    #define _POSIX_C_SOURCE 199309L
    #include <unistd.h>
    #include <pthread.h>
    #include <sys/socket.h>
    #include <netdb.h>
    #include <arpa/inet.h>
    #include <signal.h>
    #include <sys/time.h>
    #include <sys/resource.h>
    #include <errno.h>
    #define sock_t int
    #define INVALID_SOCK -1
    #define sclose close
    #define BOOL int
    #define TRUE 1
    #define FALSE 0
    #define CALLBACK
    #define WINAPI
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "DSHA1.h"

/* ==================== CẤU HÌNH ==================== */
#define MAX_WORKERS 4

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

/* ==================== BIẾN TOÀN CỤC ==================== */
static int g_running = 1;
static PoolInfo global_pool;
static int pool_initialized = 0;
static Config cfg;
static int current_worker = 0;

#ifdef _WIN32
static HWND hwndMain;
#else
static pthread_mutex_t pool_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/* ==================== HÀM TIỆN ÍCH ==================== */
void safe_usleep(int ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
#endif
}

double get_ticks_ms(void) {
#ifdef _WIN32
    return (double)GetTickCount();
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
#endif
}

int is_safe_mining_key(const char *key) {
    if (strchr(key, ',')) return 0;
    if (strchr(key, '\n') || strchr(key, '\r')) return 0;
    for (const char *p = key; *p; p++)
        if (iscntrl((unsigned char)*p) && *p != '\t') return 0;
    return 1;
}

/* ==================== ĐỌC CONFIG ==================== */
void read_config(void) {
    FILE *f = fopen("config.txt", "r");
    strcpy(cfg.username, "Nam2010");
    strcpy(cfg.mining_key, "258013");
    strcpy(cfg.difficulty, "LOW");
    strcpy(cfg.rig_identifier, "CrossMiner");
    cfg.thread_count = 1;
    cfg.nice_level = 0;

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
        end = val + strlen(val) - 1;
        while (end > val && isspace((unsigned char)*end)) *end-- = '\0';
        if (strcmp(key, "username") == 0) strncpy(cfg.username, val, 63);
        else if (strcmp(key, "mining_key") == 0) strncpy(cfg.mining_key, val, 63);
        else if (strcmp(key, "difficulty") == 0) strncpy(cfg.difficulty, val, 15);
        else if (strcmp(key, "rig_identifier") == 0) strncpy(cfg.rig_identifier, val, 63);
        else if (strcmp(key, "thread_count") == 0) cfg.thread_count = atoi(val);
        else if (strcmp(key, "nice_level") == 0) cfg.nice_level = atoi(val);
    }
    fclose(f);
    if (cfg.thread_count < 1) cfg.thread_count = 1;
    if (cfg.thread_count > MAX_WORKERS) cfg.thread_count = MAX_WORKERS;
}

/* ==================== LẤY POOL QUA HTTP ==================== */
int fetch_pool_from_server(PoolInfo *pool) {
    sock_t sock;
    struct sockaddr_in addr;
    struct hostent *hp;
    char request[512], response[2048];
    int len;

    printf("Dang lay pool tu server (HTTP)...\n");

    hp = gethostbyname("server.duinocoin.com");
    if (!hp) {
        printf("Khong phan giai duoc server.\n");
        return 0;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("Khong tao duoc socket.\n");
        return 0;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    memcpy(&addr.sin_addr, hp->h_addr_list[0], hp->h_length);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        printf("Khong ket noi duoc den server HTTP.\n");
        sclose(sock);
        return 0;
    }

    strcpy(request,
           "GET /getPool HTTP/1.0\r\n"
           "Host: server.duinocoin.com\r\n"
           "User-Agent: DuinoMiner/1.0\r\n"
           "Connection: close\r\n\r\n");
    send(sock, request, strlen(request), 0);

    memset(response, 0, sizeof(response));
    len = 0;
    int total = sizeof(response) - 1;
    while (len < total) {
        int got = recv(sock, response + len, total - len, 0);
        if (got <= 0) break;
        len += got;
    }
    response[len] = '\0';
    sclose(sock);

    char *body = strstr(response, "\r\n\r\n");
    if (!body) body = response;
    else body += 4;

    char *ip_start = strstr(body, "\"ip\":\"");
    if (!ip_start) return 0;
    ip_start += 6;
    char *ip_end = strchr(ip_start, '"');
    if (!ip_end) return 0;
    int ip_len = ip_end - ip_start;
    if (ip_len >= 64) ip_len = 63;
    strncpy(pool->ip, ip_start, ip_len);
    pool->ip[ip_len] = '\0';

    char *port_start = strstr(body, "\"port\":");
    if (!port_start) return 0;
    port_start += 7;
    pool->port = atoi(port_start);

    printf("✅ Pool: %s:%d\n", pool->ip, pool->port);
    return 1;
}

int init_global_pool(void) {
    if (!pool_initialized) {
        if (!fetch_pool_from_server(&global_pool))
            return 0;
        pool_initialized = 1;
    }
    return 1;
}

int get_pool(PoolInfo *pool) {
    if (!pool_initialized) return 0;
    memcpy(pool, &global_pool, sizeof(PoolInfo));
    return 1;
}

/* ==================== KẾT NỐI TCP ==================== */
sock_t tcp_connect(const char *ip, int port) {
    sock_t sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return INVALID_SOCK;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

#ifdef _WIN32
    addr.sin_addr.s_addr = inet_addr(ip);
    if (addr.sin_addr.s_addr == INADDR_NONE) {
        struct hostent *hp = gethostbyname(ip);
        if (!hp) { sclose(sock); return INVALID_SOCK; }
        memcpy(&addr.sin_addr, hp->h_addr_list[0], hp->h_length);
    }
#else
    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1) {
        struct hostent *hp = gethostbyname(ip);
        if (!hp) { sclose(sock); return INVALID_SOCK; }
        memcpy(&addr.sin_addr, hp->h_addr_list[0], hp->h_length);
    }
    struct timeval tv = {5, 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        sclose(sock);
        return INVALID_SOCK;
    }

#ifdef _WIN32
    int timeout = 5000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
#endif
    return sock;
}

int send_tcp(sock_t sock, const char *data) {
    int len = strlen(data);
    return send(sock, data, len, 0) == len;
}

int recv_line(sock_t sock, char *buffer, int size) {
    int i = 0;
    char c;
    while (i < size - 1 && recv(sock, &c, 1, 0) > 0) {
        if (c == '\n') {
            buffer[i] = '\0';
            return 1;
        }
        buffer[i++] = c;
    }
    buffer[i] = '\0';
    return i > 0;
}

/* ==================== SHA1 ==================== */
static inline void sha1_string(const char *input, unsigned char *output) {
    DSHA1_CTX ctx;
    dsha1_init(&ctx);
    dsha1_write(&ctx, (const unsigned char*)input, strlen(input));
    dsha1_finalize(&ctx, output);
}

/* ==================== GIẢI JOB ==================== */
typedef struct {
    char base[256];
    unsigned char target[20];
    int diff;
} Job;

static long long solve_job(const Job *job, double *elapsed_ms) {
    double start = get_ticks_ms();
    char buffer[512];
    unsigned char hash[20];
    long long max_nonce = job->diff * 100LL;
    int base_len = strlen(job->base);
    memcpy(buffer, job->base, base_len);

    for (long long nonce = 0; nonce <= max_nonce; nonce++) {
        if (nonce < 10) {
            buffer[base_len] = '0' + nonce;
            buffer[base_len+1] = '\0';
        } else if (nonce < 100) {
            buffer[base_len] = '0' + nonce/10;
            buffer[base_len+1] = '0' + nonce%10;
            buffer[base_len+2] = '\0';
        } else {
            sprintf(buffer + base_len, "%lld", nonce);
        }

        sha1_string(buffer, hash);
        if (memcmp(hash, job->target, 20) == 0) {
            *elapsed_ms = get_ticks_ms() - start;
            return nonce;
        }
    }
    return -1;
}

static const char* format_hashrate(double h) {
    static char buf[64];
    if (h >= 1e9)      sprintf(buf, "%.2f GH/s", h/1e9);
    else if (h >= 1e6) sprintf(buf, "%.2f MH/s", h/1e6);
    else if (h >= 1e3) sprintf(buf, "%.2f kH/s", h/1e3);
    else               sprintf(buf, "%.2f H/s", h);
    return buf;
}

/* ==================== WORKER ẢO ==================== */
void do_work(int worker_id) {
    static sock_t sock = INVALID_SOCK;
    static int accepted = 0, rejected = 0;
    static time_t t0, last_stats;

    if (!g_running) return;

    PoolInfo pool;
    if (!get_pool(&pool)) {
        printf("[w%d] Pool chua san sang.\n", worker_id);
        return;
    }

    if (sock < 0) {
        printf("[w%d] Ket noi den %s:%d\n", worker_id, pool.ip, pool.port);
        sock = tcp_connect(pool.ip, pool.port);
        if (sock < 0) {
            printf("[w%d] Ket noi that bai.\n", worker_id);
            return;
        }
        char ver[128];
        if (recv_line(sock, ver, sizeof(ver)))
            printf("[w%d] ✅ Server v%s\n", worker_id, ver);
        t0 = time(NULL);
        last_stats = t0;
    }

    char req[256];
    sprintf(req, "JOB,%s,%s,%s,\n", cfg.username, cfg.difficulty, cfg.mining_key);
    if (!send_tcp(sock, req)) {
        printf("[w%d] Gui JOB loi.\n", worker_id);
        sclose(sock); sock = INVALID_SOCK;
        return;
    }

    char jobline[1024];
    if (!recv_line(sock, jobline, sizeof(jobline))) {
        printf("[w%d] Khong nhan job.\n", worker_id);
        sclose(sock); sock = INVALID_SOCK;
        return;
    }

    char *base = strtok(jobline, ",");
    char *target_hex = strtok(NULL, ",");
    char *diff_str = strtok(NULL, ",");
    if (!base || !target_hex || !diff_str) return;

    Job job;
    strncpy(job.base, base, 255);
    job.base[255] = '\0';
    if (strlen(target_hex) != 40) return;
    int i;
    for (i = 0; i < 20; i++)
        sscanf(target_hex + i*2, "%2hhx", &job.target[i]);
    job.diff = atoi(diff_str);

    double elapsed;
    long long nonce = solve_job(&job, &elapsed);
    if (nonce >= 0) {
        double hashrate = (elapsed > 0) ? (nonce * 1000.0 / elapsed) : 0.0;
        char result[256];
        sprintf(result, "%lld,%.2f,CMiner,%s,,%u\n",
                nonce, hashrate, cfg.rig_identifier, worker_id);
        if (!send_tcp(sock, result)) {
            printf("[w%d] Gui ket qua loi.\n", worker_id);
            sclose(sock); sock = INVALID_SOCK;
            return;
        }
        char feedback[128];
        if (!recv_line(sock, feedback, sizeof(feedback))) {
            printf("[w%d] Khong co phan hoi.\n", worker_id);
            sclose(sock); sock = INVALID_SOCK;
            return;
        }
        if (strcmp(feedback, "GOOD") == 0) {
            accepted++;
            printf("[w%d] ✅ %s | good: %d\n", worker_id, format_hashrate(hashrate), accepted);
        } else if (strncmp(feedback, "BAD,", 4) == 0) {
            rejected++;
            printf("[w%d] ❌ %s (rej=%d)\n", worker_id, feedback+4, rejected);
        } else {
            printf("[w%d] %s\n", worker_id, feedback);
        }
        time_t now = time(NULL);
        if (now - last_stats >= 30) {
            printf("[w%d] %d good / %d bad | Uptime: %.0fs\n",
                   worker_id, accepted, rejected, difftime(now, t0));
            last_stats = now;
        }
    }
}

/* ==================== VÒNG LẶP CHÍNH (Linux: dùng thread) ==================== */
#ifdef _WIN32
/* Win16: timer + window procedure */
LONG CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            SetTimer(hwnd, 1, 50, NULL);
            return 0;
        case WM_TIMER:
            if (g_running) {
                do_work(current_worker);
                current_worker = (current_worker + 1) % cfg.thread_count;
            }
            return 0;
        case WM_CLOSE:
            g_running = 0;
            KillTimer(hwnd, 1);
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
#else
/* Linux: thread riêng chạy worker theo timer */
static void *worker_loop(void *arg) {
    (void)arg;
    while (g_running) {
        do_work(current_worker);
        current_worker = (current_worker + 1) % cfg.thread_count;
        safe_usleep(50);
    }
    return NULL;
}

static void signal_handler(int sig) {
    (void)sig;
    g_running = 0;
}
#endif

/* ==================== MAIN ==================== */
#ifdef _WIN32
int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
#else
int main(void)
#endif
{
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(1,1), &wsa) != 0) {
        MessageBox(NULL, "WinSock 1.1 khong san sang!", "Error", MB_OK);
        return 1;
    }
#endif

    read_config();
    if (!is_safe_mining_key(cfg.mining_key)) {
        fprintf(stderr, "Mining key khong hop le!\n");
        return 1;
    }

#ifdef _WIN32
    WNDCLASS wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "DuinoMiner";
    if (!RegisterClass(&wc)) {
        WSACleanup();
        return 1;
    }
    hwndMain = CreateWindow("DuinoMiner", "Duino-Coin Miner",
                             WS_OVERLAPPEDWINDOW, 0, 0, 0, 0,
                             NULL, NULL, hInstance, NULL);
    if (!hwndMain) {
        WSACleanup();
        return 1;
    }
#endif

    printf("========================================\n");
    printf("  Duino-Coin C Miner (cross Win16/Linux)\n");
    printf("========================================\n");
    printf("User: %s  Diff: %s  Workers: %d\n",
           cfg.username, cfg.difficulty, cfg.thread_count);

    if (!init_global_pool()) {
        fprintf(stderr, "Khong the lay pool info!\n");
        return 1;
    }

    printf("Miner started. Press Ctrl+C to stop.\n");

#ifdef _WIN32
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    WSACleanup();
#else
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    pthread_t tid;
    pthread_create(&tid, NULL, worker_loop, NULL);
    while (g_running) safe_usleep(200);
    g_running = 0;
    pthread_join(tid, NULL);
#endif

    printf("Miner stopped.\n");
    return 0;
}
