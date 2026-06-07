/*******************************************************************************
* Description    : 菜单系统头文件
* Input          : none
* Output         : none
* Return         : none
* Application note: 多层树状菜单导航，支持UP/DOWN/ENTER/ESC按键操作
*******************************************************************************/
#ifndef __MENU_H
#define __MENU_H

#include "key.h"

/* 菜单项最大标题长度 */
#define MENU_TITLE_MAX_LEN  16

/* 前向声明 */
struct menu_item_t;
typedef struct menu_item_t menu_item_t;

/* 菜单项回调函数类型 */
typedef void (*menu_callback_t)(void);

/* 菜单项结构体 */
struct menu_item_t {
    char title[MENU_TITLE_MAX_LEN];      /* 菜单项标题 */
    menu_item_t *parent;                 /* 父菜单（NULL表示根） */
    menu_item_t *child;                  /* 子菜单入口（NULL表示叶子节点） */
    menu_item_t *sibling;                /* 同级下一个菜单项（NULL表示同级末尾） */
    menu_callback_t callback;            /* 确认回调（叶子节点执行） */
    uint8_t reserved;                    /* 保留对齐 */
};

/* 菜单导航器状态 */
typedef struct {
    menu_item_t *current;               /* 当前焦点菜单项 */
    menu_item_t *root;                  /* 根菜单 */
} menu_navigator_t;

/*******************************************************************************
* Description    : 初始化菜单导航器
* Input          : root_menu - 根菜单项指针
* Output         : none
* Return         : none
* Application note: 系统启动时调用一次
*******************************************************************************/
void menu_init(menu_item_t *root_menu);

/*******************************************************************************
* Description    : 处理菜单导航
* Input          : event - 按键事件
* Output         : none
* Return         : none
* Application note: UP/DOWN切换选项，ENTER进入/执行回调，ESC返回上级
*******************************************************************************/
void menu_navigate(key_event_t event);

/*******************************************************************************
* Description    : 获取当前焦点菜单项
* Input          : none
* Output         : none
* Return         : 当前焦点菜单项指针
* Application note: 供UI层调用以显示当前菜单
*******************************************************************************/
menu_item_t *menu_get_current(void);

/*******************************************************************************
* Description    : 获取菜单项的同级列表头
* Input          : item - 任意菜单项指针
* Output         : none
* Return         : 同级列表的头指针
* Application note: 内部函数，用于菜单遍历
*******************************************************************************/
menu_item_t *menu_get_sibling_head(menu_item_t *item);

/* 菜单显示回调类型 - 由UI层实现 */
typedef void (*menu_render_callback_t)(menu_item_t *current, menu_item_t **siblings, uint8_t count);

/*******************************************************************************
* Description    : 取当前焦点项在其同级列表中的索引(0..N-1)
*                  供 VTFP QUERY_STATE 暴露给 PC
*******************************************************************************/
uint32_t menu_get_cursor_index(void);

/*******************************************************************************
* Description    : 记录最近一次按键事件(供 VTFP QUERY_STATE 暴露给 PC)
*******************************************************************************/
void menu_set_last_key(key_event_t evt);
key_event_t menu_get_last_key(void);

#endif /* __MENU_H */
