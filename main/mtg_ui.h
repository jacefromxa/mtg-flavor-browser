#pragma once

#include "mtg_state.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 初始化并创建 MTG 箴言浏览器专属 UI（必须在持锁状态下调用） */
bool mtg_ui_create(void);

/* 根据当前应用状态刷新 UI 呈现（必须在持锁状态下调用） */
void mtg_ui_refresh(const mtg_state_t *state);

/* 销毁 UI 资源（如果需要） */
void mtg_ui_destroy(void);

#ifdef __cplusplus
}
#endif
