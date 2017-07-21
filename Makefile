TARGET = pwm
OBJ = pwm.o

#export STAGING_DIR = ~/openwrt/staging_dir
#TOOLCHAIN_DIR = $(STAGING_DIR)/toolchain-mips_34kc_gcc-4.8-linaro_uClibc-0.9.33.2
#LDCFLAGS = -L$(TOOLCHAIN_DIR)/usr/lib
#LD_LIBRARY_PATH = $(TOOLCHAIN_DIR)/usr/lib

CC = mipsel-openwrt-linux-gcc

all: $(TARGET)

$(TARGET): $(OBJ)
	. ./env
	$(CC) $(LCDFLAGS) -o $@ $^

clean:
	rm -f $(TARGET) *~ \#*\# *.o
