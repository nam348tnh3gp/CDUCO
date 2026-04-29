/*
 * DUINO-COIN C MINER for Mac OS 9 (Classic Mac OS)
 * 
 * Yêu cầu:
 *   - Metrowerks CodeWarrior Pro 6 (hoặc mới hơn)
 *   - Mac OS 9.0 trở lên + Open Transport 2.0+
 *   - Thư viện SIOUX (console I/O) – đi kèm CodeWarrior
 *   - File DSHA1.h trong cùng thư mục
 *
 * Build:
 *   - Tạo project mới trong CodeWarrior (Mac OS C Stationery)
 *   - Thêm file duino_macos9.c + DSHA1.h
 *   - Link với: OpenTransportLib, CarbonLib (nếu có), SIOUX
 *   - Build -> ra ứng dụng PEF/CFM cho PowerPC
 */

#include <OpenTransport.h>
#include <OpenTptInternet.h>
#include <Threads.h>
#include <Timer.h>
#include <SIOUX.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "DSHA1.h"

/* ==================== CẤU HÌNH ==================== */
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
    int nice_level; // 0-19 (Mac OS 9 ảo hóa: điều chỉnh thời gian ngủ)
} Config;

typedef struct {
    int id;
    Config cfg;
    unsigned int mtid;
    EndpointRef endpoint;
} WorkerArgs;

static Config cfg;
static PoolInfo global_pool;
static int pool_initialized = 0;
static volatile int g_running = 1;
static ThreadID g_threads[8];
static int g_num_threads = 0;
static int g_sleep_ticks = 0; // Thời gian "nhường" CPU

/* ==================== HÀM ĐIỀU CHỈNH ĐỘ ƯU TIÊN ==================== */
void set_miner_priority(int nice_level) {
    if (nice_level >= 19)        g_sleep_ticks = 12;  // Nhường CPU nhiều nhất
    else if (nice_level >= 10)   g_sleep_ticks = 6;
    else if (nice_level <= 0)    g_sleep_ticks = 1;   // Gần như không nhường
    else                         g_sleep_ticks = 3;   // Mặc định

    printf("✅ Mac OS 9 Priority: nice=%d (sleep %d ticks/loop)\n", nice_level, g_sleep_ticks);
}

/* ==================== OPEN TRANSPORT HELPERS ==================== */
OSStatus InitOpenTransport(void) {
    return InitOpenTransportInContext(kInitOTForApplicationMask, NULL);
}

void CloseOpenTransport(void) {
    CloseOpenTransportInContext(NULL);
}

EndpointRef CreateTCPEndpoint(void) {
    return OTOpenEndpointInContext(
        OTCreateConfiguration(kTCPName),
        0, NULL, NULL
    );
}

OSStatus ConnectToHost(EndpointRef ep, const char *ip, int port) {
    InetHost inetHost;
    
    OTInitInetAddress(&inetHost, port, (char*)ip);
    inetHost.fAddressType = AF_INET;
    
    return OTConnect(ep, (TEndpoint*)&inetHost, NULL);
}

OSStatus SendData(EndpointRef ep, const char *data, size_t len) {
    return OTSnd(ep, (void*)data, len, 0);
}

OSStatus RecvLine(EndpointRef ep, char *buffer, size_t size) {
    size_t i = 0;
    char c;
    while (i < size - 1) {
        OTResult r = OTRcv(ep, &c, 1, NULL);
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

void CloseEndpoint(EndpointRef ep) {
    if (ep) OTCloseProvider(ep);
}

/* ==================== DNS RESOLVE ==================== */
OSStatus ResolveHost(const char *hostname, InetHost *outAddr) {
    InetHostInfo hostInfo;
    OSStatus err;
    
    err = OTInetStringToHost(hostname, &hostInfo);
    if (err == noErr && hostInfo.addrs[0]) {
        *outAddr = hostInfo.addrs[0];
        return noErr;
    }
    return err;
}

/* ==================== LẤY POOL QUA HTTP ==================== */
int fetch_pool_from_server(PoolInfo *pool) {
    EndpointRef ep;
    InetHost addr;
    OSStatus err;
    char request[512], response[2048];
    int total = 0;
    
    printf("Dang lay pool tu server (HTTP)...\n");
    
    err = ResolveHost("server.duinocoin.com", &addr);
    if (err != noErr) {
        printf("Khong phan giai duoc server.\n");
        return 0;
    }
    
    ep = CreateTCPEndpoint();
    if (!ep) return 0;
    
    err = ConnectToHost(ep, "server.duinocoin.com", 80);
    if (err != noErr) {
        CloseEndpoint(ep);
        return 0;
    }
    
    strcpy(request,
           "GET /getPool HTTP/1.0\r\n"
           "Host: server.duinocoin.com\r\n"
           "User-Agent: DuinoMinerMacOS9/1.0\r\n"
           "Connection: close\r\n\r\n");
    SendData(ep, request, strlen(request));
    
    memset(response, 0, sizeof(response));
    while (total < (int)sizeof(response) - 1) {
        int r = OTRcv(ep, response + total, sizeof(response) - 1 - total, NULL);
        if (r <= 0) break;
        total += r;
    }
    response[total] = '\0';
    CloseEndpoint(ep);
    
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

/* ==================== SOLVE JOB (giống POSIX) ==================== */
static inline void sha1_string(const char *input, unsigned char *output) {
    DSHA1_CTX ctx;
    dsha1_init(&ctx);
    dsha1_write(&ctx, (const unsigned char*)input, strlen(input));
    dsha1_finalize(&ctx, output);
}

typedef struct { char base[256]; unsigned char target[20]; int diff; } Job;

static long long solve_job(const Job *job, double *elapsed_ms) {
    unsigned long start = TickCount();
    char buffer[512];
    unsigned char hash[20];
    long long max_nonce = job->diff * 100LL;
    int base_len = strlen(job->base);
    memcpy(buffer, job->base, base_len);
    int nonce_count = 0;

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
            *elapsed_ms = (TickCount() - start) * (1000.0 / 60.0);
            return nonce;
        }

        // Cơ chế "nhường" CPU: cứ mỗi 50 nonce thì sleep 1 tick
        if (++nonce_count >= 50) {
            nonce_count = 0;
            Delay(g_sleep_ticks, NULL);
        }
    }
    return -1;
}

/* ==================== WORKER THREAD (Thread Manager) ==================== */
void *worker_thread(void *param) {
    WorkerArgs *args = (WorkerArgs*)param;
    int id = args->id;
    Config cfg = args->cfg;
    EndpointRef ep = NULL;
    int accepted = 0, rejected = 0;
    
    while (g_running) {
        if (!ep) {
            printf("[w%d] Ket noi den %s:%d\n", id, global_pool.ip, global_pool.port);
            ep = CreateTCPEndpoint();
            if (ep && ConnectToHost(ep, global_pool.ip, global_pool.port) == noErr) {
                char ver[128];
                if (RecvLine(ep, ver, sizeof(ver)))
                    printf("[w%d] Server v%s\n", id, ver);
            } else {
                printf("[w%d] Ket noi that bai.\n", id);
                CloseEndpoint(ep);
                ep = NULL;
                Delay(60 * 5, NULL);  // 5 giây
                continue;
            }
        }
        
        char req[256];
        sprintf(req, "JOB,%s,%s,%s,\n", cfg.username, cfg.difficulty, cfg.mining_key);
        if (SendData(ep, req, strlen(req)) != noErr) {
            CloseEndpoint(ep); ep = NULL;
            continue;
        }
        
        char jobline[1024];
        if (!RecvLine(ep, jobline, sizeof(jobline))) {
            CloseEndpoint(ep); ep = NULL;
            continue;
        }
        
        Job job;
        char *base = strtok(jobline, ",");
        char *target_hex = strtok(NULL, ",");
        char *diff_str = strtok(NULL, ",");
        if (!base || !target_hex || !diff_str) continue;
        
        strncpy(job.base, base, 255);
        for (int i = 0; i < 20; i++)
            sscanf(target_hex + i*2, "%2hhx", &job.target[i]);
        job.diff = atoi(diff_str);
        
        double elapsed;
        long long nonce = solve_job(&job, &elapsed);
        if (nonce >= 0) {
            char result[256];
            sprintf(result, "%lld,%.2f,CMiner,%s,,%u\n",
                    nonce, (nonce * 1000.0) / elapsed, cfg.rig_identifier, args->mtid);
            SendData(ep, result, strlen(result));
            
            char feedback[128];
            if (RecvLine(ep, feedback, sizeof(feedback))) {
                if (strcmp(feedback, "GOOD") == 0) {
                    accepted++;
                    printf("[w%d] Accepted! Total: %d\n", id, accepted);
                }
            }
        }
    }
    return NULL;
}

/* ==================== MAIN ==================== */
int main(void) {
    SIOUXSettings.autocloseonquit = false;
    
    printf("\n========================================\n");
    printf("  Duino-Coin C Miner for Mac OS 9\n");
    printf("========================================\n");
    
    // Đọc config (giản lược)
    strcpy(cfg.username, "Nam2010");
    strcpy(cfg.mining_key, "258013");
    strcpy(cfg.difficulty, "LOW");
    strcpy(cfg.rig_identifier, "MacOS9");
    cfg.thread_count = 1;
    cfg.nice_level = 19; // Mặc định nhường CPU tối đa
    
    set_miner_priority(cfg.nice_level);
    
    InitOpenTransport();
    
    if (!fetch_pool_from_server(&global_pool)) {
        printf("Khong lay duoc pool!\n");
        return 1;
    }
    
    WorkerArgs args;
    args.id = 0;
    args.cfg = cfg;
    args.mtid = (unsigned int)TickCount();
    
    g_threads[0] = NewThread(kCooperativeThread, 
                            (ThreadEntryProcPtr)worker_thread, 
                            &args, 0, kCreateIfNeeded, NULL, NULL);
    g_num_threads = 1;
    
    printf("Miner started.\n");
    
    while (g_running) {
        Delay(60, NULL);
    }
    
    g_running = 0;
    DisposeThread(g_threads[0], NULL, false);
    CloseOpenTransport();
    
    printf("Miner stopped.\n");
    return 0;
}
