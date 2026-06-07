#include "menu.h"
#include <stddef.h>

/* 菜单导航器全局状态 */
static menu_navigator_t g_navigator = {
    .current = NULL,
    .root = NULL
};

/* 最近一次按键事件(供 VTFP QUERY_STATE 读取) */
static key_event_t g_last_key = KEY_EVENT_NONE;

/*******************************************************************************
* Description    : 初始化菜单导航器
* Input          : root_menu - 根菜单项指针
* Output         : none
* Return         : none
* Application note: 系统启动时调用一次
*******************************************************************************/
void menu_init(menu_item_t *root_menu)
{
    g_navigator.root = root_menu;
    g_navigator.current = root_menu;
}

/*******************************************************************************
* Description    : 获取菜单项的同级列表头
* Input          : item - 任意菜单项指针
* Output         : none
* Return         : 同级列表的头指针
* Application note: 内部函数，用于菜单遍历
*******************************************************************************/
menu_item_t *menu_get_sibling_head(menu_item_t *item)
{
    if (item == NULL) return NULL;

    /* 查找同级列表头：向上找parent或prev sibling */
    menu_item_t *head = item;
    while (head->sibling != NULL) {
        /* 如果有prev sibling，往前找 */
        menu_item_t *p = head->parent;
        if (p != NULL) {
            menu_item_t *s = p->child;
            while (s != NULL && s->sibling != head) {
                s = s->sibling;
            }
            if (s != NULL && s->sibling == head) {
                head = s;
                break;
            }
        }
        break;
    }
    return head;
}

/*******************************************************************************
* Description    : 查找同级列表中当前项的下一个
* Input          : item - 当前菜单项指针
* Output         : none
* Return         : 下一个同级项指针，如果已是最后一个则返回NULL
* Application note: 内部函数
*******************************************************************************/
static menu_item_t *get_next_sibling(menu_item_t *item)
{
    return item->sibling;
}

/*******************************************************************************
* Description    : 查找同级列表中当前项的上一个
* Input          : item - 当前菜单项指针
* Output         : none
* Return         : 上一个同级项指针，如果已是第一个则返回NULL
* Application note: 内部函数
*******************************************************************************/
static menu_item_t *get_prev_sibling(menu_item_t *item)
{
    if (item == NULL || item->parent == NULL) return NULL;

    menu_item_t *head = item->parent->child;
    menu_item_t *prev = NULL;

    while (head != NULL) {
        if (head == item) break;
        prev = head;
        head = head->sibling;
    }

    return prev;
}

/*******************************************************************************
* Description    : 计数同级列表项个数
* Input          : item - 任意菜单项指针
* Output         : none
* Return         : 同级列表项个数
* Application note: 内部函数
*******************************************************************************/
static uint8_t count_siblings(menu_item_t *item)
{
    uint8_t count = 0;
    menu_item_t *head = menu_get_sibling_head(item);

    while (head != NULL) {
        count++;
        head = head->sibling;
    }

    return count;
}

/*******************************************************************************
* Description    : 处理菜单导航
* Input          : event - 按键事件
* Output         : none
* Return         : none
* Application note: UP/DOWN切换选项，ENTER进入/执行回调，ESC返回上级
*******************************************************************************/
void menu_navigate(key_event_t event)
{
    menu_item_t *cur = g_navigator.current;

    if (cur == NULL) return;

    switch (event) {
        case KEY_EVENT_UP:
        case KEY_EVENT_UP_LONG: {
            /* 查找上一个同级项 */
            menu_item_t *prev = get_prev_sibling(cur);
            if (prev == NULL) {
                /* 已是第一项，跳到最后一个 */
                menu_item_t *head = menu_get_sibling_head(cur);
                while (head->sibling != NULL) {
                    head = head->sibling;
                }
                g_navigator.current = head;
            } else {
                g_navigator.current = prev;
            }
            break;
        }

        case KEY_EVENT_DOWN:
        case KEY_EVENT_DOWN_LONG: {
            /* 查找下一个同级项 */
            menu_item_t *next = get_next_sibling(cur);
            if (next != NULL) {
                g_navigator.current = next;
            } else {
                /* 已是最后一项，跳到第一个 */
                g_navigator.current = menu_get_sibling_head(cur);
            }
            break;
        }

        case KEY_EVENT_ENTER:
        case KEY_EVENT_ENTER_LONG: {
            /* 有子菜单则进入，否则执行回调 */
            if (cur->child != NULL) {
                g_navigator.current = cur->child;
            } else if (cur->callback != NULL) {
                cur->callback();
            }
            break;
        }

        case KEY_EVENT_ESC:
        case KEY_EVENT_ESC_LONG: {
            /* 返回父菜单 */
            if (cur->parent != NULL) {
                g_navigator.current = cur->parent;
            }
            break;
        }

        default:
            break;
    }
}

/*******************************************************************************
* Description    : 获取当前焦点菜单项
* Input          : none
* Output         : none
* Return         : 当前焦点菜单项指针
* Application note: 供UI层调用以显示当前菜单
*******************************************************************************/
menu_item_t *menu_get_current(void)
{
    return g_navigator.current;
}

/* 计数从 head 开始的同级项数 */
static uint32_t menu_count_from(menu_item_t *head) {
    uint32_t n = 0;
    while (head != NULL) { n++; head = head->sibling; }
    return n;
}

/* 在同级列表中查找 item 的索引;找不到返回 0 */
static uint32_t menu_index_of(menu_item_t *item) {
    if (item == NULL) return 0u;
    menu_item_t *head = item;
    /* 若有 parent,从 parent->child 开始找;否则自身就是头 */
    if (item->parent != NULL && item->parent->child != NULL) {
        head = item->parent->child;
    }
    uint32_t idx = 0;
    while (head != NULL && head != item) { idx++; head = head->sibling; }
    return idx;
}

uint32_t menu_get_cursor_index(void) {
    return menu_index_of(g_navigator.current);
}

void menu_set_last_key(key_event_t evt) {
    g_last_key = evt;
}

key_event_t menu_get_last_key(void) {
    return g_last_key;
}
