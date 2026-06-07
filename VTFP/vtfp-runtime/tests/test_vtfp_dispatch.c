/**
 * @file    test_vtfp_dispatch.c
 * @brief   Host-pc tests for vtfp_dispatch (standard commands)
 */

#include <stdio.h>
#include <string.h>
#include "vtfp_dispatch.h"
#include "vtfp_command.h"
#include "vtfp_result.h"

static int failures = 0;

#define EXPECT(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL: %s\n", msg); failures++; } \
    else        { printf("OK:   %s\n", msg); }                      \
} while (0)

int main(void) {
    printf("=== vtfp_dispatch test ===\n");

    /* QUERY_INFO test: response is a binary blob in .vtfp_data */
    static uint8_t resp_buf[256];
    vtfp_request_t req = { .seq = 0, .param = 0, .data = NULL, .data_len = 0 };
    vtfp_response_t resp = { .result = 0, .data = resp_buf, .data_len = sizeof(resp_buf) };

    int32_t rc = vtfp_dispatch_standard(VTFP_CMD_QUERY_INFO, &req, &resp);
    EXPECT(rc == 0, "QUERY_INFO returns 0");
    EXPECT(resp.result == VTFP_R_OK, "QUERY_INFO result is OK");
    EXPECT(resp.data_len > 8, "QUERY_INFO response is at least 8 bytes (header)");

    /* Verify the binary header */
    uint16_t protocol_version = (uint16_t)resp_buf[0] | ((uint16_t)resp_buf[1] << 8);
    uint16_t features         = (uint16_t)resp_buf[2] | ((uint16_t)resp_buf[3] << 8);
    uint16_t pid_len          = (uint16_t)resp_buf[4] | ((uint16_t)resp_buf[5] << 8);
    uint16_t fw_len           = (uint16_t)resp_buf[6] | ((uint16_t)resp_buf[7] << 8);

    EXPECT(protocol_version == VTFP_VERSION, "QUERY_INFO protocol_version matches");
    EXPECT(features == 0, "QUERY_INFO features is 0 (default)");
    EXPECT(pid_len > 0, "QUERY_INFO product_id_len is non-zero");
    EXPECT(fw_len > 0, "QUERY_INFO fw_version_len is non-zero");
    EXPECT(resp.data_len == (uint32_t)(8 + pid_len + fw_len), "QUERY_INFO total size matches");

    /* Verify the product_id is NUL-terminated and matches the default */
    const char *pid = (const char *)resp_buf + 8;
    EXPECT(strcmp(pid, "VTFP-1.0") == 0, "QUERY_INFO product_id is 'VTFP-1.0'");

    /* Verify the fw_version is NUL-terminated and matches the default */
    const char *fw = (const char *)resp_buf + 8 + pid_len;
    EXPECT(strcmp(fw, "0.1.0") == 0, "QUERY_INFO fw_version is '0.1.0'");

    /* QUERY_STATE test: response is 8-byte zero state */
    memset(resp_buf, 0xFF, sizeof(resp_buf));
    rc = vtfp_dispatch_standard(VTFP_CMD_QUERY_STATE, &req, &resp);
    EXPECT(rc == 0, "QUERY_STATE returns 0");
    EXPECT(resp.data_len == 8, "QUERY_STATE response is 8 bytes");
    int all_zero = 1;
    for (int i = 0; i < 8; i++) if (resp_buf[i] != 0) { all_zero = 0; break; }
    EXPECT(all_zero, "QUERY_STATE response is all zeros (default)");

    /* Unknown standard command */
    rc = vtfp_dispatch_standard(0x18, &req, &resp);
    EXPECT(rc == -1, "unknown standard command returns -1");

    /* Command outside standard range */
    rc = vtfp_dispatch_standard(0x42, &req, &resp);
    EXPECT(rc == -1, "non-standard command returns -1");

    if (failures) { fprintf(stderr, "\n%d check(s) failed\n", failures); return 1; }
    printf("\nAll vtfp_dispatch checks passed.\n");
    return 0;
}
