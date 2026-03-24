TARGET = psp-http
OBJS = http.o

CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti

BUILD_PRX = 1
PSP_FW_VERSION = 600
PSP_LARGE_MEMORY = 1

LIBDIR = -L. -L$(shell psp-config --pspsdk-path)/lib
LIBS = -lpsputility -lpspnet_apctl -lpspnet_inet -lpspnet -lpspdebug -lpspdisplay

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = PSP HTTP
PSP_EBOOT_ID = PSPHTTP0

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak