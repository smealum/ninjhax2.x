#ifndef TAKEOVER_H
#define TAKEOVER_H

void getProcessMap(Handle fsuserHandle, u8 mediatype, u32 tid_low, u32 tid_high, void* out, u32* tmp);
void superto(u64 tid, u32 mediatype, u32* argbuf, u32 argbuflength);

#endif
