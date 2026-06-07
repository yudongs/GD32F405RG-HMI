/**
 * @file    vtfp_user_dispatch.h
 * @brief   Public interface for vtfp_user_dispatch.c
 * @note    Generated alongside the .c by init-vtfp.py codegen.
 */
#ifndef __VTFP_USER_DISPATCH_H
#define __VTFP_USER_DISPATCH_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Register all user-defined VTFP handlers (KEY_PRESS, SET_DISPLAY_MODE,
 *         DUMP_FRAMEBUFFER, QUERY_STATE). Call once after vtfp_init() and
 *         before entering the main poll loop.
 */
void vtfp_user_handlers_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __VTFP_USER_DISPATCH_H */
