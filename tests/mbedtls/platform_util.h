#pragma once
#include <cstddef>
inline void mbedtls_platform_zeroize(void *p, size_t n) {
  volatile unsigned char *v = static_cast<volatile unsigned char *>(p);
  while (n--) *v++ = 0;
}
