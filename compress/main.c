#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compress.h"

int main(int argc, char *argv[])
{
  static unsigned char buffer[0xFFFFFF];
  size_t               inlen = 0, outlen;
  ssize_t              rc;
  uint8_t              *result;
  FILE                 *in_file = stdin, *out_file = stdout;
  const char           *in_name = "stdin", *out_name = "stdout";

  if(argc > 1)
  {
    in_name = argv[1];
    in_file = fopen(in_name, "rb");
    if(!in_file)
    {
      fprintf(stderr, "fopen %s: %s\n", in_name, strerror(errno));
      return EXIT_FAILURE;
    }
  }

  if(argc > 2)
  {
    out_name = argv[2];
    out_file = fopen(out_name, "wb");
    if(!out_file)
    {
      fprintf(stderr, "fopen %s: %s\n", out_name, strerror(errno));
      fclose(in_file);
      return EXIT_FAILURE;
    }
  }

  while(inlen < sizeof(buffer))
  {
    rc = fread(buffer+inlen, 1, sizeof(buffer)-inlen, in_file);
    if(rc < 0)
    {
      fprintf(stderr, "fread %s: %s\n", in_name, strerror(errno));
      fclose(in_file);
      fclose(out_file);
      return EXIT_FAILURE;
    }
    else if(rc == 0)
      break;
    else
      inlen += rc;
  }

  if(!feof(in_file))
  {
    fprintf(stderr, "File too large\n");
    fclose(in_file);
    return EXIT_FAILURE;
  }

  fclose(in_file);

  result = (uint8_t*)lz11_encode(buffer, inlen, &outlen);
  if(!result)
  {
    fprintf(stderr, "Failed to compress %s: %s\n", in_name, strerror(ENOMEM));
    fclose(out_file);
    return EXIT_FAILURE;
  }

  inlen = 0;
  while(inlen < outlen)
  {
    rc = fwrite(result+inlen, 1, outlen-inlen, out_file);
    if(rc < 0)
    {
      fprintf(stderr, "fwrite %s: %s\n", out_name, strerror(errno));
      fclose(out_file);
      return EXIT_FAILURE;
    }
    else if(rc > 0)
      inlen += rc;
  }

  if(fclose(out_file) != 0)
  {
    fprintf(stderr, "fclose %s: %s\n", out_name, strerror(errno));
    return EXIT_FAILURE;
  }

  fprintf(stderr, "LZ11 Compressed %zu -> %zu bytes\n", rc, outlen);

  return EXIT_SUCCESS;
}
