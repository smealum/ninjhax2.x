#ifndef SYS_H
#define SYS_H

#include <ctr/types.h>

extern u8* _heap_base;
extern const u32 _heap_size;
extern const u32 _gsp_heap_size;

Result svc_getResourceLimit(Handle* resourceLimit, Handle process);
Result svc_getResourceLimitLimitValues(s64* values, Handle resourceLimit, u32* names, s32 nameCount);
Result svc_getResourceLimitCurrentValue(s64* values, Handle resourceLimit, u32* names, s32 nameCount);

#endif
