TARGET = EBOOT
OBJS = main.o
CFLAGS = -O2 -G0 -Wall -Wextra -std=c99
CXXFLAGS = $(CFLAGS)
LIBS = -lpspgu -lpspgum -lpspctrl -lpspdisplay -lpspdebug -lpspge -lpspnet -lpspnet_apctl
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Necro Morselli: The Forgotten Path
PSP_EBOOT_ICON = assets/ICON0.PNG
BUILD_PRX = 1
PSP_FW_VERSION = 500
include $(PSPSDK)/lib/build.mak
