ROOT := ../..
CROSS_COMPILE ?= arm-none-eabi-
CC := $(CROSS_COMPILE)gcc
OBJCOPY := $(CROSS_COMPILE)objcopy
SIZE := $(CROSS_COMPILE)size
PYTHON ?= python3
TARGET ?= nrf52840_s140_6.1.1
BUILD_ROOT := $(ROOT)/_build/nfc_uicr_repair
BUILD := $(BUILD_ROOT)/$(TARGET)

ifeq ($(TARGET),nrf52832_s132_6.1.1)
  DEVICE_DEFINE := NRF52832_XXAA
  STARTUP_DEVICE := nrf52
  APP_START := 0x26000
  FLASH_END := 0x6D000
  RAM_END := 0x20010000
  UF2_FAMILY := 0x1B57745F
else ifeq ($(TARGET),nrf52833_s140_7.3.0)
  DEVICE_DEFINE := NRF52833_XXAA
  STARTUP_DEVICE := nrf52833
  APP_START := 0x27000
  FLASH_END := 0x6D000
  RAM_END := 0x20020000
  UF2_FAMILY := 0x621E937A
else ifeq ($(TARGET),nrf52840_s140_6.1.1)
  DEVICE_DEFINE := NRF52840_XXAA
  STARTUP_DEVICE := nrf52840
  APP_START := 0x26000
  FLASH_END := 0xED000
  RAM_END := 0x20040000
  UF2_FAMILY := 0xADA52840
else
  $(error Unsupported TARGET '$(TARGET)'; use nrf52832_s132_6.1.1, nrf52833_s140_7.3.0, or nrf52840_s140_6.1.1)
endif

OUT := nfc_uicr_repair_$(TARGET)
ELF := $(BUILD)/$(OUT).elf
HEX := $(BUILD)/$(OUT).hex
BIN := $(BUILD)/$(OUT).bin
UF2 := $(BUILD)/$(OUT).uf2
STARTUP := $(ROOT)/lib/nrfx/mdk/gcc_startup_$(STARTUP_DEVICE).S
SYSTEM := $(ROOT)/lib/nrfx/mdk/system_$(STARTUP_DEVICE).c

CPPFLAGS := -D$(DEVICE_DEFINE) -D__STARTUP_CLEAR_BSS -D__START=main -D__HEAP_SIZE=0 \
            -I$(ROOT)/lib/nrfx/mdk -I$(ROOT)/src/cmsis/include
CFLAGS := -mcpu=cortex-m4 -mthumb -mabi=aapcs -mfloat-abi=hard -mfpu=fpv4-sp-d16 \
          -Os -g3 -ffunction-sections -fdata-sections -fno-common \
          -Wall -Wextra -Werror -Wno-array-bounds
LDFLAGS := $(CFLAGS) -nostartfiles -specs=nosys.specs -specs=nano.specs \
           -Wl,--gc-sections -Wl,-Map,$(ELF).map \
           -Wl,--defsym=APP_START=$(APP_START) \
           -Wl,--defsym=FLASH_END=$(FLASH_END) \
           -Wl,--defsym=RAM_START=0x20000000 \
           -Wl,--defsym=RAM_END=$(RAM_END) \
           -T linker.ld

.PHONY: all all-targets clean

all: $(UF2)

all-targets:
	$(MAKE) TARGET=nrf52832_s132_6.1.1 all
	$(MAKE) TARGET=nrf52833_s140_7.3.0 all
	$(MAKE) TARGET=nrf52840_s140_6.1.1 all

$(BUILD):
	mkdir -p $@

$(BUILD)/main.o: main.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/startup.o: $(STARTUP) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/system.o: $(SYSTEM) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(ELF): $(BUILD)/main.o $(BUILD)/startup.o $(BUILD)/system.o linker.ld
	$(CC) $(filter %.o,$^) $(LDFLAGS) -o $@
	$(SIZE) $@

$(HEX): $(ELF)
	$(OBJCOPY) -O ihex $< $@

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

$(UF2): $(HEX) $(BIN)
	$(PYTHON) $(ROOT)/lib/uf2/utils/uf2conv.py -f $(UF2_FAMILY) -c -o $@ $(HEX)

clean:
	rm -rf $(BUILD_ROOT)
