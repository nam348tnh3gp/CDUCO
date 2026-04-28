/*
 * DUINO-COIN C MINER for WINDOWS 3.1 (Win16) – HTTP direct version
 * 
 * Compile: cl /AM /G2 /Os duino.c dsha1.c winsock.lib
 *          (dùng Large memory model nếu cần)
 */

#define STRICT
#include <windows.h>
#include <winsock.h>          /* Winsock 1.1 16-bit */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "DSHA1.h"            /* thư viện SHA1 thuần C */

/* ==================== CẤU HÌNH ==================== */
#define MAX_WORKERS 4         /* số worker ảo tối đa (mỗi timer 1 job) */

/* ==================== CẤU TRÚC DỮ LIỆU ==================== */
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
static HWND hwndMain;
static BOOL g_running = TRUE;
static PoolInfo global_pool;
static BOOL pool_initialized = FALSE;
static Config cfg;
static int current_worker = 0;

/* ==================== HÀM TIỆN ÍCH ==================== */
void safe_usleep(int ms) { Sleep(ms); }

int is_safe_mining_key(const char *key) {
    if (strchr(key, ',')) return 0;
    if (strchr(key, '\n') || strchr(key, '\r')) return 0;
    for (const char *p = key; *p; p++)
        if (iscntrl(*p) && *p != '\t') return 0;
    return 1;
}

/* Đọc file config.txt */
void read_config(void) {
    FILE *f = fopen("config.txt", "r");
    /* mặc định */
    strcpy(cfg.username, "Nam2010");
    strcpy(cfg.mining_key, "258013");
    strcpy(cfg.difficulty, "LOW");
    strcpy(cfg.rig_identifier, "Win3.1-HTTP");
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
        while (isspace(*key)) key++;
        char *end = key + strlen(key) - 1;
        while (end > key && isspace(*end)) *end-- = '\0';
        end = val + strlen(val) - 1;
        while (end > val && isspace(*end)) *end-- = '\0';
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

/* ==================== LẤY POOL QUA HTTP (port 80) ==================== */
BOOL fetch_pool_from_server(PoolInfo *pool) {
    SOCKET sock;
    struct sockaddr_in addr;
    struct hostent *hp;
    char request[512], response[2048];
    int len;

    printf("🌐 Dang lay pool tu server (HTTP)...\n");

    hp = gethostbyname("server.duinocoin.com");
    if (!hp) {
        printf("❌ Khong phan giai duoc server.duinocoin.com\n");
        return FALSE;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("❌ Khong tao duoc socket\n");
        return FALSE;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);   /* HTTP */
    memcpy(&addr.sin_addr, hp->h_addr, hp->h_length);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        printf("❌ Khong ket noi duoc den server HTTP\n");
        closesocket(sock);
        return FALSE;
    }

    /* Gửi HTTP GET đơn giản */
    strcpy(request,
           "GET /getPool HTTP/1.0\r\n"
           "Host: server.duinocoin.com\r\n"
           "User-Agent: DuinoMinerWin31/1.0\r\n"
           "Connection: close\r\n\r\n");
    send(sock, request, strlen(request), 0);

    /* Nhận toàn bộ phản hồi (đọc đến khi socket đóng) */
    memset(response, 0, sizeof(response));
    len = 0;
    int total = sizeof(response) - 1;
    while (len < total) {
        int got = recv(sock, response + len, total - len, 0);
        if (got <= 0) break;
        len += got;
    }
    response[len] = '\0';
    closesocket(sock);

    /* Tìm body (sau header \r\n\r\n) */
    char *body = strstr(response, "\r\n\r\n");
    if (!body) body = response;
    else body += 4;

    printf("📡 Phan hoi nhan duoc:\n%s\n", body);

    /* Parse JSON thủ công */
    char *ip_start = strstr(body, "\"ip\":\"");
    if (!ip_start) {
        printf("❌ Khong tim thay IP\n");
        return FALSE;
    }
    ip_start += 6;
    char *ip_end = strchr(ip_start, '"');
    if (!ip_end) return FALSE;
    int ip_len = ip_end - ip_start;
    if (ip_len >= 64) ip_len = 63;
    strncpy(pool->ip, ip_start, ip_len);
    pool->ip[ip_len] = '\0';

    char *port_start = strstr(body, "\"port\":");
    if (!port_start) {
        printf("❌ Khong tim thay PORT\n");
        return FALSE;
    }
    port_start += 7;
    pool->port = atoi(port_start);

    printf("✅ Pool nhan duoc: %s:%d\n", pool->ip, pool->port);
    return TRUE;
}

BOOL init_global_pool(void) {
    if (!pool_initialized) {
        if (!fetch_pool_from_server(&global_pool))
            return FALSE;
        pool_initialized = TRUE;
    }
    return TRUE;
}

BOOL get_pool(PoolInfo *pool) {
    if (!pool_initialized) return FALSE;
    memcpy(pool, &global_pool, sizeof(PoolInfo));
    return TRUE;
}

/* ==================== KẾT NỐI TCP ĐẾN POOL MINING ==================== */
SOCKET tcp_connect(const char *ip, int port) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) return INVALID_SOCKET;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);
    if (addr.sin_addr.s_addr == INADDR_NONE) {
        struct hostent *hp = gethostbyname(ip);
        if (!hp) { closesocket(sock); return INVALID_SOCKET; }
        memcpy(&addr.sin_addr, hp->h_addr, hp->h_length);
    }

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(sock);
        return INVALID_SOCKET;
    }

    /* Đặt timeout nhận/gửi 5 giây */
    int timeout = 5000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
    return sock;
}

BOOL send_tcp(SOCKET sock, const char *data) {
    int len = strlen(data);
    return send(sock, data, len, 0) == len;
}

BOOL recv_line(SOCKET sock, char *buffer, int size) {
    int i = 0;
    char c;
    while (i < size - 1 && recv(sock, &c, 1, 0) > 0) {
        if (c == '\n') {
            buffer[i] = '\0';
            return TRUE;
        }
        buffer[i++] = c;
    }
    buffer[i] = '\0';
    return i > 0;
}

/* ==================== HÀM BĂM SHA1 (dùng DSHA1.h) ==================== */
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
    DWORD start = GetTickCount();   /* độ phân giải ~55ms trên Win3.1 */

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
            /* dùng sprintf an toàn cho Win16 */
            char tmp[32];
            wsprintf(tmp, "%lld", nonce);
            int nonce_len = strlen(tmp);
            memcpy(buffer + base_len, tmp, nonce_len);
            buffer[base_len + nonce_len] = '\0';
        }

        sha1_string(buffer, hash);

        if (memcmp(hash, job->target, 20) == 0) {
            *elapsed_ms = (double)(GetTickCount() - start);
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

/* ==================== WORKER ẢO (gọi bởi timer) ==================== */
void do_work(int worker_id) {
    static SOCKET sock = INVALID_SOCKET;
    static int accepted = 0, rejected = 0;
    static time_t t0, last_stats;

    if (!g_running) return;

    PoolInfo pool;
    if (!get_pool(&pool)) {
        printf("[w%d] ⚠️ Pool chua san sang.\n", worker_id);
        return;
    }

    /* Kết nối lại nếu mất */
    if (sock == INVALID_SOCKET) {
        printf("[w%d] 🔌 Ket noi den %s:%d\n", worker_id, pool.ip, pool.port);
        sock = tcp_connect(pool.ip, pool.port);
        if (sock == INVALID_SOCKET) {
            printf("[w%d] ❌ Ket noi that bai, thu lai sau.\n", worker_id);
            return;
        }
        char ver[128];
        if (recv_line(sock, ver, sizeof(ver)))
            printf("[w%d] ✅ Server v%s\n", worker_id, ver);
        t0 = time(NULL);
        last_stats = t0;
    }

    /* Yêu cầu job */
    char req[256];
    wsprintf(req, "JOB,%s,%s,%s,\n", cfg.username, cfg.difficulty, cfg.mining_key);
    if (!send_tcp(sock, req)) {
        printf("[w%d] ❌ Gui JOB loi.\n", worker_id);
        closesocket(sock); sock = INVALID_SOCKET;
        return;
    }

    /* Nhận job */
    char jobline[1024];
    if (!recv_line(sock, jobline, sizeof(jobline))) {
        printf("[w%d] ❌ Khong nhan duoc job.\n", worker_id);
        closesocket(sock); sock = INVALID_SOCKET;
        return;
    }

    char *base = strtok(jobline, ",");
    char *target_hex = strtok(NULL, ",");
    char *diff_str = strtok(NULL, ",");
    if (!base || !target_hex || !diff_str) {
        printf("[w%d] ⚠️ Job format loi: %s\n", worker_id, jobline);
        return;
    }

    Job job;
    strncpy(job.base, base, 255);
    job.base[255] = '\0';
    if (strlen(target_hex) != 40) return;
    for (int i = 0; i < 20; i++)
        sscanf(target_hex + i*2, "%2hhx", &job.target[i]);
    job.diff = atoi(diff_str);

    /* Đào */
    double elapsed;
    long long nonce = solve_job(&job, &elapsed);
    if (nonce >= 0) {
        double hashrate = (elapsed > 0) ? (nonce * 1000.0 / elapsed) : 0.0;
        char result[256];
        wsprintf(result, "%lld,%.2f,CMiner,%s,,%u\n",
                nonce, hashrate, cfg.rig_identifier, worker_id);
        if (!send_tcp(sock, result)) {
            printf("[w%d] ❌ Gui ket qua loi.\n", worker_id);
            closesocket(sock); sock = INVALID_SOCKET;
            return;
        }
        char feedback[128];
        if (!recv_line(sock, feedback, sizeof(feedback))) {
            printf("[w%d] ❌ Khong co phan hoi.\n", worker_id);
            closesocket(sock); sock = INVALID_SOCKET;
            return;
        }
        if (strcmp(feedback, "GOOD") == 0) {
            accepted++;
            printf("[w%d] ✅ %s | Tong: %d good\n", worker_id, format_hashrate(hashrate), accepted);
        } else if (strncmp(feedback, "BAD,", 4) == 0) {
            rejected++;
            printf("[w%d] ❌ %s (rej=%d)\n", worker_id, feedback+4, rejected);
        } else if (strcmp(feedback, "BLOCK") == 0) {
            printf("[w%d] ⛓️ BLOCK FOUND!\n", worker_id);
        } else {
            printf("[w%d] %s\n", worker_id, feedback);
        }
        time_t now = time(NULL);
        if (now - last_stats >= 30) {
            printf("[w%d] 📊 %d good / %d bad | Uptime: %.0fs\n",
                   worker_id, accepted, rejected, difftime(now, t0));
            last_stats = now;
        }
    }
}

/* ==================== CỬA SỔ ẨN & TIMER ==================== */
LONG CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            /* Timer 50 ms – mỗi lần chạy một worker ảo */
            SetTimer(hwnd, 1, 50, NULL);
            return 0;
        case WM_TIMER:
            if (g_running) {
                do_work(current_worker);
                current_worker = (current_worker + 1) % cfg.thread_count;
            }
            return 0;
        case WM_CLOSE:
            g_running = FALSE;
            KillTimer(hwnd, 1);
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/* ==================== WINMAIN ==================== */
int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(1,1), &wsa) != 0) {
        MessageBox(NULL, "Can not start WinSock 1.1!", "Error", MB_OK);
        return 1;
    }

    read_config();
    if (!is_safe_mining_key(cfg.mining_key)) {
        MessageBox(NULL, "Mining key invalid!", "Error", MB_OK);
        WSACleanup();
        return 1;
    }

    /* Đăng ký lớp cửa sổ ảo */
    WNDCLASS wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "DuinoMinerWin31";
    if (!RegisterClass(&wc)) {
        MessageBox(NULL, "RegisterClass failed!", "Error", MB_OK);
        WSACleanup();
        return 1;
    }

    /* Tạo cửa sổ ẩn để nhận timer */
    hwndMain = CreateWindow("DuinoMinerWin31", "Duino-Coin Miner",
                             WS_OVERLAPPEDWINDOW, 0, 0, 0, 0,
                             NULL, NULL, hInstance, NULL);
    if (!hwndMain) {
        MessageBox(NULL, "CreateWindow failed!", "Error", MB_OK);
        WSACleanup();
        return 1;
    }

    printf("========================================\n");
    printf("  Duino-Coin C Miner Win16 – HTTP mode\n");
    printf("========================================\n");
    printf("User: %s  Difficulty: %s  Workers: %d\n",
           cfg.username, cfg.difficulty, cfg.thread_count);
    printf("Trying to get pool...\n");

    if (!init_global_pool()) {
        MessageBox(NULL, "Can not fetch pool info!", "Error", MB_OK);
        DestroyWindow(hwndMain);
        WSACleanup();
        return 1;
    }

    printf("Miner started. Close window or press Ctrl+C in DOS box to stop.\n");

    /* Vòng lặp message */
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    WSACleanup();
    return 0;
}
