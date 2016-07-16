#include <string.h>
#include "decompress.h"

void lz11Decompress(const u8 *src, u8 *dst, int size) {
  int i;
  unsigned char flags;

  while(size > 0) {
    // read in the flags data
    // from bit 7 to bit 0, following blocks:
    //     0: raw byte
    //     1: compressed block
    flags = *src++;
    for(i = 0; i < 8 && size > 0; i++, flags <<= 1) {
      if(flags&0x80) { // compressed block
        int len;  // length
        int disp; // displacement
        switch((*src)>>4) {
          case 0: // extended block
            len   = (*src++)<<4;
            len  |= ((*src)>>4);
            len  += 0x11;
            break;
          case 1: // extra extended block
            len   = ((*src++)&0x0F)<<12;
            len  |= (*src++)<<4;
            len  |= ((*src)>>4);
            len  += 0x111;
            break;
          default: // normal block
            len   = ((*src)>>4)+1;
        }

        disp  = ((*src++)&0x0F)<<8;
        disp |= *src++;

        size -= len;

        // for len, copy data from the displacement
        // to the current buffer position
        for(u8 *p = dst-disp-1; len > 0; --len) *dst++ = *p++;
        dst += len;
      }

      else { // uncompressed block
        // copy a raw byte from the input to the output
        *dst++ = *src++;
        size--;
      }
    }
  }
}

