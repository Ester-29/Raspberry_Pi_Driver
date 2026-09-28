# Makefile - build soft_rtc (dung chung cho Ubuntu VM va Raspberry Pi)
#
# File nay duoc make doc HAI lan:
#   Lan 1: ban go "make" -> KERNELRELEASE rong -> chay nhanh "else".
#   Lan 2: Kbuild cua kernel doc lai file nay (KERNELRELEASE da co gia tri)
#          -> chi can biet obj-m: can build module nao.

ifneq ($(KERNELRELEASE),)
# ---------- Phan Kbuild doc ----------
# obj-m: build thanh module nap duoc (.ko). obj-y se la build dinh vao kernel.
obj-m := soft_rtc.o
# Chi can tren may ao x86: tao platform_device thay cho Device Tree
obj-m += soft_rtc_dev.o

else
# ---------- Phan ban goi 'make' ----------
# Cay build cua kernel DANG CHAY (goi linux-headers cung cap)
KDIR ?= /lib/modules/$(shell uname -r)/build
# Thu muc chua source module nay
PWD := $(CURDIR)

all: modules dtbo rtc_test

# -C $(KDIR): vao cay kernel de dung Makefile/.config/header cua kernel do
# M=$(PWD)  : bao Kbuild build module "ngoai cay" nam o thu muc nay
modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# Bien dich Device Tree overlay: .dts (van ban) -> .dtbo (nhi phan)
dtbo: soft-rtc.dtbo
soft-rtc.dtbo: soft-rtc-overlay.dts
	dtc -@ -I dts -O dtb -o $@ $<

# Chuong trinh test o user-space (gcc thuong, khong lien quan Kbuild)
rtc_test: rtc_test.c
	$(CC) -Wall -O2 -o $@ $<

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f soft-rtc.dtbo rtc_test

.PHONY: all modules dtbo clean
endif
