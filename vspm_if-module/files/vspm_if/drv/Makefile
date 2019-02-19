CFILES = vspm_if_main.c vspm_if_sub.c

obj-m += vspm_if.o
vspm_if-objs := $(CFILES:.c=.o)

U_INCLUDE := -I$(PWD)
U_INCLUDE += -I$(PWD)/../include
U_INCLUDE += -I$(ANDROID_BUILD_TOP)/hardware/renesas/vspm/vspm-module/files/vspm/include/
EXTRA_CFLAGS += $(U_INCLUDE)

all:
	make -C $(KERNELDIR) M=$(PWD) modules
clean:
	make -C $(KERNELDIR) M=$(PWD) clean
install:
	$(CP) ../include/vspm_if.h $(INCSHARED)

