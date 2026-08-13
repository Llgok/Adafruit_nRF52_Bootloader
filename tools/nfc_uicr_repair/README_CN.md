<h1 align="center">NFC UICR Repair</h1>

## **[English](./README.md) | 中文**

**NFC UICR Repair** 是一个一次性修复应用，用于将 nRF52832、nRF52833 和 nRF52840 的 `UICR.NFCPINS` 从 GPIO 模式恢复为 NFC 模式。修复过程不会替换板子中已安装的 Bootloader。

## 目录

- [特性](#特性)
- [安全警告](#安全警告)
- [支持目标](#支持目标)
- [编译](#编译)
- [使用方法](#使用方法)
- [输出文件](#输出文件)

## 特性

- 擦除 UICR 前检查 `NFCPINS`。
- NFC 模式已经启用时不修改 UICR。
- 根据目标芯片备份所有文档化的可写 UICR words。
- 优先恢复 Bootloader 地址和 MBR 参数页地址，再恢复其他 UICR 数据。
- 保留 `NRFFW`、`NRFHW`、`CUSTOMER`、`PSELRESET`、`APPROTECT` 以及目标芯片特有的可写配置。
- 不恢复 `NFCPINS`，使其擦除值保持 NFC 模式。
- 校验 `NFCPINS` 和每一个已恢复的 word。
- 成功后设置 `GPREGRET=0x57`，复位进入 Adafruit nRF52 Bootloader DFU 模式。
- 校验失败时停机，不会再次擦除。

## 安全警告

> [!CAUTION]
> UICR 擦除和恢复不是原子操作。如果在 UICR 已擦除、Bootloader 地址尚未恢复期间断电，已安装的 Bootloader 可能无法启动，并需要使用 SWD/J-Link 恢复。

使用前必须注意：

- 先在可以通过 SWD 恢复的测试板上验证。
- 使用稳定的 USB 电源。
- 修复期间不要断电或按 Reset。
- MCU 和 SoftDevice 内存布局必须同时匹配。
- 不要使用为其他 nRF 目标生成的文件。

## 支持目标

| 目标 | Application 起始地址 | UF2 Family | 说明 |
| --- | ---: | ---: | --- |
| `nrf52832_s132_6.1.1` | `0x26000` | `0x1B57745F` | nRF52832 与 S132 6.1.1 内存布局 |
| `nrf52833_s140_7.3.0` | `0x27000` | `0x621E937A` | nRF52833 与 S140 7.3.0 内存布局 |
| `nrf52840_s140_6.1.1` | `0x26000` | `0xADA52840` | nRF52840 与 S140 6.1.1 内存布局 |

> [!IMPORTANT]
> 一个 UF2 文件无法安全支持全部 nRF 芯片。必须使用同时匹配 MCU 和 SoftDevice 内存布局的输出文件。

> [!NOTE]
> 本仓库的 nRF52832 Bootloader 使用 UART，不支持 USB MSC/UF2。nRF52832 UF2 只能用于能够接收 family `0x1B57745F` 的其他 Bootloader，否则需要通过合适的传输方式使用生成的 HEX 或 BIN。

## 编译

### 编译全部目标

在 `tools/nfc_uicr_repair` 目录执行：

```bash
make CROSS_COMPILE=/path/to/arm-none-eabi- all-targets
```

### 编译单个目标

```bash
make CROSS_COMPILE=/path/to/arm-none-eabi- \
  TARGET=nrf52840_s140_6.1.1
```

编译产物位于仓库根目录：

```text
_build/nfc_uicr_repair/<target>/
```

## 使用方法

1. 进入板子现有的 UF2 Bootloader。
2. 复制与 MCU 和 SoftDevice 内存布局匹配的修复 UF2。
3. 等待 UF2 磁盘重新出现。
4. 复制正常的 Application UF2。

根据 MCU 和 SoftDevice 内存布局，选择目标名称相匹配的输出目录。

UF2 磁盘重新出现，说明已安装的 Bootloader 仍然可以启动，并且修复应用已经请求进入 DFU 模式。

## 输出文件

每个目标都会生成：

```text
nfc_uicr_repair_<target>.elf
nfc_uicr_repair_<target>.hex
nfc_uicr_repair_<target>.bin
nfc_uicr_repair_<target>.uf2
```

修复 UF2 只覆盖 Application 区域，不会替换 Bootloader 代码。修复应用启动后才会主动修改 UICR。
