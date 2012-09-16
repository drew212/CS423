EXTRA_CFLAGS +=
KERNEL_SRC:= /lib/modules/$(shell uname -r)/build
SUBDIR= $(PWD)
GCC:=gcc
RM:=rm

.PHONY : clean

all: clean modules mp1app

obj-m:= mp1.o

mp1app: mp1app.c
	gcc mp1app.c -o mp1app

modules:
	$(MAKE) -C $(KERNEL_SRC) M=$(SUBDIR) modules

clean:
	$(RM) -f *.ko *.o *.mod.c Module.symvers modules.order mp1app
