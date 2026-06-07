/**
 * @file    vtfp_user_dispatch.c
 * @brief   Hand-written handlers for the abstract-controller sample
 * @note    This is what a user actually writes. The codegen
 *          (vtfp-runtime/codegen/gen_c_dispatch.py) produces a
 *          SIMILAR file with stub bodies; you replace the stubs
 *          with real implementations like the ones below.
 */

#include <vtfp/vtfp.h>
#include <string.h>

/* === Internal state (domain-neutral, for demo) === */
static struct {
    uint32_t params[4];      /* SET_PARAM storage (0..3) */
    uint32_t state_word;     /* QUERY_STATE response */
    uint8_t  output_bits;    /* TOGGLE_OUTPUT bits 0..7 */
    uint32_t button_presses; /* counter for last button press */
} s_state = {0};

/* === BEGIN user handler implementations (editable) === */

/**
 * @brief  QUERY_STATE — return the 8-byte state word
 *         (params[0] in low 32 bits, button_presses in high 32 bits)
 */
static int32_t my_query_state_handler(const vtfp_request_t *req, vtfp_response_t *resp) {
    (void)req;
    /* Pack 8 bytes: button_presses (u32 LE) + params[0] (u32 LE) */
    uint8_t *p = resp->data;
    if (resp->data_len < 8) {
        return (int32_t)VTFP_R_BUF_OVERFLOW;
    }
    p[0] = (uint8_t)(s_state.button_presses & 0xFF);
    p[1] = (uint8_t)((s_state.button_presses >> 8) & 0xFF);
    p[2] = (uint8_t)((s_state.button_presses >> 16) & 0xFF);
    p[3] = (uint8_t)((s_state.button_presses >> 24) & 0xFF);
    p[4] = (uint8_t)(s_state.params[0] & 0xFF);
    p[5] = (uint8_t)((s_state.params[0] >> 8) & 0xFF);
    p[6] = (uint8_t)((s_state.params[0] >> 16) & 0xFF);
    p[7] = (uint8_t)((s_state.params[0] >> 24) & 0xFF);
    resp->data_len = 8;
    return 0;
}

/**
 * @brief  SET_PARAM — set params[idx] (low byte of param) to (param >> 8)
 *         Encoding: param = (idx & 0xFF) | ((value & 0xFFFFFF) << 8)
 */
static int32_t my_set_param_handler(const vtfp_request_t *req, vtfp_response_t *resp) {
    (void)resp;
    uint8_t idx = (uint8_t)(req->param & 0xFF);
    uint32_t value = req->param >> 8;
    if (idx >= 4) {
        return (int32_t)VTFP_R_PARAM_RANGE;
    }
    s_state.params[idx] = value;
    return 0;
}

/**
 * @brief  TOGGLE_OUTPUT — toggle the bit (param & 0xFF) in s_state.output_bits
 *         Requires ARM.
 */
static int32_t my_toggle_output_handler(const vtfp_request_t *req, vtfp_response_t *resp) {
    (void)resp;
    uint8_t bit = (uint8_t)(req->param & 0xFF);
    if (bit >= 8) {
        return (int32_t)VTFP_R_PARAM_RANGE;
    }
    s_state.output_bits ^= (uint8_t)(1U << bit);
    s_state.button_presses++;  /* count the "button press" */
    return 0;
}

/* === END user handler implementations === */

/**
 * @brief  Register all user-defined handlers. Call from main() after
 *         vtfp_init() and before entering the main loop.
 */
void vtfp_user_handlers_init(void) {
    vtfp_register_handler(0x01, my_query_state_handler,    VTFP_ARM_NONE);
    vtfp_register_handler(0x02, my_set_param_handler,      VTFP_ARM_NONE);
    vtfp_register_handler(0x03, my_toggle_output_handler,   VTFP_ARM_REQUIRED);
}
