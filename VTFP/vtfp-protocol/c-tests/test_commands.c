/**
 * @file    test_commands.c
 * @brief   Verify command ID partition boundaries and standard IDs are distinct
 */

#include <stdio.h>
#include "vtfp_command.h"
#include "vtfp_result.h"

static int failures = 0;

#define EXPECT(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL: %s\n", msg); failures++; } \
    else        { printf("OK:   %s\n", msg); }                      \
} while (0)

int main(void) {
    printf("=== command / result ID test ===\n");

    /* Partition boundaries are non-overlapping */
    EXPECT(vtfp_cmd_is_user(0x01) && vtfp_cmd_is_user(0x0F), "0x01..0x0F is user");
    EXPECT(vtfp_cmd_is_standard(0x10) && vtfp_cmd_is_standard(0x1F), "0x10..0x1F is standard");
    EXPECT(vtfp_cmd_is_safety(0xFE) && vtfp_cmd_is_safety(0xFF), "0xFE..0xFF is safety");

    /* Out-of-partition IDs are rejected */
    EXPECT(!vtfp_cmd_is_user(0x10) && !vtfp_cmd_is_user(0xFF), "user range rejects non-user IDs");
    EXPECT(!vtfp_cmd_is_standard(0x20) && !vtfp_cmd_is_standard(0x00), "standard range rejects non-standard IDs");
    EXPECT(!vtfp_cmd_is_safety(0x10) && !vtfp_cmd_is_safety(0xEF), "safety range rejects non-safety IDs");

    /* Standard command IDs are distinct */
    EXPECT(VTFP_CMD_QUERY_INFO != VTFP_CMD_QUERY_STATE
        && VTFP_CMD_QUERY_INFO != VTFP_CMD_RESET
        && VTFP_CMD_QUERY_INFO != VTFP_CMD_HEARTBEAT
        && VTFP_CMD_QUERY_STATE != VTFP_CMD_RESET
        && VTFP_CMD_QUERY_STATE != VTFP_CMD_HEARTBEAT
        && VTFP_CMD_RESET != VTFP_CMD_HEARTBEAT,
        "standard command IDs are pairwise distinct");

    /* Safety commands are distinct */
    EXPECT(VTFP_CMD_ARM != VTFP_CMD_DISARM, "ARM and DISARM have distinct IDs");

    /* Result codes are distinct and in 0..6 / 0xFF */
    EXPECT(VTFP_R_NOT_ARMED == 1 && VTFP_R_ARM_EXPIRED == 2
        && VTFP_R_SAFETY_KEY == 3 && VTFP_R_PARAM_RANGE == 4
        && VTFP_R_NOT_INIT == 5 && VTFP_R_BUF_OVERFLOW == 6
        && VTFP_R_UNKNOWN_CMD == 0xFF,
        "result codes match spec values");

    if (failures) { fprintf(stderr, "\n%d check(s) failed\n", failures); return 1; }
    printf("\nAll command/result checks passed.\n");
    return 0;
}
