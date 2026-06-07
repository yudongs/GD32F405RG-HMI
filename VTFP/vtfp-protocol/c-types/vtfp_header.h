/**
 * @file    vtfp_header.h
 * @brief   VTFP v1 control header — shared RAM structure
 * @note    44 bytes, packed, little-endian. PC and MCU both write/read this
 *          struct at a fixed SRAM base (default 0x20000000).
 *
 * Field offsets (DO NOT CHANGE without bumping protocol MAJOR version):
 *   0..3   magic        (4)  = 'V','T','F','P' little-endian = 0x50465456
 *   4..5   version      (2)  = 0x0001 for v1
 *   6..7   features     (2)  = capability bitmap
 *   8..11  command      (4)  PC→MCU, written by PC, read+cleared by MCU
 *  12..15  result       (4)  MCU→PC, written by MCU, read+cleared by PC
 *  16..19  seq          (4)  monotonic transaction id (PC increments)
 *  20..23  param        (4)  command-specific argument
 *  24..27  data_addr    (4)  SRAM address of .vtfp_data segment
 *  28..31  data_len     (4)  valid bytes in data segment (≤ 1024 by default)
 *  32..35  flags        (4)  safety_key + status bits
 *  36..39  checksum     (4)  CRC-32 over bytes 0..35 (excluding this + reserved)
 *  40..43  reserved     (4)  protocol extension — MUST be 0 on wire
 *
 * Total: 44 bytes. `sizeof(vtfp_header_t)` MUST equal 44.
 */

#ifndef __VTFP_HEADER_H
#define __VTFP_HEADER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* VTFP v1 magic: 'V' 'T' 'F' 'P' as little-endian uint32_t */
#define VTFP_MAGIC              0x50465456UL

/* VTFP v1 protocol version (1) */
#define VTFP_VERSION            0x0001U

/* Default header placement: start of SRAM on STM32F4 (192KB SRAM @ 0x20000000) */
#define VTFP_DEFAULT_RAM_BASE   0x20000000UL

/* Feature bitmap (header.features field) */
#define VTFP_FEAT_LCD_CAPTURE   (1U << 0)  /* LCD screen capture available */
#define VTFP_FEAT_RTT_LOG        (1U << 1)  /* RTT log streaming available */
#define VTFP_FEAT_OTA_HOOK       (1U << 2)  /* OTA upgrade hook available */

/* Result codes (header.result field) — also defined in vtfp_result.h */
#define VTFP_OK                 0U
#define VTFP_ERR_NOT_ARMED      1U
#define VTFP_ERR_ARM_EXPIRED    2U
#define VTFP_ERR_SAFETY_KEY     3U
#define VTFP_ERR_PARAM_RANGE    4U
#define VTFP_ERR_NOT_INIT       5U
#define VTFP_ERR_BUF_OVERFLOW   6U
#define VTFP_ERR_UNKNOWN_CMD    0xFFU

/**
 * @brief  VTFP v1 control header
 * @note   Packed: byte-for-byte identical layout across compilers.
 *         All multi-byte fields are little-endian on the wire.
 */
typedef struct __attribute__((packed)) {
    uint32_t magic;           /* VTFP_MAGIC (0x50465456) */
    uint16_t version;         /* VTFP_VERSION (0x0001) */
    uint16_t features;        /* VTFP_FEAT_* bitmap */
    volatile uint32_t command;  /* PC writes, MCU reads+clears */
    volatile uint32_t result;   /* MCU writes, PC reads+clears */
    uint32_t seq;             /* PC monotonic counter */
    uint32_t param;           /* command-specific */
    uint32_t data_addr;       /* SRAM address of data segment */
    uint32_t data_len;        /* valid bytes in data segment */
    uint32_t flags;           /* safety_key / status */
    uint32_t checksum;        /* CRC-32 over bytes 0..35 */
    uint32_t reserved;        /* MUST be 0 on wire */
} vtfp_header_t;

/* Compile-time size assertion: header MUST be exactly 44 bytes */
_Static_assert(sizeof(vtfp_header_t) == 44,
               "vtfp_header_t must be 44 bytes; check field packing");

#ifdef __cplusplus
}
#endif

#endif /* __VTFP_HEADER_H */
