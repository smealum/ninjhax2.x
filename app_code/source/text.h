#ifndef TEXT_H
#define TEXT_H

void print_str(char* str);
void print_hex(u32 val);

void drawCharacter(u8* fb, char c, u16 x, u16 y);
void drawString(u8* fb, char* str, u16 x, u16 y);

#endif
