
ARCH=i386-elf-
CC = $(ARCH)gcc
LD=$(ARCH)ld

INCDIR	=$(SDKDIR)/drv/include

CFLAGS	= -c  -nostdinc  -I$(INCDIR) -I../include
OBJS =  tunif.o


%.o%.c:
	$(CC) $(CFLAGS) -c -o $@ $<

all:	tunif.dll 

tunif.dll: $(OBJS) $(MAKEDEP)
	$(LD) -d -r -nostdlib -o $@ $(OBJS)
	$(ARCH)objdump  -t  $@ | grep "*UND*" > reloc.lst
	cp $@ $(SDKDIR)/livecd/iso/system/drivers -fr

clean:  
	rm -f  *.bak *.lst
	rm -f *.o *.dll
	


