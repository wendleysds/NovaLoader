TARGET_PLATFORM ?= bios
ARCH ?= x86
CROSS_COMPILE ?=

# tools

CPP     = $(CC) -E
AS      = nasm
CC      = $(CROSS_COMPILE)gcc
LD      = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy

# dirs

SRC_DIR = src

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
BIN_DIR:=$(BUILD_DIR)/bin

# Flags

INCLUDE := -Iinclude -I$(SRC_DIR)/arch/$(ARCH)/$(TARGET_PLATFORM)/include

CFLAGS := \
	$(INCLUDE) \
	-ffreestanding \
	-nostdlib \
	-nostdinc \
	-nostartfiles \
	-nodefaultlibs

# Language / warnings
CFLAGS += \
	-std=gnu11 \
	-Wall \
	-Werror \
	-Wno-int-to-pointer-cast \
	-Wno-attribute-alias \
	-Wno-cpp \
	-Wno-unused-function \
	-Wno-unused-parameter \
	-Wno-address-of-packed-member

# Optimization
CFLAGS += \
	-Os \
	-fstrength-reduce \
	-ffunction-sections \
	-fdata-sections

# Code generation / binary layout
CFLAGS += \
	-fno-strict-aliasing \
	-fno-align-jumps \
	-fno-align-functions \
	-fno-align-loops \
	-fno-align-labels \
	-fno-asynchronous-unwind-tables \
	-fno-unwind-tables \
	-fno-ident \
	-fno-pic \
	-fno-stack-protector \
	-fno-omit-frame-pointer

# Floating-point / SIMD
CFLAGS += \
	-mno-sse \
	-mno-mmx \
	-mno-sse2 \
	-mno-3dnow \
	-mno-avx \
	-mno-sse4a

CFLAGS += -include config.h -g

ASFLAGS := -f elf32 -g

LDFLAGS := -nostdlib -m elf_i386 --no-warn-rwx-segments --gc-sections -z noexecstack

ifeq ($(TARGET_PLATFORM), bios)
    CFLAGS += -m16 -march=i386
    CFLAGS += -D_BIOS 
    CFLAGS += -Wno-array-bounds
else ifeq ($(TARGET_PLATFORM), uefi)
    CFLAGS += -m32
    CFLAGS += -D_UEFI
endif

# files

STAGE1_FILE = $(SRC_DIR)/arch/$(ARCH)/$(TARGET_PLATFORM)/stage1.asm
STAGE1_BIN = $(BIN_DIR)/stage1.bin

STAGE2_ELF = $(BIN_DIR)/stage2.elf
STAGE2_BIN = $(BIN_DIR)/stage2.bin

LD_FILE = linker.ld

C_FILES   =
ASM_FILES =

ifeq ($(TARGET_PLATFORM), bios)
    C_FILES   = $(shell find $(SRC_DIR) -type f -name "*.c" ! -path "$(SRC_DIR)/arch/$(ARCH)/uefi/*")
	ASM_FILES =  $(filter-out $(STAGE1_FILE), $(shell find $(SRC_DIR) -type f -name "*.asm" ! -path "$(SRC_DIR)/arch/$(ARCH)/uefi/*"))
else ifeq ($(TARGET_PLATFORM), uefi)
    C_FILES   = $(shell find $(SRC_DIR) -type f -name "*.c" ! -path "$(SRC_DIR)/arch/$(ARCH)/bios/*)
    ASM_FILES =  $(filter-out $(STAGE1_FILE), $(shell find $(SRC_DIR) -type f -name "*.asm" ! -path "$(SRC_DIR)/arch/$(ARCH)/bios/*"))
else
	@echo "Unknow Platform!"
	exit(1)
endif

C_OBJS   = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(C_FILES))
ASM_OBJS = $(patsubst $(SRC_DIR)/%.asm, $(OBJ_DIR)/%.asm.o, $(ASM_FILES))

# Compile rule

all: $(STAGE1_BIN) $(STAGE2_BIN)
	@echo "Build done!"

$(STAGE1_BIN): $(STAGE1_FILE)
	@mkdir -p $(dir $@)
	@echo "  AS      $@"
	@$(AS) -f bin -o $@ $<

$(STAGE2_ELF): $(C_OBJS) $(ASM_OBJS)
	@mkdir -p $(dir $@)
	@echo "  LD      $<"
	@$(LD) $(LDFLAGS) -T $(LD_FILE) -o $@ $^

$(STAGE2_BIN): $(STAGE2_ELF)
	@mkdir -p $(dir $@)
	@echo "  OBJCOPY $<"
	@$(OBJCOPY) -O binary $< $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "  CC      $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.asm.o: $(SRC_DIR)/%.asm
	@mkdir -p $(dir $@)
	@echo "  CPP     $<"
	@$(CPP) -x c -P -MMD -MP -D__ASSEMBLY__ $(CFLAGS) $< -o $(OBJ_DIR)/$*.i
	@echo "  AS      $<"
	@$(AS) $(ASFLAGS) -o $@ $(OBJ_DIR)/$*.i
	@rm -f $(OBJ_DIR)/$*.i

clear:
	rm -rf $(BUILD_DIR)
