#define COMPRESSION_INTERNAL
#include "compress.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LZ10_MAX_LEN  18
#define LZ10_MAX_DISP 4096

#define LZ11_MAX_LEN  65808
#define LZ11_MAX_DISP 4096

typedef enum
{
  LZ10,
  LZ11,
} lzss_mode_t;

const uint8_t*
find_best_match(const uint8_t *start,
                const uint8_t *buffer,
                size_t        len,
                size_t        max_disp,
                size_t        *outlen)
{
  const uint8_t *best_start = NULL;
  size_t        best_len    = 0;

  if(start + max_disp < buffer)
    start = buffer - max_disp;

  uint8_t *p = memrchr(start, *buffer, buffer - start);
  while(p != NULL)
  {
    size_t test_len = 1;
    for(size_t i = 1; i < len; ++i)
    {
      if(p[i] == buffer[i])
        ++test_len;
      else
        break;
    }

    if(test_len >= best_len)
    {
      best_start = p;
      best_len   = test_len;
    }

    if(best_len == len)
      break;

    p = memrchr(start, *buffer, p - start);
  }

  if(best_len)
  {
    *outlen = best_len;
    return best_start;
  }

  *outlen = 0;
  return NULL;
}

static void*
lzss_common_encode(const uint8_t *buffer,
                   size_t        len,
                   size_t        *outlen,
                   lzss_mode_t   mode)
{
  buffer_t      result;
  const uint8_t *start = buffer;
#ifndef NDEBUG
  const uint8_t *end   = buffer + len;
#endif
  size_t        shift = 7, code_pos = 4;

  const size_t  max_len  = mode == LZ10 ? LZ10_MAX_LEN  : LZ11_MAX_LEN;
  const size_t  max_disp = mode == LZ10 ? LZ10_MAX_DISP : LZ11_MAX_DISP;

  uint8_t       header[4];

  if(mode == LZ10)
    compression_header(header, 0x10, len);
  else
    compression_header(header, 0x11, len);

  assert(mode == LZ10 || mode == LZ11);

  buffer_init(&result);

  if(buffer_push(&result, header, sizeof(header)) != 0)
  {
    buffer_destroy(&result);
    return NULL;
  }

  if(buffer_push(&result, NULL, 1) != 0)
  {
    buffer_destroy(&result);
    return NULL;
  }
  
  result.data[code_pos] = 0;

  while(len > 0)
  {
    assert(buffer < end);
    assert(buffer + len == end);

    const uint8_t *tmp;
    size_t        tmplen;

    if(buffer != start)
    {
      if(len < max_len)
        tmp = find_best_match(start, buffer, len, max_disp, &tmplen);
      else
        tmp = find_best_match(start, buffer, max_len, max_disp, &tmplen);

      if(tmp != NULL)
      {
        assert(tmp >= start);
        assert(tmp < buffer);
        assert(buffer - tmp <= max_disp);
        assert(tmplen <= max_len);
        assert(tmplen <= len);
        assert(memcmp(buffer, tmp, tmplen) == 0);
      }
    }
    else
      tmplen = 1;

    if(tmplen > 2 && tmplen < len)
    {
      size_t skip_len, next_len;

      if(len+1 < max_len)
        find_best_match(start, buffer+1, len-1, max_disp, &skip_len);
      else
        find_best_match(start, buffer+1, max_len, max_disp, &skip_len);
      if(skip_len < 3)
        skip_len = 1;

      if(len+tmplen < max_len)
        find_best_match(start, buffer+tmplen, len-tmplen, max_disp, &next_len);
      else
        find_best_match(start, buffer+tmplen, max_len, max_disp, &next_len);
      if(next_len < 3)
        next_len = 1;

      if(tmplen + next_len <= skip_len + 1)
        tmplen = 1;
    }

    if(tmplen < 3)
    {
      if(buffer_push(&result, buffer, 1) != 0)
      {
        buffer_destroy(&result);
        return NULL;
      }
      tmplen = 1;
    }
    else if(mode == LZ10)
    {
      if(buffer_push(&result, NULL, 2) != 0)
      {
        buffer_destroy(&result);
        return NULL;
      }

      result.data[code_pos] |= (1 << shift);
      size_t disp = buffer - tmp - 1;
      result.data[result.len-2] = ((tmplen-3) << 4) | (disp >> 8);
      result.data[result.len-1] = disp;
    }
    else if(tmplen <= 16)
    {
      if(buffer_push(&result, NULL, 2) != 0)
      {
        buffer_destroy(&result);
        return NULL;
      }

      result.data[code_pos] |= (1 << shift);
      size_t disp = buffer - tmp - 1;
      result.data[result.len-2] = ((tmplen-0x1) << 4) | (disp >> 8);
      result.data[result.len-1] = disp;
    }
    else if(tmplen <= 272)
    {
      if(buffer_push(&result, NULL, 3) != 0)
      {
        buffer_destroy(&result);
        return NULL;
      }

      result.data[code_pos] |= (1 << shift);
      size_t disp = buffer - tmp - 1;
      result.data[result.len-3] = (tmplen-0x11) >> 4;
      result.data[result.len-2] = ((tmplen-0x11) << 4) | (disp >> 8);
      result.data[result.len-1] = disp;
    }
    else
    {
      if(buffer_push(&result, NULL, 4) != 0)
      {
        buffer_destroy(&result);
        return NULL;
      }

      result.data[code_pos] |= (1 << shift);
      size_t disp = buffer - tmp - 1;
      result.data[result.len-4] = (1 << 4) | (tmplen-0x111) >> 12;
      result.data[result.len-3] = (tmplen-0x111) >> 4;
      result.data[result.len-2] = ((tmplen-0x111) << 4) | (disp >> 8);
      result.data[result.len-1] = disp;
    }

    buffer += tmplen;
    len    -= tmplen;

    if(shift == 0)
    {
      shift = 8;
      if(buffer_push(&result, NULL, 1) != 0)
      {
        buffer_destroy(&result);
        return NULL;
      }

      code_pos = result.len - 1;
      result.data[code_pos] = 0;
    }
    --shift;
  }

  if(buffer_pad(&result, 4) != 0)
  {
    buffer_destroy(&result);
    return NULL;
  }

  *outlen = result.len;
  return result.data;
}

void*
lzss_encode(const void *src,
            size_t     len,
            size_t     *outlen)
{
  return lzss_common_encode(src, len, outlen, LZ10);
}

void*
lz11_encode(const void *src,
            size_t     len,
            size_t     *outlen)
{
  return lzss_common_encode(src, len, outlen, LZ11);
}

void lzss_decode(const void *source, void *dest, size_t size)
{
  const uint8_t *src = (const uint8_t*)source;
  uint8_t       *dst = (uint8_t*)dest;
  uint8_t flags = 0;
  uint8_t mask  = 0;
  unsigned int  len;
  unsigned int  disp;

  while(size > 0)
  {
    if(mask == 0)
    {
      // read in the flags data
      // from bit 7 to bit 0:
      //     0: raw byte
      //     1: compressed block
      flags = *src++;
      mask  = 0x80;
    }

    if(flags & mask) // compressed block
    {
      // disp: displacement
      // len:  length
      len  = (((*src)&0xF0)>>4)+3;
      disp = ((*src++)&0x0F);
      disp = disp<<8 | (*src++);

      if(len > size)
        len = size;

      size -= len;

      // for len, copy data from the displacement
      // to the current buffer position
      for(uint8_t *p = dst-disp-1; len > 0; --len)
        *dst++ = *p++;
    }
    else { // uncompressed block
      // copy a raw byte from the input to the output
      *dst++ = *src++;
      size--;
    }

    mask >>= 1;
  }
}

void
lz11_decode(const void *source, void *dest, size_t size)
{
  const uint8_t *src = (const uint8_t*)source;
  uint8_t       *dst = (uint8_t*)dest;
  int           i;
  uint8_t       flags;

  while(size > 0)
  {
    // read in the flags data
    // from bit 7 to bit 0, following blocks:
    //     0: raw byte
    //     1: compressed block
    flags = *src++;
    for(i = 0; i < 8 && size > 0; i++, flags <<= 1)
    {
      if(flags&0x80) // compressed block
      {
        size_t len;  // length
        size_t disp; // displacement
        switch((*src)>>4)
        {
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
            break;
        }

        disp  = ((*src++)&0x0F)<<8;
        disp |= *src++;

        if(len > size)
          len = size;

        size -= len;

        // for len, copy data from the displacement
        // to the current buffer position
        for(uint8_t *p = dst-disp-1; len > 0; --len)
          *dst++ = *p++;
      }

      else { // uncompressed block
        // copy a raw byte from the input to the output
        *dst++ = *src++;
        --size;
      }
    }
  }
}
