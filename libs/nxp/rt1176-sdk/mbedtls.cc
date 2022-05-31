#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <memory>

#include "libs/base/filesystem.h"
#include "third_party/nxp/rt1176-sdk/middleware/lwip/src/include/lwip/sockets.h"
#include "third_party/nxp/rt1176-sdk/middleware/mbedtls/include/mbedtls/net_sockets.h"
#include "third_party/nxp/rt1176-sdk/middleware/mbedtls/include/mbedtls/pk.h"
#include "third_party/nxp/rt1176-sdk/middleware/mbedtls/include/mbedtls/platform.h"
#include "third_party/nxp/rt1176-sdk/middleware/mbedtls/include/mbedtls/ssl.h"

extern "C" int coral_micro_mbedtls_printf(const char *format, ...) {
  va_list args;
  va_start(args, format);
  int formatted_size = vsnprintf(NULL, 0, format, args) + 1;
  auto formatted_string = std::make_unique<char[]>(formatted_size);
  vsnprintf(formatted_string.get(), formatted_size, format, args);
  for (size_t i = 0; i < strlen(formatted_string.get()); ++i) {
    char c = formatted_string[i];
    if (c == '\n') {
      putchar('\r');
    }
    putchar(formatted_string[i]);
  }
  va_end(args);
  return formatted_size;
}

// The below are implementations based on
// third_party/nxp/rt1176-sdk/middleware/mbedtls/library/net_sockets.c In a
// "normal" mbedtls build, these require something Unixy, or Windows. These
// modify the stock implementations to work with our network stack (lwip).
static int net_would_block(const mbedtls_net_context *ctx) {
  int err = errno;

  if ((lwip_fcntl(ctx->fd, F_GETFL, 0) & O_NONBLOCK) != O_NONBLOCK) {
    errno = err;
    return 0;
  }

  switch (errno = err) {
    case EAGAIN:
      return 1;
  }
  return 0;
}

extern "C" int mbedtls_net_send(void *ctx, unsigned char const *buf,
                                size_t len) {
  int ret;
  int fd = (reinterpret_cast<mbedtls_net_context *>(ctx))->fd;

  if (fd < 0) {
    return MBEDTLS_ERR_NET_INVALID_CONTEXT;
  }

  ret = lwip_write(fd, buf, len);

  if (ret < 0) {
    if (net_would_block(reinterpret_cast<mbedtls_net_context *>(ctx)) != 0) {
      return MBEDTLS_ERR_SSL_WANT_WRITE;
    }

    if (errno == EPIPE || errno == ECONNRESET) {
      return MBEDTLS_ERR_NET_CONN_RESET;
    }

    if (errno == EINTR) {
      return MBEDTLS_ERR_SSL_WANT_WRITE;
    }

    return MBEDTLS_ERR_NET_SEND_FAILED;
  }

  return ret;
}

extern "C" int mbedtls_net_recv(void *ctx, unsigned char *buf, size_t len) {
  int ret;
  int fd = (reinterpret_cast<mbedtls_net_context *>(ctx))->fd;

  if (fd < 0) {
    return MBEDTLS_ERR_NET_INVALID_CONTEXT;
  }

  ret = (int)lwip_read(fd, buf, len);

  if (ret < 0) {
    if (net_would_block(reinterpret_cast<mbedtls_net_context *>(ctx)) != 0) {
      return MBEDTLS_ERR_SSL_WANT_READ;
    }

    if (errno == EPIPE || errno == ECONNRESET) {
      return MBEDTLS_ERR_NET_CONN_RESET;
    }

    if (errno == EINTR) {
      return MBEDTLS_ERR_SSL_WANT_READ;
    }

    return MBEDTLS_ERR_NET_RECV_FAILED;
  }

  return ret;
}

// The below are based on implementations in
// third_party/nxp/rt1176-sdk/middleware/mbedtls/library/pkparse.c In a normal
// curl build, these require filesystem support. While we have a filesystem,
// it's not accessible by normal things like fopen. Adapt the original
// implementation to use our filesystem API.
extern "C" int mbedtls_pk_load_file(const char *path, unsigned char **buf,
                                    size_t *n) {
  MBEDTLS_INTERNAL_VALIDATE_RET(path != NULL, MBEDTLS_ERR_PK_BAD_INPUT_DATA);
  MBEDTLS_INTERNAL_VALIDATE_RET(buf != NULL, MBEDTLS_ERR_PK_BAD_INPUT_DATA);
  MBEDTLS_INTERNAL_VALIDATE_RET(n != NULL, MBEDTLS_ERR_PK_BAD_INPUT_DATA);

  lfs_file_t handle;
  if (!coral::micro::filesystem::Open(&handle, path)) {
    return MBEDTLS_ERR_PK_FILE_IO_ERROR;
  }

  *n = coral::micro::filesystem::Size(&handle);

  if (*n + 1 == 0 || (*buf = reinterpret_cast<unsigned char *>(
                          mbedtls_calloc(1, *n + 1))) == NULL) {
    coral::micro::filesystem::Close(&handle);
    return MBEDTLS_ERR_PK_ALLOC_FAILED;
  }

  if (static_cast<size_t>(coral::micro::filesystem::Read(&handle, *buf, *n)) != *n) {
    coral::micro::filesystem::Close(&handle);

    mbedtls_platform_zeroize(*buf, *n);
    mbedtls_free(*buf);

    return MBEDTLS_ERR_PK_FILE_IO_ERROR;
  }

  coral::micro::filesystem::Close(&handle);

  (*buf)[*n] = '\0';

  if (strstr(reinterpret_cast<const char *>(*buf), "-----BEGIN ") != NULL) ++*n;

  return (0);
}

extern "C" int mbedtls_pk_parse_keyfile(mbedtls_pk_context *ctx,
                                        const char *path, const char *pwd) {
  return MBEDTLS_ERR_PK_FILE_IO_ERROR;
}

// The below are based on implementations in
// third_party/nxp/rt1176-sdk/middleware/mbedtls/library/x509_crt.c In a normal
// curl build, these require filesystem support. While we have a filesystem,
// it's not accessible by normal things like fopen. Adapt the original
// implementation to use our filesystem API.
extern "C" int mbedtls_x509_crt_parse_file(mbedtls_x509_crt *chain,
                                           const char *path) {
  int ret;
  size_t n;
  unsigned char *buf;

  if ((ret = mbedtls_pk_load_file(path, &buf, &n)) != 0) {
    return ret;
  }

  ret = mbedtls_x509_crt_parse(chain, buf, n);

  mbedtls_platform_zeroize(buf, n);
  mbedtls_free(buf);

  return ret;
}

extern "C" int mbedtls_x509_crl_parse_file(mbedtls_x509_crl *chain,
                                           const char *path) {
  return MBEDTLS_ERR_X509_FILE_IO_ERROR;
}

extern "C" int mbedtls_x509_crt_parse_path(mbedtls_x509_crt *chain,
                                           const char *path) {
  return MBEDTLS_ERR_X509_FILE_IO_ERROR;
}
