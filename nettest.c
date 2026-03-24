#include <pspkernel.h>
#include <pspdebug.h>
#include <pspnet.h>

PSP_MODULE_INFO("NETTEST", PSP_MODULE_USER, 1, 1);

int main() {
    pspDebugScreenInit();
    if (sceNetInit(128*1024, 42, 4*1024, 42, 4*1024) < 0) {
        pspDebugScreenPrintf("sceNetInit FAILED\n");
    } else {
        pspDebugScreenPrintf("sceNetInit OK\n");
    }
    sceKernelSleepThread();
}