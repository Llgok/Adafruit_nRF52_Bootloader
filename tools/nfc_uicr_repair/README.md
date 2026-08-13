<h1 align="center">NFC UICR Repair</h1>

## **English | [Chinese](./README_CN.md)**

**NFC UICR Repair** is a one-shot application for restoring `UICR.NFCPINS` from GPIO mode to NFC mode on nRF52832, nRF52833, and nRF52840 devices. It repairs UICR without replacing the installed bootloader.

## Table of Contents

- [Features](#features)
- [Safety Warning](#safety-warning)
- [Supported Targets](#supported-targets)
- [Build](#build)
- [Usage](#usage)
- [Output Files](#output-files)

## Features

- Checks `NFCPINS` before erasing UICR.
- Leaves UICR unchanged when NFC mode is already enabled.
- Backs up all documented writable UICR words for the selected target.
- Restores the bootloader and MBR parameter addresses before other UICR data.
- Preserves `NRFFW`, `NRFHW`, `CUSTOMER`, `PSELRESET`, `APPROTECT`, and target-specific writable configuration.
- Does not restore `NFCPINS`, so its erased value selects NFC mode.
- Verifies `NFCPINS` and every restored word.
- Sets `GPREGRET=0x57` and resets into Adafruit nRF52 Bootloader DFU mode after success.
- Stops without another erase if verification fails.

## Safety Warning

> [!CAUTION]
> UICR erase and restore are not atomic. Power loss after UICR erase and before the bootloader address is restored can make the installed bootloader unreachable and require SWD/J-Link recovery.

Before using this tool:

- Test it on hardware that can be recovered through SWD first.
- Use stable USB power.
- Do not disconnect power or press Reset during repair.
- Match both the MCU and SoftDevice layout.
- Do not use an output built for another nRF target.

## Supported Targets

| Target | Application Start | UF2 Family | Description |
| --- | ---: | ---: | --- |
| `nrf52832_s132_6.1.1` | `0x26000` | `0x1B57745F` | nRF52832 with the S132 6.1.1 layout |
| `nrf52833_s140_7.3.0` | `0x27000` | `0x621E937A` | nRF52833 with the S140 7.3.0 layout |
| `nrf52840_s140_6.1.1` | `0x26000` | `0xADA52840` | nRF52840 with the S140 6.1.1 layout |

> [!IMPORTANT]
> A single UF2 cannot safely support every nRF chip. Always use the output matching both the MCU and SoftDevice layout.

> [!NOTE]
> This repository's nRF52832 bootloader uses UART instead of USB MSC/UF2. The nRF52832 UF2 can only be used with another bootloader that accepts family `0x1B57745F`. Otherwise, use the generated HEX or BIN with a suitable transport.

## Build

### Build All Targets

Run from `tools/nfc_uicr_repair`:

```bash
make CROSS_COMPILE=/path/to/arm-none-eabi- all-targets
```

### Build One Target

```bash
make CROSS_COMPILE=/path/to/arm-none-eabi- \
  TARGET=nrf52840_s140_6.1.1
```

The build outputs are written under the repository root:

```text
_build/nfc_uicr_repair/<target>/
```

## Usage

1. Enter the existing UF2 bootloader.
2. Copy the repair UF2 matching the MCU and SoftDevice layout.
3. Wait for the UF2 drive to reappear.
4. Copy the normal application UF2.

Select the output directory whose target name matches the MCU and SoftDevice layout.

The UF2 drive reappearing confirms that the installed bootloader remains reachable and that the repair application requested DFU mode.

## Output Files

Each target produces:

```text
nfc_uicr_repair_<target>.elf
nfc_uicr_repair_<target>.hex
nfc_uicr_repair_<target>.bin
nfc_uicr_repair_<target>.uf2
```

The repair UF2 overwrites only the application region. It does not replace the bootloader code. UICR is modified by the repair application after it starts.
