/*
 * @Description: nRF52832、nRF52833 和 nRF52840 NFC UICR 一次性修复程序
 * @Author: LILYGO_L
 * @Date: 2026-08-13 08:00:00
 * @LastEditTime: 2026-08-13 16:40:21
 * @License: GPL 3.0
 */

#include <stdint.h>

#include "nrf.h"

#define UICR_ERASED_VALUE 0xFFFFFFFFUL  // UICR 擦除后的默认值
#define DFU_MAGIC_UF2_RESET 0x57U        // Adafruit UF2 DFU 重启标志
#define NFCPINS_OFFSET 0x20CU            // NFCPINS 相对 UICR 基地址的偏移
#define UICR_WORD(offset) \
  (*(volatile uint32_t *)(0x10001000UL + (offset)))  // 访问指定 UICR word

#if defined(NRF52832_XXAA)
#define NRFFW_COUNT 15U            // nRF52832 NRFFW word 数量
#define HAS_DEBUGCTRL_REGOUT0 0     // nRF52832 没有 DEBUGCTRL 和 REGOUT0
#elif defined(NRF52833_XXAA) || defined(NRF52840_XXAA)
#define NRFFW_COUNT 13U            // nRF52833 和 nRF52840 NRFFW word 数量
#define HAS_DEBUGCTRL_REGOUT0 1     // nRF52833 和 nRF52840 支持对应寄存器
#else
#error "Supported targets are NRF52832_XXAA, NRF52833_XXAA and NRF52840_XXAA"
#endif

#define NRFHW_COUNT 12U       // NRFHW word 数量
#define CUSTOMER_COUNT 32U    // CUSTOMER word 数量
#define MAX_BACKUP_WORDS 70U  // 三种目标中最大的 UICR 备份项上限

// 单个已编程 UICR word 的备份信息
// 保存 UICR word 相对基地址的偏移和擦除前的值
// 用于 UICR 擦除完成后恢复原有配置
typedef struct {
  uint16_t offset;  // 相对 UICR 基地址的 word 偏移
  uint32_t value;   // 擦除前读取到的 word 值
} uicr_word_backup_t;

static uicr_word_backup_t backup[MAX_BACKUP_WORDS];  // UICR RAM 备份表
static uint32_t backup_count;                        // 当前有效备份项数量

/**
 * @brief 等待 NVMC 完成当前擦写操作。
 */
static void wait_for_nvmc(void) {
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
  }
}

/**
 * @brief 备份一个非擦除状态的 UICR word。
 * @param offset 目标 word 相对 UICR 基地址的偏移。
 */
static void backup_word(uint32_t offset) {
  uint32_t const value = UICR_WORD(offset);  // 当前 UICR word 值

  if (value != UICR_ERASED_VALUE) {
    backup[backup_count].offset = (uint16_t)offset;
    backup[backup_count].value = value;
    ++backup_count;
  }
}

/**
 * @brief 连续备份一段 UICR word。
 * @param first_offset 第一个 word 相对 UICR 基地址的偏移。
 * @param count 需要检查的 word 数量。
 */
static void backup_range(uint32_t first_offset, uint32_t count) {
  for (uint32_t i = 0; i < count; ++i) {  // 当前连续区域 word 序号
    backup_word(first_offset + i * sizeof(uint32_t));
  }
}

/**
 * @brief 收集当前芯片所有文档化可写 UICR word，NFCPINS 除外。
 */
static void collect_uicr_backup(void) {
#if defined(NRF52832_XXAA)
  backup_range(0x000U, 3U);  // nRF52832 UNUSED0~UNUSED2
  backup_word(0x010U);       // nRF52832 UNUSED3
#endif

  backup_range(0x014U, NRFFW_COUNT);  // Nordic firmware 保留配置
  backup_range(0x050U, NRFHW_COUNT);  // Nordic hardware 保留配置
  backup_range(0x080U, CUSTOMER_COUNT);  // 用户自定义 UICR 数据
  backup_range(0x200U, 2U);              // PSELRESET[0~1]
  backup_word(0x208U);                   // APPROTECT

#if HAS_DEBUGCTRL_REGOUT0
  backup_word(0x210U);  // DEBUGCTRL
  backup_word(0x304U);  // REGOUT0
#endif
}

/**
 * @brief 写入一个 UICR word 并等待 NVMC 完成。
 * @param offset 目标 word 相对 UICR 基地址的偏移。
 * @param value 需要写入的值。
 */
static void write_uicr_word(uint32_t offset, uint32_t value) {
  UICR_WORD(offset) = value;
  wait_for_nvmc();
}

/**
 * @brief 如果指定偏移存在备份，则立即恢复该 UICR word。
 * @param offset 目标 word 相对 UICR 基地址的偏移。
 */
static void restore_offset_if_present(uint32_t offset) {
  for (uint32_t i = 0; i < backup_count; ++i) {  // 当前备份项序号
    if (backup[i].offset == offset) {
      write_uicr_word(offset, backup[i].value);
      return;
    }
  }
}

/**
 * @brief 恢复已备份 UICR 数据，并优先恢复 Bootloader 启动字段。
 */
static void restore_uicr(void) {
  NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
  wait_for_nvmc();

  restore_offset_if_present(0x014U);  // MBR Bootloader 地址
  restore_offset_if_present(0x018U);  // MBR 参数页地址

  for (uint32_t i = 0; i < backup_count; ++i) {  // 当前备份项序号
    uint32_t const offset = backup[i].offset;  // 当前待恢复 UICR 偏移

    if ((offset != 0x014U) && (offset != 0x018U)) {
      write_uicr_word(offset, backup[i].value);
    }
  }

  NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
  wait_for_nvmc();
}

/**
 * @brief 校验 NFCPINS NFC 模式和全部已恢复 UICR word。
 * @return 校验成功返回 1，失败返回 0。
 */
static int verify_uicr(void) {
  if ((UICR_WORD(NFCPINS_OFFSET) & UICR_NFCPINS_PROTECT_Msk) !=
      (UICR_NFCPINS_PROTECT_NFC << UICR_NFCPINS_PROTECT_Pos)) {
    return 0;
  }

  for (uint32_t i = 0; i < backup_count; ++i) {  // 当前备份项序号
    if (UICR_WORD(backup[i].offset) != backup[i].value) {
      return 0;
    }
  }

  return 1;
}

/**
 * @brief 设置 Adafruit UF2 DFU 标志并执行系统复位。
 */
static void enter_dfu(void) {
  NRF_POWER->GPREGRET = DFU_MAGIC_UF2_RESET;
  __DSB();
  NVIC_SystemReset();
}

/**
 * @brief 执行一次 NFCPINS 修复流程。
 * @return 正常流程通过系统复位离开；校验失败时停机，不返回。
 */
int main(void) {
  if ((UICR_WORD(NFCPINS_OFFSET) & UICR_NFCPINS_PROTECT_Msk) ==
      (UICR_NFCPINS_PROTECT_NFC << UICR_NFCPINS_PROTECT_Pos)) {
    enter_dfu();
  }

  collect_uicr_backup();

  NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Een << NVMC_CONFIG_WEN_Pos;
  wait_for_nvmc();
  NRF_NVMC->ERASEUICR = NVMC_ERASEUICR_ERASEUICR_Erase;
  wait_for_nvmc();
  NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
  wait_for_nvmc();

  // 不恢复 NFCPINS，使擦除后的 PROTECT 位保持 NFC 模式。
  restore_uicr();

  if (verify_uicr()) {
    enter_dfu();
  }

  // 校验失败后停机，避免重复擦除 UICR 或启动未知应用。
  __disable_irq();
  while (1) {
    __WFE();
  }
}
