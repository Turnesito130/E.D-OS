CC = clang
LD = ld.lld
BUILD_DIR = build
TARGET = $(BUILD_DIR)/ed-os-kernel.elf
ISO = $(BUILD_DIR)/ed-os.iso
CFLAGS = --target=x86_64-unknown-none-elf -std=c11 -O2 -Wall -Wextra -Werror \
         -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie \
         -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -mcmodel=kernel
ASFLAGS = --target=x86_64-unknown-none-elf -ffreestanding -fno-pic -fno-pie
LDFLAGS = -nostdlib -static -z max-page-size=0x1000 -T linker.ld

.PHONY: all iso clean
all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/entry.o: src/entry.S | $(BUILD_DIR)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel.o: src/kernel.c include/memory.h include/idt.h include/serial.h include/paging.h include/keyboard.h include/irq.h include/scheduler.h include/context.h include/vfs.h include/ramfs.h include/block.h include/gdt.h include/edfs.h include/pci.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD_DIR)/irq.o: src/irq.c include/irq.h include/keyboard.h include/memory.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD_DIR)/keyboard.o: src/keyboard.c include/keyboard.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD_DIR)/pci.o: src/pci.c include/pci.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD_DIR)/paging.o: src/paging.c include/paging.h include/memory.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD_DIR)/memory.o: src/memory.c include/memory.h include/page_ownership.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD_DIR)/page-ownership.o: src/page_ownership.c include/page_ownership.h include/memory.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD_DIR)/context.o: src/context.c include/context.h include/memory.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD_DIR)/context-switch.o: src/context.S | $(BUILD_DIR)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/scheduler.o: src/scheduler.c include/scheduler.h include/context.h include/memory.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD_DIR)/vfs.o: src/vfs.c include/vfs.h include/ramfs.h include/edfs.h include/block.h include/memory.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD_DIR)/ramfs.o: src/ramfs.c include/ramfs.h include/memory.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD_DIR)/ipc.o: src/ipc.c include/ipc.h include/scheduler.h include/context.h include/memory.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD_DIR)/edfs.o: src/edfs.c include/edfs.h include/block.h include/memory.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD_DIR)/block.o: src/block.c include/block.h include/memory.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD_DIR)/buddy.o: src/buddy.c include/buddy.h include/memory.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD_DIR)/gdt.o: src/gdt.c include/gdt.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD_DIR)/gdt-load.o: src/gdt.S | $(BUILD_DIR)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/idt.o: src/idt.c include/idt.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD_DIR)/serial.o: src/serial.c include/serial.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD_DIR)/syscall.o: src/syscall.c include/syscall.h include/scheduler.h include/memory.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@


$(BUILD_DIR)/interrupts.o: src/interrupts.S | $(BUILD_DIR)
	$(CC) $(ASFLAGS) -c $< -o $@

$(TARGET): $(BUILD_DIR)/entry.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/gdt.o $(BUILD_DIR)/gdt-load.o $(BUILD_DIR)/context.o $(BUILD_DIR)/context-switch.o $(BUILD_DIR)/memory.o $(BUILD_DIR)/page-ownership.o $(BUILD_DIR)/pci.o $(BUILD_DIR)/paging.o $(BUILD_DIR)/keyboard.o $(BUILD_DIR)/irq.o $(BUILD_DIR)/block.o $(BUILD_DIR)/edfs.o $(BUILD_DIR)/buddy.o $(BUILD_DIR)/scheduler.o $(BUILD_DIR)/ipc.o $(BUILD_DIR)/ramfs.o $(BUILD_DIR)/vfs.o $(BUILD_DIR)/idt.o $(BUILD_DIR)/serial.o $(BUILD_DIR)/syscall.o $(BUILD_DIR)/interrupts.o linker.ld
	$(LD) $(LDFLAGS) -o $@ $(BUILD_DIR)/entry.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/gdt.o $(BUILD_DIR)/gdt-load.o $(BUILD_DIR)/context.o $(BUILD_DIR)/context-switch.o $(BUILD_DIR)/memory.o $(BUILD_DIR)/page-ownership.o $(BUILD_DIR)/pci.o $(BUILD_DIR)/paging.o $(BUILD_DIR)/keyboard.o $(BUILD_DIR)/irq.o $(BUILD_DIR)/block.o $(BUILD_DIR)/edfs.o $(BUILD_DIR)/buddy.o $(BUILD_DIR)/scheduler.o $(BUILD_DIR)/ipc.o $(BUILD_DIR)/ramfs.o $(BUILD_DIR)/vfs.o $(BUILD_DIR)/idt.o $(BUILD_DIR)/serial.o $(BUILD_DIR)/syscall.o $(BUILD_DIR)/interrupts.o

iso: $(TARGET)
	@test -n "$(LIMINE_DIR)" || (echo "Set LIMINE_DIR to a Limine binary release directory"; exit 1)
	LIMINE_DIR="$(LIMINE_DIR)" bash ./scripts/build-iso.sh "$(TARGET)" "$(ISO)"

clean:
	rm -rf $(BUILD_DIR)
