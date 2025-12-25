CROSS = riscv64-unknown-elf-

CC      = $(CROSS)gcc
LD      = $(CROSS)ld
OBJDUMP = $(CROSS)objdump
OBJCOPY = $(CROSS)objcopy

# ==========================================
# ==========================================
CFLAGS = -Wall -O2 -ffreestanding -mcmodel=medany -march=rv64gc -mabi=lp64 
CFLAGS += -fno-builtin -fno-omit-frame-pointer -fno-pie -no-pie
CFLAGS += -Wno-error -Wno-all -w # -w 关闭所有警告

KERNEL_LDFLAGS = -T kernel/kernel.ld
USER_LDFLAGS   = -N -e main -Ttext 0


# ==========================================
# 2. 暴力自动扫描所有源文件
# ==========================================

# 扫描 kernel 目录下所有的 .c 和 .S
KSRCS = $(wildcard kernel/*.c) $(wildcard kernel/*.S)
KOBJS = $(patsubst %.c,%.o,$(patsubst %.S,%.o,$(KSRCS)))

# 扫描 user 目录下所有的 .c 和 .S 
USRCS = $(wildcard user/*.c) $(filter-out user/usys.S, $(wildcard user/*.S))
# 把 .c .S 都换成 .o
UOBJS = $(patsubst %.c,%.o,$(patsubst %.S,%.o,$(USRCS)))
# 加上必须生成的 usys.o
UOBJS += user/usys.o

TARGET = kernel.elf

all: $(TARGET)

# ==========================================
# 3. 编译规则
# ==========================================

kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel/%.o: kernel/%.S
	$(CC) $(CFLAGS) -c $< -o $@

user/%.o: user/%.c
	$(CC) $(CFLAGS) -nostdlib -I. -c $< -o $@

user/%.o: user/%.S
	$(CC) $(CFLAGS) -nostdlib -I. -c $< -o $@

# 自动生成 usys.S
user/usys.S: user/usys.pl
	perl user/usys.pl > user/usys.S

# ==========================================
# 4. 链接 initcode
# ==========================================


user/initcode.elf: user/entry.o user/usys.o user/ulib_fake.o
	$(LD) $(USER_LDFLAGS) -o $@ user/entry.o user/usys.o user/ulib_fake.o



user/initcode.bin: user/initcode.elf
	$(OBJCOPY) -O binary $< $@

kernel/initcode.o: user/initcode.bin
	$(LD) -r -b binary $< -o $@

# ==========================================
# 5. 链接内核
# ==========================================

$(TARGET): $(KOBJS) kernel/initcode.o kernel/kernel.ld
	$(LD) $(KERNEL_LDFLAGS) $(KOBJS) kernel/initcode.o -o $(TARGET)

run: $(TARGET)
	qemu-system-riscv64 -machine virt -nographic -bios default -kernel $(TARGET)

clean:
	rm -f kernel/*.o kernel/*.d user/*.o user/usys.S user/*.elf user/*.bin kernel/initcode.o $(TARGET)
