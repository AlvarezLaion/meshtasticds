# Makefile for meshtastic3ds
# Compatible with devkitPro 3DS toolchain

TARGET	:= meshtastic3ds
BUILD_DIR	:= build

# Protocol settings from src/modules/BridgeModule.cpp
# BRIDGE_PORT is used by the 3DS client to connect to the mesh bridge
CXXFLAGS	+= -DBRIDGE_PORT=4444

# Toolchain settings
ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO environment variable properly")
endif

# Include and library paths
LIBCTRU ?= $(DEVKITPRO)/libctru
C_INCLUDES	:= -I$(CURDIR) -I$(LIBCTRU)/include
LIBDIRS		:= -L$(LIBCTRU)/lib

# Libraries to link (graphics removed for console-only)
LIBS	:= -lctru -lstdc++ -lm

# Source files
# Automatically find all .cpp and .c files in the current directory
CPPFILES	:= $(wildcard *.cpp)
CFILES		:= $(wildcard *.c)

# Object files
OBJS	:= $(addprefix $(BUILD_DIR)/, $(CFILES:.c=.o)) $(addprefix $(BUILD_DIR)/, $(CPPFILES:.cpp=.o))

# Compiler and Linker
CC	:= $(DEVKITARM)/bin/arm-none-eabi-gcc
CXX	:= $(DEVKITARM)/bin/arm-none-eabi-g++
LD	:= $(DEVKITARM)/bin/arm-none-eabi-g++

# Flags
# 3DS CPU is ARM11 MPCore = armv6k architecture; 3dsx_crt0.o lives in armv6k/fpu/
CFLAGS	:= -g -Wall -O2 -march=armv6k -mfpu=vfp -mfloat-abi=hard -mtp=soft
CXXFLAGS	+= $(CFLAGS)

# Standard linker flags for 3DS
DEVKITARM_LIBDIR := $(DEVKITARM)/arm-none-eabi/lib/armv6k/fpu
LDFLAGS	:= -march=armv6k -mfpu=vfp -mfloat-abi=hard -mtp=soft -specs=3dsx.specs -B$(DEVKITARM_LIBDIR) -Wl,--gc-sections

# Output files
ELF	:= $(BUILD_DIR)/$(TARGET).elf
BIN	:= $(BUILD_DIR)/$(TARGET).3dsx
CIA	:= $(BUILD_DIR)/$(TARGET).cia

.PHONY: all clean cia

all: $(BIN)

$(BIN): $(ELF)
	@mkdir -p $(BUILD_DIR)
	$(DEVKITPRO)/tools/bin/3dsxtool $(ELF) $(BIN)

$(ELF): $(OBJS)
	$(LD) $(LDFLAGS) $(LIBDIRS) $(OBJS) $(LIBS) -o $(ELF)

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(C_INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(C_INCLUDES) -c $< -o $@

cia: $(BIN)
	@echo "CIA packaging not supported without makerom; .3dsx is the primary output."
	@echo "Use the .3dsx file for Citra or Homebrew Launcher."
	@cp $(BIN) $(CIA) 2>/dev/null || true

clean:
	rm -rf $(BUILD_DIR)
