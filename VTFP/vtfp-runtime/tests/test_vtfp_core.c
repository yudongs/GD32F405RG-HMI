/**
 * @file    test_vtfp_core.c
 * @brief   Host-pc tests for vtfp_core poll loop
 * @note    Uses a mock shared-RAM region in process memory. The mock
 *          header at g_mock_header is what vtfp_poll() reads from and
 *          writes to. Tests write a fake command to g_mock_header,
 *          call vtfp_poll(), and check the response.
 */

#include <stdio.h>
#include <string.h>
#include "vtfp/vtfp.h"
#include "vtfp_core.h"  /* for g_header */
#include "vtfp_command.h"
#include "vtfp_result.h"
#include "vtfp_header.h"

#ifdef _WIN32
  #include <windows.h>
#else
  #include <sys/mman.h>
  #include <unistd.h>
#endif

static int failures = 0;

#define EXPECT(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL: %s\n", msg); failures++; } \
    else        { printf("OK:   %s\n", msg); }                      \
} while (0)

#define MOCK_RAM_SIZE  (64 + 1024)

/* Mock shared RAM region: 4KB header + 1KB data.
 * NOTE: must live in the low 32-bit address space because vtfp_config_t
 * carries addresses as uint32_t (designed for the embedded target).
 * We allocate explicitly at a low fixed address on each host OS. */
static uint8_t *g_mock_ram = NULL;

/* Allocate a low-32-bit-addressable buffer. Returns 0 on success. */
static int mock_ram_alloc(void) {
    if (g_mock_ram) return 0;
#ifdef _WIN32
    /* VirtualAlloc with explicit address; address must be 64K-aligned */
    g_mock_ram = (uint8_t *)VirtualAlloc((void *)0x10000000, MOCK_RAM_SIZE,
                                         MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (g_mock_ram == NULL) {
        fprintf(stderr, "VirtualAlloc failed: %lu\n", GetLastError());
        return -1;
    }
#else
    void *p = mmap((void *)0x10000000, MOCK_RAM_SIZE,
                   PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    if (p == MAP_FAILED) {
        perror("mmap");
        return -1;
    }
    g_mock_ram = (uint8_t *)p;
#endif
    /* Round-trip the address through (uint32_t)(uintptr_t) to confirm
     * the buffer is reachable in 32-bit. */
    uintptr_t a = (uintptr_t)g_mock_ram;
    if (a > 0xFFFFFFFFUL) {
        fprintf(stderr, "mock RAM at high address 0x%lx — would not fit in uint32_t\n", (unsigned long)a);
        return -1;
    }
    return 0;
}

static void mock_ram_free(void) {
    if (!g_mock_ram) return;
#ifdef _WIN32
    VirtualFree(g_mock_ram, 0, MEM_RELEASE);
#else
    munmap((void *)g_mock_ram, MOCK_RAM_SIZE);
#endif
    g_mock_ram = NULL;
}

/* Mock handler that echoes the param back as the result */
static int32_t mock_echo_handler(const vtfp_request_t *req, vtfp_response_t *resp) {
    resp->result = req->param;
    return 0;
}

/* Mock handler that requires ARM */
static int32_t mock_dangerous_handler(const vtfp_request_t *req, vtfp_response_t *resp) {
    (void)req;
    resp->result = VTFP_R_OK;
    return 0;
}

static void mock_write_command(uint8_t cmd, uint32_t param, uint32_t seq) {
    vtfp_header_t *hdr = (vtfp_header_t *)g_mock_ram;
    hdr->command  = cmd;
    hdr->param   = param;
    hdr->seq      = seq;
    hdr->result   = VTFP_R_OK;
    hdr->data_len = 0;
    hdr->flags    = 0;
    hdr->checksum = 0;
    /* Recompute checksum */
    extern int32_t vtfp_checksum_verify(const vtfp_header_t *hdr);
    extern void vtfp_checksum_write(vtfp_header_t *hdr);
    vtfp_checksum_write(hdr);
}

int main(void) {
    printf("=== vtfp_core test ===\n");

    EXPECT(mock_ram_alloc() == 0, "mock_ram_alloc at low 32-bit address");
    if (!g_mock_ram) {
        fprintf(stderr, "aborting: mock RAM allocation failed\n");
        return 1;
    }

    /* Initialize vtfp pointing at our mock RAM */
    vtfp_config_t cfg = {
        .header_base = (uint32_t)(uintptr_t)g_mock_ram,
        .data_addr   = (uint32_t)(uintptr_t)g_mock_ram + 64,
        .data_size   = 1024,
        .poll_period_ms = 20,
        .arm_timeout_ms = 2000,
        .safety_key     = 0xAA55AA55,
        .auto_disarm    = true,
    };
    EXPECT(vtfp_init(&cfg) == 0, "vtfp_init returns 0");

    /* Test 1: no command pending returns 0 */
    EXPECT(vtfp_poll() == 0, "vtfp_poll returns 0 when no command");

    /* Test 2: register and invoke a safe handler (no ARM needed) */
    EXPECT(vtfp_register_handler(0x01, mock_echo_handler, VTFP_ARM_NONE) == 0,
           "register handler 0x01 (safe)");
    mock_write_command(0x01, 0xDEADBEEF, 42);
    EXPECT(vtfp_poll() == 1, "vtfp_poll returns 1 after handling");
    EXPECT(g_header->result == 0xDEADBEEF, "echo handler set result to param");
    EXPECT(g_header->command == 0, "command cleared after handling");
    EXPECT(g_header->seq == 42, "seq preserved");

    /* Test 3: dangerous handler without ARM returns NOT_ARMED */
    EXPECT(vtfp_register_handler(0x02, mock_dangerous_handler, VTFP_ARM_REQUIRED) == 0,
           "register handler 0x02 (dangerous)");
    mock_write_command(0x02, 0, 100);
    EXPECT(vtfp_poll() == 1, "vtfp_poll returns 1 after NOT_ARMED");
    EXPECT(g_header->result == VTFP_R_NOT_ARMED, "result is NOT_ARMED");

    /* Test 4: ARM with wrong key returns SAFETY_KEY */
    mock_write_command(VTFP_CMD_ARM, 0xDEADBEEF, 0);  /* wrong key */
    EXPECT(vtfp_poll() == 1, "vtfp_poll returns 1 after failed ARM");
    EXPECT(g_header->result == VTFP_R_SAFETY_KEY, "result is SAFETY_KEY");

    /* Test 5: ARM with correct key succeeds */
    mock_write_command(VTFP_CMD_ARM, 0xAA55AA55, 0);
    EXPECT(vtfp_poll() == 1, "vtfp_poll returns 1 after successful ARM");
    EXPECT(g_header->result == VTFP_R_OK, "result is OK after ARM");

    /* Test 6: dangerous handler now succeeds (still armed) */
    mock_write_command(0x02, 0, 200);
    EXPECT(vtfp_poll() == 1, "vtfp_poll returns 1 after dangerous cmd");
    EXPECT(g_header->result == VTFP_R_OK, "dangerous cmd succeeded");
    /* After auto_disarm, ARM should be cleared */
    mock_write_command(0x02, 0, 201);  /* second dangerous cmd */
    EXPECT(vtfp_poll() == 1, "vtfp_poll returns 1 after second dangerous cmd");
    EXPECT(g_header->result == VTFP_R_NOT_ARMED, "second dangerous cmd fails (auto-disarm)");

    /* Test 7: unknown command returns UNKNOWN_CMD */
    mock_write_command(0x05, 0, 0);
    EXPECT(vtfp_poll() == 1, "vtfp_poll returns 1 after UNKNOWN_CMD");
    EXPECT(g_header->result == VTFP_R_UNKNOWN_CMD, "result is UNKNOWN_CMD");

    /* Test 8: standard commands (QUERY_INFO) work */
    mock_write_command(VTFP_CMD_QUERY_INFO, 0, 0);
    EXPECT(vtfp_poll() == 1, "vtfp_poll returns 1 after QUERY_INFO");
    EXPECT(g_header->result == VTFP_R_OK, "QUERY_INFO succeeded");
    EXPECT(g_header->data_len > 8, "QUERY_INFO returned data");

    /* Test 9: DISARM works */
    mock_write_command(VTFP_CMD_ARM, 0xAA55AA55, 0);  /* re-ARM */
    vtfp_poll();
    mock_write_command(VTFP_CMD_DISARM, 0, 0);
    EXPECT(vtfp_poll() == 1, "vtfp_poll returns 1 after DISARM");
    EXPECT(g_header->result == VTFP_R_OK, "DISARM succeeded");
    /* Now dangerous should fail */
    mock_write_command(0x02, 0, 0);
    EXPECT(vtfp_poll() == 1, "vtfp_poll returns 1 after second dangerous cmd post-DISARM");
    EXPECT(g_header->result == VTFP_R_NOT_ARMED, "dangerous cmd fails after DISARM");

    if (failures) { fprintf(stderr, "\n%d check(s) failed\n", failures); mock_ram_free(); return 1; }
    printf("\nAll vtfp_core checks passed.\n");
    mock_ram_free();
    return 0;
}
