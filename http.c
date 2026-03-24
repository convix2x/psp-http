#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspnet.h>
#include <pspnet_inet.h>
#include <psputility.h>
#include <pspnet_apctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <pspmodulemgr.h>

PSP_MODULE_INFO("PSPHTTP", 0, 1, 1);
PSP_MAIN_THREAD_STACK_SIZE_KB(64);

#define printf pspDebugScreenPrintf
#define CHECK_ERR(func, name) \
    { \
        int res = (func); \
        if (res < 0) { \
            printf("%s FAILED: 0x%08X\n", name, res); \
            sceKernelDelayThread(10000000); \
            sceKernelExitGame(); \
        } else { \
            printf("%s: OK\n", name); \
        } \
    }

int exit_callback(int arg1, int arg2, void *common) {
    sceKernelExitGame();
    return 0;
}

int CallbackThread(SceSize args, void *argp) {
    int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}

int SetupCallbacks(void) {
    int thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0x4000, 0, 0);
    if (thid >= 0) sceKernelStartThread(thid, 0, 0);
    return thid;
}

int main() {
    SetupCallbacks();
    pspDebugScreenInit();

    printf("--- PSP HTTP ---\n");

    printf("Loading Kernel Drivers...\n");

    sceKernelLoadModule("flash0:/kd/ifman.prx", 0, NULL);
    sceKernelLoadModule("flash0:/kd/pspnet.prx", 0, NULL);
    sceKernelLoadModule("flash0:/kd/pspnet_inet.prx", 0, NULL);
    sceKernelLoadModule("flash0:/kd/pspnet_apctl.prx", 0, NULL);
    sceUtilityLoadNetModule(PSP_NET_MODULE_COMMON);
    sceUtilityLoadNetModule(PSP_NET_MODULE_INET);

    printf("Drivers active. Starting sceNetInit...\n");
    CHECK_ERR(sceNetInit(128*1024, 42, 4*1024, 42, 4*1024), "sceNetInit");
    CHECK_ERR(sceNetInetInit(), "sceNetInetInit");
    CHECK_ERR(sceNetApctlInit(0x8000, 42), "sceNetApctlInit");

    printf("Connecting to Access Point...\n");
    CHECK_ERR(sceNetApctlConnect(1), "ApctlConnect");

    int timeout = 0;
    while (1) {
        int state;
        sceNetApctlGetState(&state);
        if (state == PSP_NET_APCTL_STATE_GOT_IP) break;

        if (timeout++ > 300) {
            printf("Error: Connection Timeout!\n");
            sceKernelDelayThread(5000000);
            sceKernelExitGame();
        }
        sceKernelDelayThread(50000);
    }

    union SceNetApctlInfo info;
    if (sceNetApctlGetInfo(PSP_NET_APCTL_INFO_IP, &info) == 0) {
        printf("CONNECTED!\n");
        printf("IP: %s\n", info.ip);
        printf("URL: http://%s:8080/INDEX.HTM\n", info.ip);
    }

    int server_sock = sceNetInetSocket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        printf("Socket Fail: %d\n", server_sock);
        sceKernelSleepThread();
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    CHECK_ERR(sceNetInetBind(server_sock, (struct sockaddr *)&addr, sizeof(addr)), "Bind");
    CHECK_ERR(sceNetInetListen(server_sock, 1), "Listen");

    printf("Waiting for browsers...\n\n");
    while (1) {
        struct sockaddr_in client_addr;
        uint32_t addr_len = sizeof(client_addr);
        int client_sock = sceNetInetAccept(server_sock, (struct sockaddr *)&client_addr, &addr_len);

        if (client_sock >= 0) {
            static char buffer[2048];
            memset(buffer, 0, sizeof(buffer));

            int len = sceNetInetRecv(client_sock, buffer, sizeof(buffer) - 1, 0);
            if (len > 0) {
                printf("Request from %s\n", inet_ntoa(client_addr.sin_addr));

                if (strstr(buffer, "GET /")) {
                    char *header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n";
                    sceNetInetSend(client_sock, header, strlen(header), 0);

                    FILE *f = fopen("ms0:/PSP/GAME/PSPHTTP0/WWW/INDEX.HTM", "r");
                    if (f) {
                        int bytes;
                        while ((bytes = fread(buffer, 1, sizeof(buffer), f)) > 0) {
                            sceNetInetSend(client_sock, buffer, bytes, 0);
                        }
                        fclose(f);
                    } else {
                        char *err = "<html><body><h1>404</h1><p>Check ms0:/PSP/GAME/PSPHTTP0/WWW/INDEX.HTM</p></body></html>";
                        sceNetInetSend(client_sock, err, strlen(err), 0);
                    }
                }
            }
            sceNetInetClose(client_sock);
        }
        sceKernelDelayThread(10000); 
    }

    return 0;
}