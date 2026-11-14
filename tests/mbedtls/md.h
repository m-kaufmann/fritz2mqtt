// Host test adapter only: use Apple's CommonCrypto to exercise the production
// protocol algorithm. The ESP32 firmware uses genuine Mbed TLS from ESP-IDF.
#pragma once
#include <CommonCrypto/CommonDigest.h>
#include <CommonCrypto/CommonHMAC.h>
#include <vector>
#define MBEDTLS_MD_SHA256 1
#define MBEDTLS_MD_MD5 2
using mbedtls_md_info_t = int;
struct mbedtls_md_context_t { CCHmacContext ctx; std::vector<unsigned char> key; };
inline const mbedtls_md_info_t *mbedtls_md_info_from_type(int type) {
  static int sha = MBEDTLS_MD_SHA256, md5 = MBEDTLS_MD_MD5;
  return type == sha ? &sha : type == md5 ? &md5 : nullptr;
}
inline void mbedtls_md_init(mbedtls_md_context_t *) {}
inline int mbedtls_md_setup(mbedtls_md_context_t *, const int *, int) { return 0; }
inline int mbedtls_md_hmac_starts(mbedtls_md_context_t *c, const unsigned char *p, size_t n) {
  c->key.assign(p, p + n); CCHmacInit(&c->ctx, kCCHmacAlgSHA256, p, n); return 0;
}
inline int mbedtls_md_hmac_update(mbedtls_md_context_t *c, const unsigned char *p, size_t n) {
  CCHmacUpdate(&c->ctx, p, n); return 0;
}
inline int mbedtls_md_hmac_finish(mbedtls_md_context_t *c, unsigned char *out) { CCHmacFinal(&c->ctx, out); return 0; }
inline int mbedtls_md_hmac_reset(mbedtls_md_context_t *c) {
  CCHmacInit(&c->ctx, kCCHmacAlgSHA256, c->key.data(), c->key.size()); return 0;
}
inline void mbedtls_md_free(mbedtls_md_context_t *) {}
inline int mbedtls_md(const int *info, const unsigned char *p, size_t n, unsigned char *out) {
  if (*info != MBEDTLS_MD_MD5) return -1;
  CC_MD5(p, static_cast<CC_LONG>(n), out); return 0;
}
