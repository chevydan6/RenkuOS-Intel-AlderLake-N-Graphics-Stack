#pragma once
struct mutex {};
static inline void mutex_init(mutex*,const char*){}
static inline void mutex_lock(mutex*){}
static inline void mutex_unlock(mutex*){}
