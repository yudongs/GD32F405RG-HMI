/**
 * @file    vtfp_core.c
 * @brief   VTFP v1 runtime — main poll loop and dispatch
 * @note    This is the entry point for the runtime. vtfp_poll() is called
 *          from the user's main loop (typically every 20ms in RT-Thread).
 *          The poll reads the shared-RAM header, validates it, dispatches
 *          to a user-registered handler, writes the result, and clears
 *          the command field so the next transaction can begin.
 */

#include "vtfp_core.h"
#include "vtfp_checksum.h"
#include "vtfp_arm.h"
#include "vtfp_command.h"
#include "vtfp_result.h"
#include "vtfp_dispatch.h"
#include <string.h>

/* Globals (declared in vtfp_core.h) */
vtfp_dispatch_entry_t g_dispatch[VTFP_MAX_HANDLERS];
vtfp_config_t         g_config;
volatile vtfp_header_t *g_header = NULL;
volatile uint8_t       *g_data   = NULL;

/* Default configuration (used when vtfp_init is called with NULL) */
static const vtfp_config_t s_default_config = {
    .header_base     = 0x20000000,
    .data_addr       = 0x20000000 + sizeof(vtfp_header_t),
    .data_size       = 1024,
    .poll_period_ms  = 20,
    .arm_timeout_ms  = 2000,
    .safety_key      = 0xAA55AA55,
    .auto_disarm     = true,
};

/* Default "now_ms" provider for ports that don't supply one.
 * REMOVED for GD32F405RG build — vtfp_port_freertos.c provides the real
 * FreeRTOS-backed implementation. The weak fallback causes a "multiply
 * defined" linker error under AC5 because AC5 does not honor the
 * __attribute__((weak)) semantics. If you port this runtime to a different
 * RTOS, supply your own vtfp_port_now_ms() implementation in the port file. */

int32_t vtfp_init(const vtfp_config_t *cfg) {
    /* Clear dispatch table */
    memset(g_dispatch, 0, sizeof(g_dispatch));

    /* Apply config (use defaults if NULL) */
    if (cfg == NULL) {
        g_config = s_default_config;
    } else {
        g_config = *cfg;
        /* Sanity-check / fill-in defaults */
        if (g_config.header_base == 0) g_config.header_base = 0x20000000;
        if (g_config.data_addr   == 0) g_config.data_addr   = g_config.header_base + sizeof(vtfp_header_t);
        if (g_config.data_size   == 0) g_config.data_size   = 1024;
        if (g_config.arm_timeout_ms == 0) g_config.arm_timeout_ms = 2000;
        if (g_config.safety_key == 0) g_config.safety_key = 0xAA55AA55;
    }

    /* Point to shared RAM */
    g_header = (volatile vtfp_header_t *)g_config.header_base;
    g_data   = (volatile uint8_t *)g_config.data_addr;

    /* Initialize header in shared RAM (first time) */
    g_header->magic    = VTFP_MAGIC;
    g_header->version  = VTFP_VERSION;
    g_header->features = 0;  /* M2 doesn't set features yet; M3 codegen will */
    g_header->command  = 0;
    g_header->result   = VTFP_OK;
    g_header->seq      = 0;
    g_header->param   = 0;
    g_header->data_addr = g_config.data_addr;
    g_header->data_len  = 0;
    g_header->flags    = 0;
    g_header->checksum = 0;
    g_header->reserved = 0;
    g_header->data_addr = g_config.data_addr;
    vtfp_checksum_write((vtfp_header_t *)g_header);

    /* Initialize ARM state machine */
    vtfp_arm_init(g_config.arm_timeout_ms, g_config.safety_key, g_config.auto_disarm);

    return 0;
}

int32_t vtfp_poll(void) {
    if (g_header == NULL) return 0;

    /* Snapshot the header (PC may be writing while we read) */
    vtfp_header_t hdr;
    memcpy((void *)&hdr, (const void *)g_header, sizeof(hdr));

    /* No command pending */
    if (hdr.command == 0) {
        return 0;
    }

    /* Validate magic + version */
    if (hdr.magic != VTFP_MAGIC) {
        /* PC wrote garbage; clear command and report UNKNOWN_CMD */
        g_header->command = 0;
        g_header->result  = VTFP_R_UNKNOWN_CMD;
        return -1;
    }
    if (hdr.version != VTFP_VERSION) {
        g_header->command = 0;
        g_header->result  = VTFP_R_UNKNOWN_CMD;
        return -1;
    }

    /* Verify checksum */
    if (vtfp_checksum_verify(&hdr) != 0) {
        g_header->command = 0;
        g_header->result  = VTFP_R_UNKNOWN_CMD;  /* or VTFP_ERR_CHECKSUM if we had one */
        return -1;
    }

    /* Handle ARM / DISARM specially */
    if (hdr.command == VTFP_CMD_ARM) {
        vtfp_arm_try_arm(hdr.param, vtfp_port_now_ms());
        g_header->command = 0;
        g_header->result  = (vtfp_arm_get_state() == VTFP_ARM_STATE_ARMED) ? VTFP_OK : VTFP_R_SAFETY_KEY;
        return 1;
    }
    if (hdr.command == VTFP_CMD_DISARM) {
        vtfp_arm_disarm();
        g_header->command = 0;
        g_header->result  = VTFP_OK;
        return 1;
    }

    /* Standard command IDs (0x10..0x1F) — dispatch via vtfp_dispatch_standard */
    if (hdr.command >= VTFP_CMD_STD_BASE && hdr.command <= VTFP_CMD_STD_MAX) {
        vtfp_request_t  req  = {
            .seq      = hdr.seq,
            .param   = hdr.param,
            .data     = (const uint8_t *)g_data,
            .data_len = hdr.data_len,
        };
        vtfp_response_t resp = {
            .result   = VTFP_OK,
            .data     = (uint8_t *)g_data,
            .data_len = 0,
        };
        int32_t rc = vtfp_dispatch_standard((uint8_t)hdr.command, &req, &resp);
        if (rc == -1) {
            g_header->command = 0;
            g_header->result  = VTFP_R_UNKNOWN_CMD;
            return -1;
        }
        g_header->result   = (rc != 0) ? (uint32_t)rc : resp.result;
        g_header->data_len = resp.data_len;
        g_header->command  = 0;
        return 1;
    }

    /* Look up the handler */
    if (hdr.command < VTFP_CMD_USER_BASE || hdr.command > VTFP_CMD_USER_MAX) {
        g_header->command = 0;
        g_header->result  = VTFP_R_UNKNOWN_CMD;
        return -1;
    }
    vtfp_dispatch_entry_t *entry = &g_dispatch[hdr.command];
    if (entry->fn == NULL) {
        g_header->command = 0;
        g_header->result  = VTFP_R_UNKNOWN_CMD;
        return 1;
    }

    /* Check ARM policy */
    if (entry->policy == VTFP_ARM_REQUIRED) {
        if (vtfp_arm_is_armed(vtfp_port_now_ms()) == 0) {
            g_header->command = 0;
            g_header->result  = VTFP_R_NOT_ARMED;
            return 1;
        }
    }

    /* Invoke the handler */
    vtfp_request_t  req  = {
        .seq      = hdr.seq,
        .param   = hdr.param,
        .data     = (const uint8_t *)g_data,
        .data_len = hdr.data_len,
    };
    vtfp_response_t resp = {
        .result   = VTFP_OK,
        .data     = (uint8_t *)g_data,
        .data_len = 0,
    };
    int32_t rc = entry->fn(&req, &resp);
    if (rc != 0) {
        resp.result = (uint32_t)rc;
    }

    /* Write result back */
    g_header->result   = resp.result;
    g_header->data_len = resp.data_len;
    g_header->command  = 0;  /* clear command so PC sees we're done */

    /* Auto-disarm after a dangerous command */
    if (entry->policy == VTFP_ARM_REQUIRED && g_config.auto_disarm) {
        vtfp_arm_disarm();
    }

    return 1;
}

const char *vtfp_version(void) {
    return "0.1.0";
}

int32_t vtfp_register_handler(uint8_t cmd, vtfp_handler_fn fn, vtfp_arm_policy_t policy) {
    if (cmd < VTFP_CMD_USER_BASE || cmd > VTFP_CMD_USER_MAX) {
        return -1;  /* out of user partition */
    }
    if (cmd >= VTFP_MAX_HANDLERS) {
        return -1;  /* table too small (shouldn't happen given VTFP_MAX_HANDLERS=32) */
    }
    g_dispatch[cmd].fn = fn;
    g_dispatch[cmd].policy = policy;
    return 0;
}
