CC = clang
LD = ld.lld
BUILD_DIR = build
TARGET = $(BUILD_DIR)/ed-os-kernel.elf
ISO = $(BUILD_DIR)/ed-os.iso
CFLAGS = --target=x86_64-unknown-none-elf -std=c11 -O2 -Wall -Wextra -Werror \
         -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie \
         -mno-red-zone -mcmodel=kernel
ASFLAGS = --target=x86_64-unknown-none-elf -ffreestanding -fno-pic -fno-pie
LDFLAGS = -nostdlib -static -z max-page-size=0x1000 -T linker.ld

.PHONY: all iso clean
all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/entry.o: src/entry.S | $(BUILD_DIR)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel.o: src/kernel.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(BUILD_DIR)/entry.o $(BUILD_DIR)/kernel.o linker.ld
	$(LD) $(LDFLAGS) -o $@ $(BUILD_DIR)/entry.o $(BUILD_DIR)/kernel.o

iso: $(TARGET)
	@test -n "$(LIMINE_DIR)" || (echo "Set LIMINE_DIR to a Limine binary release directory"; exit 1)
	LIMINE_DIR="$(LIMINE_DIR)" ./scripts/build-iso.sh "$(TARGET)" "$(ISO)"

clean:
	rm -rf $(BUILD_DIR)
