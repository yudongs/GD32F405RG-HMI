/**
 * @file    vtfp_dispatch.c
 * @brief   VTFP standard command handlers
 */

#include "vtfp_dispatch.h"
#include "vtfp_command.h"
#include "vtfp_result.h"
#include "vtfp_header.h"
#include <string.h>

#ifndef VTFP_RUNTIME_PRODUCT_ID
#define VTFP_RUNTIME_PRODUCT_ID   "VTFP-1.0"
#endif

#ifndef VTFP_RUNTIME_FW_VERSION
#define VTFP_RUNTIME_FW_VERSION   "0.1.0"
#endif

#ifndef VTFP_RUNTIME_FEATURES
#define VTFP_RUNTIME_FEATURES    0U
#endif

uint16_t vtfp_runtime_features(void)        { return VTFP_RUNTIME_FEATURES; }
const char *vtfp_runtime_product_id(void)  { return VTFP_RUNTIME_PRODUCT_ID; }
const char *vtfp_runtime_fw_version(void)  { return VTFP_RUNTIME_FW_VERSION; }

static int32_t handle_query_info(const vtfp_request_t *req, vtfp_response_t *resp) {
    (void)req;
    const char *product_id = vtfp_runtime_product_id();
    const char *fw_version = vtfp_runtime_fw_version();
    size_t pid_len = strlen(product_id) + 1;
    size_t fw_len  = strlen(fw_version) + 1;
    size_t total   = 8 + pid_len + fw_len;
    if (total > 256) {
        return (int32_t)VTFP_R_BUF_OVERFLOW;
    }
    uint8_t *p = resp->data;
    p[0] = (uint8_t)(VTFP_VERSION & 0xFF);
    p[1] = (uint8_t)((VTFP_VERSION >> 8) & 0xFF);
    p[2] = (uint8_t)(VTFP_RUNTIME_FEATURES & 0xFF);
    p[3] = (uint8_t)((VTFP_RUNTIME_FEATURES >> 8) & 0xFF);
    p[4] = (uint8_t)(pid_len & 0xFF);
    p[5] = (uint8_t)((pid_len >> 8) & 0xFF);
    p[6] = (uint8_t)(fw_len & 0xFF);
    p[7] = (uint8_t)((fw_len >> 8) & 0xFF);
    memcpy(p + 8, product_id, pid_len);
    memcpy(p + 8 + pid_len, fw_version, fw_len);
    resp->data_len = (uint32_t)total;
    return 0;
}

static int32_t handle_query_state(const vtfp_request_t *req, vtfp_response_t *resp) {
    (void)req;
    if (resp->data_len < 8) {
        return (int32_t)VTFP_R_BUF_OVERFLOW;
    }
    memset(resp->data, 0, 8);
    resp->data_len = 8;
    return 0;
}

int32_t vtfp_dispatch_standard(uint8_t cmd_id, const vtfp_request_t *req, vtfp_response_t *resp) {
    switch (cmd_id) {
        case VTFP_CMD_QUERY_INFO:  return handle_query_info(req, resp);
        case VTFP_CMD_QUERY_STATE: return handle_query_state(req, resp);
        default:                   return -1;  /* not a standard command we handle */
    }
}
