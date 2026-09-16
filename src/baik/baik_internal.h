/*
 * baik_internal.h - Deklarasi internal interpreter bahasa BAIK.
 *
 * Berkas ini memuat seluruh tipe, makro, dan prototipe yang dipakai bersama
 * oleh modul-modul di src/baik/. Ini BUKAN antarmuka publik: kode di luar
 * interpreter cukup menyertakan src/baik.h.
 *
 * BAIK_EXPOSE_PRIVATE didefinisikan di sini supaya makro BAIK_PRIVATE memberi
 * linkage eksternal. Ketika interpreter masih satu berkas, BAIK_PRIVATE berarti
 * `static` dan itu memadai; setelah dipecah menjadi modul, simbol-simbol itu
 * harus bisa dilihat antar berkas objek.
 */
#ifndef BAIK_INTERNAL_MODUL_H_
#define BAIK_INTERNAL_MODUL_H_

#ifndef BAIK_EXPOSE_PRIVATE
#define BAIK_EXPOSE_PRIVATE 1
#endif

#include "baik.h"
#ifndef BAIK_EM_COMMON_PLATFORM_H_
#define BAIK_EM_COMMON_PLATFORM_H_

#define BAIK_EM_P_CUSTOM 0
#define BAIK_EM_P_UNIX 1
#define BAIK_EM_P_WINDOWS 2
#define BAIK_EM_P_ESP32 15
#define BAIK_EM_P_ESP8266 3
#define BAIK_EM_P_CC3100 6
#define BAIK_EM_P_CC3200 4
#define BAIK_EM_P_CC3220 17
#define BAIK_EM_P_MSP432 5
#define BAIK_EM_P_TM4C129 14
#define BAIK_EM_P_MBED 7
#define BAIK_EM_P_WINCE 8
#define BAIK_EM_P_NXP_LPC 13
#define BAIK_EM_P_NXP_KINETIS 9
#define BAIK_EM_P_NRF51 12
#define BAIK_EM_P_NRF52 10
#define BAIK_EM_P_PIC32 11
#define BAIK_EM_P_RS14100 18
#define BAIK_EM_P_STM32 16

#ifndef BAIK_EM_PLATFORM

#if defined(TARGET_IS_MSP432P4XX) || defined(__MSP432P401R__)
#define BAIK_EM_PLATFORM BAIK_EM_P_MSP432
#elif defined(cc3200) || defined(TARGET_IS_CC3200)
#define BAIK_EM_PLATFORM BAIK_EM_P_CC3200
#elif defined(cc3220) || defined(TARGET_IS_CC3220)
#define BAIK_EM_PLATFORM BAIK_EM_P_CC3220
#elif defined(__unix__) || defined(__APPLE__)
#define BAIK_EM_PLATFORM BAIK_EM_P_UNIX
#elif defined(WINCE)
#define BAIK_EM_PLATFORM BAIK_EM_P_WINCE
#elif defined(_WIN32)
#define BAIK_EM_PLATFORM BAIK_EM_P_WINDOWS
#elif defined(__MBED__)
#define BAIK_EM_PLATFORM BAIK_EM_P_MBED
#elif defined(__USE_LPCOPEN)
#define BAIK_EM_PLATFORM BAIK_EM_P_NXP_LPC
#elif defined(FRDM_K64F) || defined(FREEDOM)
#define BAIK_EM_PLATFORM BAIK_EM_P_NXP_KINETIS
#elif defined(PIC32)
#define BAIK_EM_PLATFORM BAIK_EM_P_PIC32
#elif defined(ESP_PLATFORM)
#define BAIK_EM_PLATFORM BAIK_EM_P_ESP32
#elif defined(ICACHE_FLASH)
#define BAIK_EM_PLATFORM BAIK_EM_P_ESP8266
#elif defined(TARGET_IS_TM4C129_RA0) || defined(TARGET_IS_TM4C129_RA1) || \
    defined(TARGET_IS_TM4C129_RA2)
#define BAIK_EM_PLATFORM BAIK_EM_P_TM4C129
#elif defined(RS14100)
#define BAIK_EM_PLATFORM BAIK_EM_P_RS14100
#elif defined(STM32)
#define BAIK_EM_PLATFORM BAIK_EM_P_STM32
#endif

#ifndef BAIK_EM_PLATFORM
#error "BAIK_EM_PLATFORM is not specified and we couldn't guess it."
#endif

#endif

#define GENERIC_NET_IF_SOCKET 1
#define GENERIC_NET_IF_SIMPLELINK 2
#define GENERIC_NET_IF_LWIP_LOW_LEVEL 3
#define GENERIC_NET_IF_PIC32 4
#define GENERIC_NET_IF_NULL 5

#define GENERIC_SSL_IF_OPENSSL 1
#define GENERIC_SSL_IF_MBEDTLS 2
#define GENERIC_SSL_IF_SIMPLELINK 3

#if BAIK_EM_PLATFORM == BAIK_EM_P_CUSTOM
#include <platform_custom.h>
#endif

#if !defined(PRINTF_LIKE)
#if defined(__GNUC__) || defined(__clang__) || defined(__TI_COMPILER_VERSION__)
#define PRINTF_LIKE(f, a) __attribute__((format(printf, f, a)))
#else
#define PRINTF_LIKE(f, a)
#endif
#endif

#if !defined(WEAK)
#if (defined(__GNUC__) || defined(__clang__) || \
     defined(__TI_COMPILER_VERSION__)) &&       \
    !defined(_WIN32)
#define WEAK __attribute__((weak))
#else
#define WEAK
#endif
#endif

#ifdef __GNUC__
#define NORETURN __attribute__((noreturn))
#define NOINLINE __attribute__((noinline))
#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#define NOINSTR __attribute__((no_instrument_function))
#define DO_NOT_WARN_UNUSED __attribute__((unused))
#else
#define NORETURN
#define NOINLINE
#define WARN_UNUSED_RESULT
#define NOINSTR
#define DO_NOT_WARN_UNUSED
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))
#endif

#endif
#ifndef BAIK_EM_COMMON_PLATFORMS_PLATFORM_WINDOWS_H_
#define BAIK_EM_COMMON_PLATFORMS_PLATFORM_WINDOWS_H_
#if BAIK_EM_PLATFORM == BAIK_EM_P_WINDOWS


#ifdef _MSC_VER
#pragma warning(disable : 4127)
#pragma warning(disable : 4204)
#endif

#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#endif

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <assert.h>
#include <direct.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <limits.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <ctype.h>

#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <process.h>

#if _MSC_VER < 1700
typedef int bool;
#else
#include <stdbool.h>
#endif

#if defined(_MSC_VER) && _MSC_VER >= 1800
#define strdup _strdup
#endif

#ifndef EINPROGRESS
#define EINPROGRESS WSAEINPROGRESS
#endif
#ifndef EWOULDBLOCK
#define EWOULDBLOCK WSAEWOULDBLOCK
#endif
#ifndef __func__
#define STRX(x) #x
#define STR(x) STRX(x)
#define __func__ __FILE__ ":" STR(__LINE__)
#endif
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define to64(x) _atoi64(x)
#if !defined(__MINGW32__) && !defined(__MINGW64__)
#define popen(x, y) _popen((x), (y))
#define pclose(x) _pclose(x)
#define fileno _fileno
#endif
#if defined(_MSC_VER) && _MSC_VER >= 1400
#define fseeko(x, y, z) _fseeki64((x), (y), (z))
#else
#define fseeko(x, y, z) fseek((x), (y), (z))
#endif
#if defined(_MSC_VER) && _MSC_VER <= 1200
typedef unsigned long uintptr_t;
typedef long intptr_t;
#endif
typedef int socklen_t;
#if _MSC_VER >= 1700
#include <stdint.h>
#else
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#endif
typedef SOCKET sock_t;
typedef uint32_t in_addr_t;
#ifndef UINT16_MAX
#define UINT16_MAX 65535
#endif
#ifndef UINT32_MAX
#define UINT32_MAX 4294967295
#endif
#ifndef pid_t
#define pid_t HANDLE
#endif
#define INT64_FMT "I64d"
#define INT64_X_FMT "I64x"
#define SIZE_T_FMT "Iu"
typedef struct _stati64 BAIK_EM_stat_t;
#ifndef S_ISDIR
#define S_ISDIR(x) (((x) &_S_IFMT) == _S_IFDIR)
#endif
#ifndef S_ISREG
#define S_ISREG(x) (((x) &_S_IFMT) == _S_IFREG)
#endif
#define DIRSEP '\\'
#define BAIK_EM_DEFINE_DIRENT

#ifndef va_copy
#ifdef __va_copy
#define va_copy __va_copy
#else
#define va_copy(x, y) (x) = (y)
#endif
#endif

#ifndef GENERIC_MAX_HTTP_REQUEST_SIZE
#define GENERIC_MAX_HTTP_REQUEST_SIZE 8192
#endif

#ifndef GENERIC_MAX_HTTP_SEND_MBUF
#define GENERIC_MAX_HTTP_SEND_MBUF 4096
#endif

#ifndef GENERIC_MAX_HTTP_HEADERS
#define GENERIC_MAX_HTTP_HEADERS 40
#endif

#ifndef BAIK_EM_ENABLE_STDIO
#define BAIK_EM_ENABLE_STDIO 1
#endif

#ifndef GENERIC_ENABLE_BROADCAST
#define GENERIC_ENABLE_BROADCAST 1
#endif

#ifndef GENERIC_ENABLE_DIRECTORY_LISTING
#define GENERIC_ENABLE_DIRECTORY_LISTING 1
#endif

#ifndef GENERIC_ENABLE_FILESYSTEM
#define GENERIC_ENABLE_FILESYSTEM 1
#endif

#ifndef GENERIC_ENABLE_HTTP_CGI
#define GENERIC_ENABLE_HTTP_CGI GENERIC_ENABLE_FILESYSTEM
#endif

#ifndef GENERIC_NET_IF
#define GENERIC_NET_IF GENERIC_NET_IF_SOCKET
#endif

unsigned int sleep(unsigned int seconds);


#define timegm _mkgmtime

#define gmtime_r(a, b) \
  do {                 \
    *(b) = *gmtime(a); \
  } while (0)

#endif
#endif
#ifndef BAIK_EM_COMMON_PLATFORMS_PLATFORM_UNIX_H_
#define BAIK_EM_COMMON_PLATFORMS_PLATFORM_UNIX_H_
#if BAIK_EM_PLATFORM == BAIK_EM_P_UNIX

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600
#endif

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#ifndef _LARGEFILE_SOURCE
#define _LARGEFILE_SOURCE
#endif

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif

#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <limits.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef __APPLE__
#include <machine/endian.h>
#ifndef BYTE_ORDER
#define LITTLE_ENDIAN __DARWIN_LITTLE_ENDIAN
#define BIG_ENDIAN __DARWIN_BIG_ENDIAN
#define PDP_ENDIAN __DARWIN_PDP_ENDIAN
#define BYTE_ORDER __DARWIN_BYTE_ORDER
#endif
#endif

#if !(defined(__cplusplus) && __cplusplus >= 201103L) && \
    !(defined(__DARWIN_C_LEVEL) && __DARWIN_C_LEVEL >= 200809L)
long long strtoll(const char *, char **, int);
#endif

typedef int sock_t;
#define INVALID_SOCKET (-1)
#define SIZE_T_FMT "zu"
typedef struct stat BAIK_EM_stat_t;
#define DIRSEP '/'
#define to64(x) strtoll(x, NULL, 10)
#define INT64_FMT PRId64
#define INT64_X_FMT PRIx64

#ifndef __cdecl
#define __cdecl
#endif

#ifndef va_copy
#ifdef __va_copy
#define va_copy __va_copy
#else
#define va_copy(x, y) (x) = (y)
#endif
#endif

#define closesocket(x) close(x)

#ifndef GENERIC_MAX_HTTP_REQUEST_SIZE
#define GENERIC_MAX_HTTP_REQUEST_SIZE 8192
#endif

#ifndef GENERIC_MAX_HTTP_SEND_MBUF
#define GENERIC_MAX_HTTP_SEND_MBUF 4096
#endif

#ifndef GENERIC_MAX_HTTP_HEADERS
#define GENERIC_MAX_HTTP_HEADERS 40
#endif

#ifndef BAIK_EM_ENABLE_STDIO
#define BAIK_EM_ENABLE_STDIO 1
#endif

#ifndef GENERIC_ENABLE_BROADCAST
#define GENERIC_ENABLE_BROADCAST 1
#endif

#ifndef GENERIC_ENABLE_DIRECTORY_LISTING
#define GENERIC_ENABLE_DIRECTORY_LISTING 1
#endif

#ifndef GENERIC_ENABLE_FILESYSTEM
#define GENERIC_ENABLE_FILESYSTEM 1
#endif

#ifndef GENERIC_ENABLE_HTTP_CGI
#define GENERIC_ENABLE_HTTP_CGI GENERIC_ENABLE_FILESYSTEM
#endif

#ifndef GENERIC_NET_IF
#define GENERIC_NET_IF GENERIC_NET_IF_SOCKET
#endif

#ifndef GENERIC_HOSTS_FILE_NAME
#define GENERIC_HOSTS_FILE_NAME "/etc/hosts"
#endif

#ifndef GENERIC_RESOLV_CONF_FILE_NAME
#define GENERIC_RESOLV_CONF_FILE_NAME "/etc/resolv.conf"
#endif

#endif
#endif

#ifndef BAIK_EM_COMMON_PLATFORMS_PLATFORM_ESP32_H_
#define BAIK_EM_COMMON_PLATFORMS_PLATFORM_ESP32_H_
#if BAIK_EM_PLATFORM == BAIK_EM_P_ESP32

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <inttypes.h>
#include <machine/endian.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>

#define SIZE_T_FMT "u"
typedef struct stat BAIK_EM_stat_t;
#define DIRSEP '/'
#define to64(x) strtoll(x, NULL, 10)
#define INT64_FMT PRId64
#define INT64_X_FMT PRIx64
#define __cdecl
#define _FILE_OFFSET_BITS 32

#define GENERIC_LWIP 1

#ifndef GENERIC_NET_IF
#define GENERIC_NET_IF GENERIC_NET_IF_SOCKET
#endif

#ifndef BAIK_EM_ENABLE_STDIO
#define BAIK_EM_ENABLE_STDIO 1
#endif

#endif
#endif

#ifndef BAIK_EM_COMMON_PLATFORMS_PLATFORM_ESP8266_H_
#define BAIK_EM_COMMON_PLATFORMS_PLATFORM_ESP8266_H_
#if BAIK_EM_PLATFORM == BAIK_EM_P_ESP8266

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <inttypes.h>
#include <machine/endian.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>

#define SIZE_T_FMT "u"
typedef struct stat BAIK_EM_stat_t;
#define DIRSEP '/'
#if !defined(MGOS_VFS_DEFINE_DIRENT)
#define BAIK_EM_DEFINE_DIRENT
#endif

#define to64(x) strtoll(x, NULL, 10)
#define INT64_FMT PRId64
#define INT64_X_FMT PRIx64
#define __cdecl
#define _FILE_OFFSET_BITS 32

#define GENERIC_LWIP 1


#define LWIP_TIMEVAL_PRIVATE 0

#ifndef GENERIC_NET_IF
#include <lwip/opt.h>
#if LWIP_SOCKET
#define GENERIC_NET_IF GENERIC_NET_IF_SOCKET
#else
#define GENERIC_NET_IF GENERIC_NET_IF_LWIP_LOW_LEVEL
#endif
#endif

#ifndef BAIK_EM_ENABLE_STDIO
#define BAIK_EM_ENABLE_STDIO 1
#endif

#define inet_ntop(af, src, dst, size)                                          \
  (((af) == AF_INET) ? ipaddr_ntoa_r((const ip_addr_t *) (src), (dst), (size)) \
                     : NULL)
#define inet_pton(af, src, dst) \
  (((af) == AF_INET) ? ipaddr_aton((src), (ip_addr_t *) (dst)) : 0)

#endif
#endif

#ifndef BAIK_EM_COMMON_PLATFORMS_PLATFORM_CC3100_H_
#define BAIK_EM_COMMON_PLATFORMS_PLATFORM_CC3100_H_
#if BAIK_EM_PLATFORM == BAIK_EM_P_CC3100

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define GENERIC_NET_IF GENERIC_NET_IF_SIMPLELINK
#define GENERIC_SSL_IF GENERIC_SSL_IF_SIMPLELINK

#include <simplelink.h>
#include <netapp.h>
#undef timeval

typedef int sock_t;
#define INVALID_SOCKET (-1)

#define to64(x) strtoll(x, NULL, 10)
#define INT64_FMT PRId64
#define INT64_X_FMT PRIx64
#define SIZE_T_FMT "u"

#define SOMAXCONN 8

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
char *inet_ntoa(struct in_addr in);
int inet_pton(int af, const char *src, void *dst);

#endif
#endif

#ifndef BAIK_EM_COMMON_PLATFORMS_SIMPLELINK_BAIK_EM_SIMPLELINK_H_
#define BAIK_EM_COMMON_PLATFORMS_SIMPLELINK_BAIK_EM_SIMPLELINK_H_

#if defined(GENERIC_NET_IF) && GENERIC_NET_IF == GENERIC_NET_IF_SIMPLELINK

#if !defined(__SIMPLELINK_H__)

#include <stdbool.h>

#ifndef __TI_COMPILER_VERSION__
#undef __CONCAT
#undef FD_CLR
#undef FD_ISSET
#undef FD_SET
#undef FD_SETSIZE
#undef FD_ZERO
#undef fd_set
#endif

#if BAIK_EM_PLATFORM == BAIK_EM_P_CC3220
#include <ti/drivers/net/wifi/porting/user.h>
#include <ti/drivers/net/wifi/simplelink.h>
#include <ti/drivers/net/wifi/sl_socket.h>
#include <ti/drivers/net/wifi/netapp.h>
#else

#define PROVISIONING_API_H_
#include <simplelink/user.h>
#undef PROVISIONING_API_H_
#undef SL_INC_STD_BSD_API_NAMING

#include <simplelink/include/simplelink.h>
#include <simplelink/include/netapp.h>
#endif


#define AF_INET SL_AF_INET

#define socklen_t SlSocklen_t
#define sockaddr SlSockAddr_t
#define sockaddr_in SlSockAddrIn_t
#define in_addr SlInAddr_t

#define SOCK_STREAM SL_SOCK_STREAM
#define SOCK_DGRAM SL_SOCK_DGRAM

#define htonl sl_Htonl
#define ntohl sl_Ntohl
#define htons sl_Htons
#define ntohs sl_Ntohs

#ifndef EACCES
#define EACCES SL_EACCES
#endif
#ifndef EAFNOSUPPORT
#define EAFNOSUPPORT SL_EAFNOSUPPORT
#endif
#ifndef EAGAIN
#define EAGAIN SL_EAGAIN
#endif
#ifndef EBADF
#define EBADF SL_EBADF
#endif
#ifndef EINVAL
#define EINVAL SL_EINVAL
#endif
#ifndef ENOMEM
#define ENOMEM SL_ENOMEM
#endif
#ifndef EWOULDBLOCK
#define EWOULDBLOCK SL_EWOULDBLOCK
#endif

#define SOMAXCONN 8

#ifdef __cplusplus
extern "C" {
#endif

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
char *inet_ntoa(struct in_addr in);
int inet_pton(int af, const char *src, void *dst);

struct baik_generic_mgr;
struct baik_generic_connection;

typedef void (*baik_generic_init_cb)(struct baik_generic_mgr *mgr);
bool baik_generic_start_task(int priority, int stack_size, baik_generic_init_cb baik_generic_init);

void baik_generic_run_in_task(void (*cb)(struct baik_generic_mgr *mgr, void *arg), void *cb_arg);

int sl_fs_init(void);

void sl_restart_cb(struct baik_generic_mgr *mgr);

int sl_set_ssl_opts(int sock, struct baik_generic_connection *nc);

#ifdef __cplusplus
}
#endif

#endif


#if SL_MAJOR_VERSION_NUM < 2

#define SL_ERROR_BSD_EAGAIN SL_EAGAIN
#define SL_ERROR_BSD_EALREADY SL_EALREADY
#define SL_ERROR_BSD_ENOPROTOOPT SL_ENOPROTOOPT
#define SL_ERROR_BSD_ESECDATEERROR SL_ESECDATEERROR
#define SL_ERROR_BSD_ESECSNOVERIFY SL_ESECSNOVERIFY
#define SL_ERROR_FS_FAILED_TO_ALLOCATE_MEM SL_FS_ERR_FAILED_TO_ALLOCATE_MEM
#define SL_ERROR_FS_FILE_HAS_NOT_BEEN_CLOSE_CORRECTLY \
  SL_FS_FILE_HAS_NOT_BEEN_CLOSE_CORRECTLY
#define SL_ERROR_FS_FILE_NAME_EXIST SL_FS_FILE_NAME_EXIST
#define SL_ERROR_FS_FILE_NOT_EXISTS SL_FS_ERR_FILE_NOT_EXISTS
#define SL_ERROR_FS_NO_AVAILABLE_NV_INDEX SL_FS_ERR_NO_AVAILABLE_NV_INDEX
#define SL_ERROR_FS_NOT_ENOUGH_STORAGE_SPACE SL_FS_ERR_NO_AVAILABLE_BLOCKS
#define SL_ERROR_FS_NOT_SUPPORTED SL_FS_ERR_NOT_SUPPORTED
#define SL_ERROR_FS_WRONG_FILE_NAME SL_FS_WRONG_FILE_NAME
#define SL_ERROR_FS_INVALID_HANDLE SL_FS_ERR_INVALID_HANDLE
#define SL_NETCFG_MAC_ADDRESS_GET SL_MAC_ADDRESS_GET
#define SL_SOCKET_FD_ZERO SL_FD_ZERO
#define SL_SOCKET_FD_SET SL_FD_SET
#define SL_SOCKET_FD_ISSET SL_FD_ISSET
#define SL_SO_SECURE_DOMAIN_NAME_VERIFICATION SO_SECURE_DOMAIN_NAME_VERIFICATION

#define SL_FS_READ FS_MODE_OPEN_READ
#define SL_FS_WRITE FS_MODE_OPEN_WRITE

#define SL_FI_FILE_SIZE(fi) ((fi).FileLen)
#define SL_FI_FILE_MAX_SIZE(fi) ((fi).AllocatedLen)

#define SlDeviceVersion_t SlVersionFull
#define sl_DeviceGet sl_DevGet
#define SL_DEVICE_GENERAL SL_DEVICE_GENERAL_CONFIGURATION
#define SL_LEN_TYPE _u8
#define SL_OPT_TYPE _u8

#else

#define FS_MODE_OPEN_CREATE(max_size, flag) \
  (SL_FS_CREATE | SL_FS_CREATE_MAX_SIZE(max_size))
#define SL_FI_FILE_SIZE(fi) ((fi).Len)
#define SL_FI_FILE_MAX_SIZE(fi) ((fi).MaxSize)

#define SL_LEN_TYPE _u16
#define SL_OPT_TYPE _u16

#endif

int slfs_open(const unsigned char *fname, uint32_t flags, uint32_t *token);

#endif

#endif

#ifndef BAIK_EM_COMMON_PLATFORMS_PLATFORM_CC3200_H_
#define BAIK_EM_COMMON_PLATFORMS_PLATFORM_CC3200_H_
#if BAIK_EM_PLATFORM == BAIK_EM_P_CC3200

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#ifndef __TI_COMPILER_VERSION__
#include <fcntl.h>
#include <sys/time.h>
#endif

#define GENERIC_NET_IF GENERIC_NET_IF_SIMPLELINK
#define GENERIC_SSL_IF GENERIC_SSL_IF_SIMPLELINK

#if defined(CC3200_FS_SPIFFS) && !defined(GENERIC_ENABLE_DIRECTORY_LISTING)
#define GENERIC_ENABLE_DIRECTORY_LISTING 1
#endif

typedef int sock_t;
#define INVALID_SOCKET (-1)
#define SIZE_T_FMT "u"
typedef struct stat BAIK_EM_stat_t;
#define DIRSEP '/'
#define to64(x) strtoll(x, NULL, 10)
#define INT64_FMT PRId64
#define INT64_X_FMT PRIx64
#define __cdecl

#define fileno(x) -1

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __TI_COMPILER_VERSION__
struct SlTimeval_t;
#define timeval SlTimeval_t
int gettimeofday(struct timeval *t, void *tz);
int settimeofday(const struct timeval *tv, const void *tz);
int asprintf(char **strp, const char *fmt, ...);
#endif

#ifdef __TI_COMPILER_VERSION__
#include <file.h>

typedef unsigned int mode_t;
typedef size_t _off_t;
typedef long ssize_t;

struct stat {
  int st_ino;
  mode_t st_mode;
  int st_nlink;
  time_t st_mtime;
  off_t st_size;
};

int _stat(const char *pathname, struct stat *st);
int stat(const char *pathname, struct stat *st);

#define __S_IFMT 0170000

#define __S_IFDIR 0040000
#define __S_IFCHR 0020000
#define __S_IFREG 0100000

#define __S_ISTYPE(mode, mask) (((mode) &__S_IFMT) == (mask))

#define S_IFDIR __S_IFDIR
#define S_IFCHR __S_IFCHR
#define S_IFREG __S_IFREG
#define S_ISDIR(mode) __S_ISTYPE((mode), __S_IFDIR)
#define S_ISREG(mode) __S_ISTYPE((mode), __S_IFREG)

#if __TI_COMPILER_VERSION__ < 16000000
#define va_copy(apc, ap) ((apc) = (ap))
#endif

#endif

#ifdef CC3200_FS_SLFS
#define GENERIC_FS_SLFS
#endif

#if (defined(CC3200_FS_SPIFFS) || defined(CC3200_FS_SLFS)) && \
    !defined(GENERIC_ENABLE_FILESYSTEM)
#define GENERIC_ENABLE_FILESYSTEM 1
#define BAIK_EM_DEFINE_DIRENT
#endif

#ifndef BAIK_EM_ENABLE_STDIO
#define BAIK_EM_ENABLE_STDIO 1
#endif

#ifdef __cplusplus
}
#endif

#endif
#endif

#ifndef BAIK_EM_COMMON_PLATFORMS_PLATFORM_CC3220_H_
#define BAIK_EM_COMMON_PLATFORMS_PLATFORM_CC3220_H_
#if BAIK_EM_PLATFORM == BAIK_EM_P_CC3220

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#ifndef __TI_COMPILER_VERSION__
#include <fcntl.h>
#include <sys/time.h>
#endif

#define GENERIC_NET_IF GENERIC_NET_IF_SIMPLELINK
#ifndef GENERIC_SSL_IF
#define GENERIC_SSL_IF GENERIC_SSL_IF_SIMPLELINK
#endif

#if defined(CC3220_FS_SPIFFS) && !defined(GENERIC_ENABLE_DIRECTORY_LISTING)
#define GENERIC_ENABLE_DIRECTORY_LISTING 1
#endif

typedef int sock_t;
#define INVALID_SOCKET (-1)
#define SIZE_T_FMT "u"
typedef struct stat BAIK_EM_stat_t;
#define DIRSEP '/'
#define to64(x) strtoll(x, NULL, 10)
#define INT64_FMT PRId64
#define INT64_X_FMT PRIx64
#define __cdecl

#define fileno(x) -1

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __TI_COMPILER_VERSION__
struct SlTimeval_t;
#define timeval SlTimeval_t
int gettimeofday(struct timeval *t, void *tz);
int settimeofday(const struct timeval *tv, const void *tz);

int asprintf(char **strp, const char *fmt, ...);

#endif

#ifdef __TI_COMPILER_VERSION__

#include <file.h>

typedef unsigned int mode_t;
typedef size_t _off_t;
typedef long ssize_t;

struct stat {
  int st_ino;
  mode_t st_mode;
  int st_nlink;
  time_t st_mtime;
  off_t st_size;
};

int _stat(const char *pathname, struct stat *st);
int stat(const char *pathname, struct stat *st);

#define __S_IFMT 0170000
#define __S_IFDIR 0040000
#define __S_IFCHR 0020000
#define __S_IFREG 0100000
#define __S_ISTYPE(mode, mask) (((mode) &__S_IFMT) == (mask))
#define S_IFDIR __S_IFDIR
#define S_IFCHR __S_IFCHR
#define S_IFREG __S_IFREG
#define S_ISDIR(mode) __S_ISTYPE((mode), __S_IFDIR)
#define S_ISREG(mode) __S_ISTYPE((mode), __S_IFREG)

#endif

#ifndef BAIK_EM_ENABLE_STDIO
#define BAIK_EM_ENABLE_STDIO 1
#endif

#ifdef __cplusplus
}
#endif

#endif
#endif

#ifndef BAIK_EM_COMMON_PLATFORMS_PLATFORM_MBED_H_
#define BAIK_EM_COMMON_PLATFORMS_PLATFORM_MBED_H_
#if BAIK_EM_PLATFORM == BAIK_EM_P_MBED


#ifdef __cplusplus

#endif

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>

typedef struct stat BAIK_EM_stat_t;
#define DIRSEP '/'

#ifndef BAIK_EM_ENABLE_STDIO
#define BAIK_EM_ENABLE_STDIO 1
#endif


#if defined(__ARMCC_VERSION) || defined(__ICCARM__)
#define _TIMEVAL_DEFINED
#define gettimeofday _gettimeofday


typedef long suseconds_t;
struct timeval {
  time_t tv_sec;      
  suseconds_t tv_usec;
};

#endif

#if GENERIC_NET_IF == GENERIC_NET_IF_SIMPLELINK

#define GENERIC_SIMPLELINK_NO_OSI 1

#include <simplelink.h>

typedef int sock_t;
#define INVALID_SOCKET (-1)

#define to64(x) strtoll(x, NULL, 10)
#define INT64_FMT PRId64
#define INT64_X_FMT PRIx64
#define SIZE_T_FMT "u"

#define SOMAXCONN 8

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
char *inet_ntoa(struct in_addr in);
int inet_pton(int af, const char *src, void *dst);
int inet_aton(const char *cp, struct in_addr *inp);
in_addr_t inet_addr(const char *cp);

#endif

#endif
#endif

#ifndef BAIK_EM_COMMON_PLATFORMS_PLATFORM_NRF51_H_
#define BAIK_EM_COMMON_PLATFORMS_PLATFORM_NRF51_H_
#if BAIK_EM_PLATFORM == BAIK_EM_P_NRF51

#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define to64(x) strtoll(x, NULL, 10)
#define GENERIC_NET_IF GENERIC_NET_IF_LWIP_LOW_LEVEL
#define GENERIC_LWIP 1
#define GENERIC_ENABLE_IPV6 1

#if !defined(__ARMCC_VERSION)
#define LWIP_TIMEVAL_PRIVATE 0
#else
struct timeval;
int gettimeofday(struct timeval *tp, void *tzp);
#endif

#define INT64_FMT PRId64
#define SIZE_T_FMT "u"
#define BAIK_EM_ENABLE_STRDUP defined(__ARMCC_VERSION)

#endif
#endif

#ifndef BAIK_EM_COMMON_PLATFORMS_PLATFORM_NRF52_H_
#define BAIK_EM_COMMON_PLATFORMS_PLATFORM_NRF52_H_
#if BAIK_EM_PLATFORM == BAIK_EM_P_NRF52

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define to64(x) strtoll(x, NULL, 10)

#define GENERIC_NET_IF GENERIC_NET_IF_LWIP_LOW_LEVEL
#define GENERIC_LWIP 1
#define GENERIC_ENABLE_IPV6 1

#if !defined(ENOSPC)
#define ENOSPC 28
#endif


#if !defined(__ARMCC_VERSION)
#define LWIP_TIMEVAL_PRIVATE 0
#endif

#define INT64_FMT PRId64
#define SIZE_T_FMT "u"

#define BAIK_EM_ENABLE_STRDUP defined(__ARMCC_VERSION)

#endif
#endif

#ifndef BAIK_EM_COMMON_PLATFORMS_PLATFORM_WINCE_H_
#define BAIK_EM_COMMON_PLATFORMS_PLATFORM_WINCE_H_

#if BAIK_EM_PLATFORM == BAIK_EM_P_WINCE

#pragma warning(disable : 4127)
#pragma warning(disable : 4204)

#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#endif

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#pragma comment(lib, "ws2.lib")

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#define strdup _strdup

#ifndef EINPROGRESS
#define EINPROGRESS WSAEINPROGRESS
#endif

#ifndef EWOULDBLOCK
#define EWOULDBLOCK WSAEWOULDBLOCK
#endif

#ifndef EAGAIN
#define EAGAIN EWOULDBLOCK
#endif

#ifndef __func__
#define STRX(x) #x
#define STR(x) STRX(x)
#define __func__ __FILE__ ":" STR(__LINE__)
#endif

#define snprintf _snprintf
#define fileno _fileno
#define vsnprintf _vsnprintf
#define sleep(x) Sleep((x) *1000)
#define to64(x) _atoi64(x)
#define rmdir _rmdir

#if defined(_MSC_VER) && _MSC_VER >= 1400
#define fseeko(x, y, z) _fseeki64((x), (y), (z))
#else
#define fseeko(x, y, z) fseek((x), (y), (z))
#endif

typedef int socklen_t;

#if _MSC_VER >= 1700
#include <stdint.h>
#else
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#endif

typedef SOCKET sock_t;
typedef uint32_t in_addr_t;

#ifndef UINT16_MAX
#define UINT16_MAX 65535
#endif

#ifndef UINT32_MAX
#define UINT32_MAX 4294967295
#endif

#ifndef pid_t
#define pid_t HANDLE
#endif

#define INT64_FMT "I64d"
#define INT64_X_FMT "I64x"

#define SIZE_T_FMT "u"

#define DIRSEP '\\'
#define BAIK_EM_DEFINE_DIRENT

#ifndef va_copy
#ifdef __va_copy
#define va_copy __va_copy
#else
#define va_copy(x, y) (x) = (y)
#endif
#endif

#ifndef GENERIC_MAX_HTTP_REQUEST_SIZE
#define GENERIC_MAX_HTTP_REQUEST_SIZE 8192
#endif

#ifndef GENERIC_MAX_HTTP_SEND_MBUF
#define GENERIC_MAX_HTTP_SEND_MBUF 4096
#endif

#ifndef GENERIC_MAX_HTTP_HEADERS
#define GENERIC_MAX_HTTP_HEADERS 40
#endif

#ifndef BAIK_EM_ENABLE_STDIO
#define BAIK_EM_ENABLE_STDIO 1
#endif

#define abort() DebugBreak();

#ifndef BUFSIZ
#define BUFSIZ 4096
#endif

#ifndef GENERIC_ENABLE_THREADS
#define GENERIC_ENABLE_THREADS 0
#endif

#ifndef GENERIC_ENABLE_FILESYSTEM
#define GENERIC_ENABLE_FILESYSTEM 1
#endif

#ifndef GENERIC_NET_IF
#define GENERIC_NET_IF GENERIC_NET_IF_SOCKET
#endif

typedef struct _stati64 {
  uint32_t st_mtime;
  uint32_t st_size;
  uint32_t st_mode;
} BAIK_EM_stat_t;



#ifndef ENOENT
#define ENOENT ERROR_PATH_NOT_FOUND
#endif

#ifndef EACCES
#define EACCES ERROR_ACCESS_DENIED
#endif

#ifndef ENOMEM
#define ENOMEM ERROR_NOT_ENOUGH_MEMORY
#endif

#ifndef _UINTPTR_T_DEFINED
typedef unsigned int *uintptr_t;
#endif

#define _S_IFREG 2
#define _S_IFDIR 4

#ifndef S_ISDIR
#define S_ISDIR(x) (((x) &_S_IFDIR) != 0)
#endif

#ifndef S_ISREG
#define S_ISREG(x) (((x) &_S_IFREG) != 0)
#endif

int open(const char *filename, int oflag, int pmode);
int _wstati64(const wchar_t *path, BAIK_EM_stat_t *st);
const char *strerror();

#endif
#endif

#ifndef BAIK_EM_COMMON_PLATFORMS_PLATFORM_NXP_LPC_H_
#define BAIK_EM_COMMON_PLATFORMS_PLATFORM_NXP_LPC_H_
#if BAIK_EM_PLATFORM == BAIK_EM_P_NXP_LPC

#include <ctype.h>
#include <stdint.h>
#include <string.h>

#define SIZE_T_FMT "u"
typedef struct stat BAIK_EM_stat_t;
#define INT64_FMT "lld"
#define INT64_X_FMT "llx"
#define __cdecl

#define GENERIC_LWIP 1

#define GENERIC_NET_IF GENERIC_NET_IF_LWIP_LOW_LEVEL


#ifdef __REDLIB_INTERFACE_VERSION__


#define LWIP_TIMEVAL_PRIVATE 1

#define va_copy(d, s) __builtin_va_copy(d, s)

#define BAIK_EM_ENABLE_TO64 1
#define to64(x) BAIK_EM_to64(x)

#define BAIK_EM_ENABLE_STRDUP 1

#else

#include <sys/time.h>
#define LWIP_TIMEVAL_PRIVATE 0
#define to64(x) strtoll(x, NULL, 10)

#endif

#endif
#endif

#ifndef BAIK_EM_COMMON_PLATFORMS_PLATFORM_NXP_KINETIS_H_
#define BAIK_EM_COMMON_PLATFORMS_PLATFORM_NXP_KINETIS_H_
#if BAIK_EM_PLATFORM == BAIK_EM_P_NXP_KINETIS

#include <ctype.h>
#include <inttypes.h>
#include <string.h>
#include <sys/time.h>

#define SIZE_T_FMT "u"
typedef struct stat BAIK_EM_stat_t;
#define to64(x) strtoll(x, NULL, 10)
#define INT64_FMT "lld"
#define INT64_X_FMT "llx"
#define __cdecl

#define GENERIC_LWIP 1

#define GENERIC_NET_IF GENERIC_NET_IF_LWIP_LOW_LEVEL


#define LWIP_TIMEVAL_PRIVATE 0

#endif
#endif

#ifndef BAIK_EM_COMMON_PLATFORMS_PLATFORM_PIC32_H_
#define BAIK_EM_COMMON_PLATFORMS_PLATFORM_PIC32_H_

#if BAIK_EM_PLATFORM == BAIK_EM_P_PIC32

#define GENERIC_NET_IF GENERIC_NET_IF_PIC32

#include <stdint.h>
#include <time.h>
#include <ctype.h>
#include <stdlib.h>
#include <system_config.h>
#include <system_definitions.h>
#include <sys/types.h>

typedef TCP_SOCKET sock_t;
#define to64(x) strtoll(x, NULL, 10)

#define SIZE_T_FMT "lu"
#define INT64_FMT "lld"

#ifndef BAIK_EM_ENABLE_STDIO
#define BAIK_EM_ENABLE_STDIO 1
#endif

char *inet_ntoa(struct in_addr in);

#endif

#endif

#ifndef BAIK_EM_COMMON_PLATFORMS_PLATFORM_RS14100_H_
#define BAIK_EM_COMMON_PLATFORMS_PLATFORM_RS14100_H_
#if BAIK_EM_PLATFORM == BAIK_EM_P_RS14100

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef MGOS_HAVE_VFS_COMMON
#include <mgos_vfs.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define to64(x) strtoll(x, NULL, 10)
#define INT64_FMT "lld"
#define SIZE_T_FMT "u"
typedef struct stat BAIK_EM_stat_t;
#define DIRSEP '/'

#ifndef BAIK_EM_ENABLE_STDIO
#define BAIK_EM_ENABLE_STDIO 1
#endif

#ifndef GENERIC_ENABLE_FILESYSTEM
#define GENERIC_ENABLE_FILESYSTEM 1
#endif

#ifdef __cplusplus
}
#endif

#endif
#endif

#ifndef BAIK_EM_COMMON_PLATFORMS_PLATFORM_STM32_H_
#define BAIK_EM_COMMON_PLATFORMS_PLATFORM_STM32_H_
#if BAIK_EM_PLATFORM == BAIK_EM_P_STM32

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>

#include <stm32_sdk_hal.h>

#define to64(x) strtoll(x, NULL, 10)
#define INT64_FMT "lld"
#define SIZE_T_FMT "u"
typedef struct stat BAIK_EM_stat_t;
#define DIRSEP '/'

#ifndef BAIK_EM_ENABLE_STDIO
#define BAIK_EM_ENABLE_STDIO 1
#endif

#ifndef GENERIC_ENABLE_FILESYSTEM
#define GENERIC_ENABLE_FILESYSTEM 1
#endif

#endif
#endif

#ifndef BAIK_EM_COMMON_BAIK_EM_DBG_H_
#define BAIK_EM_COMMON_BAIK_EM_DBG_H_

#if BAIK_EM_ENABLE_STDIO
#include <stdio.h>
#endif

#ifndef BAIK_EM_ENABLE_DEBUG
#define BAIK_EM_ENABLE_DEBUG 0
#endif

#ifndef BAIK_EM_LOG_PREFIX_LEN
#define BAIK_EM_LOG_PREFIX_LEN 24
#endif

#ifndef BAIK_EM_LOG_ENABLE_TS_DIFF
#define BAIK_EM_LOG_ENABLE_TS_DIFF 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum BAIK_EM_log_level {
  LL_NONE = -1,
  LL_ERROR = 0,
  LL_WARN = 1,
  LL_INFO = 2,
  LL_DEBUG = 3,
  LL_VERBOSE_DEBUG = 4,

  _LL_MIN = -2,
  _LL_MAX = 5,
};


void BAIK_EM_log_set_level(enum BAIK_EM_log_level level);
void BAIK_EM_log_set_file_level(const char *file_level);
int BAIK_EM_log_print_prefix(enum BAIK_EM_log_level level, const char *fname, int line);

extern enum BAIK_EM_log_level BAIK_EM_log_level;

#if BAIK_EM_ENABLE_STDIO

void BAIK_EM_log_set_file(FILE *file);
void BAIK_EM_log_printf(const char *fmt, ...) PRINTF_LIKE(1, 2);

#if BAIK_EM_ENABLE_STDIO
#define LOG(l, x)                                     \
  do {                                                \
    if (BAIK_EM_log_print_prefix(l, __FILE__, __LINE__)) { \
      BAIK_EM_log_printf x;                                \
    }                                                 \
  } while (0)
#else
#define LOG(l, x) ((void) l)
#endif

#ifndef BAIK_EM_NDEBUG
#define DBG(x) LOG(LL_VERBOSE_DEBUG, x)
#else

#define DBG(x)

#endif

#else
#define LOG(l, x)
#define DBG(x)
#endif

#ifdef __cplusplus
}
#endif

#endif

#ifndef BAIK_EM_COMMON_BAIK_EM_TIME_H_
#define BAIK_EM_COMMON_BAIK_EM_TIME_H_

#include <time.h>
#ifdef __cplusplus
extern "C" {
#endif


double BAIK_EM_time(void);


double BAIK_EM_timegm(const struct tm *tm);

#ifdef __cplusplus
}
#endif

#endif

#ifndef BAIK_EM_COMMON_GENERIC_STR_H_
#define BAIK_EM_COMMON_GENERIC_STR_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct baik_generic_str {
  const char *p;
  size_t len;   
};

struct baik_generic_str baik_generic_mk_str(const char *s);
struct baik_generic_str baik_generic_mk_str_n(const char *s, size_t len);

#define GENERIC_MK_STR(str_literal) \
  { str_literal, sizeof(str_literal) - 1 }
#define GENERIC_MK_STR_N(str_literal, len) \
  { str_literal, len }
#define GENERIC_NULL_STR \
  { NULL, 0 }

int baik_generic_vcmp(const struct baik_generic_str *str2, const char *str1);
int baik_generic_vcasecmp(const struct baik_generic_str *str2, const char *str1);
struct baik_generic_str baik_generic_strdup(const struct baik_generic_str s);
struct baik_generic_str baik_generic_strdup_nul(const struct baik_generic_str s);
const char *baik_generic_strchr(const struct baik_generic_str s, int c);
int baik_generic_strcmp(const struct baik_generic_str str1, const struct baik_generic_str str2);
int baik_generic_strncmp(const struct baik_generic_str str1, const struct baik_generic_str str2, size_t n);
void baik_generic_strfree(struct baik_generic_str *s);
const char *baik_generic_strstr(const struct baik_generic_str haystack, const struct baik_generic_str needle);
struct baik_generic_str baik_generic_strstrip(struct baik_generic_str s);
int baik_generic_str_starts_with(struct baik_generic_str s, struct baik_generic_str prefix);

#ifdef __cplusplus
}
#endif

#endif

#ifndef BAIK_EM_COMMON_STR_UTIL_H_
#define BAIK_EM_COMMON_STR_UTIL_H_

#include <stdarg.h>
#include <stdlib.h>

#ifndef BAIK_EM_ENABLE_STRDUP
#define BAIK_EM_ENABLE_STRDUP 0
#endif

#ifndef BAIK_EM_ENABLE_TO64
#define BAIK_EM_ENABLE_TO64 0
#endif

#if !defined(_MSC_VER) || _MSC_VER >= 1900
#define BAIK_EM_STRINGIFY_LIT(...) #__VA_ARGS__
#else
#define BAIK_EM_STRINGIFY_LIT(x) #x
#endif

#define BAIK_EM_STRINGIFY_MACRO(x) BAIK_EM_STRINGIFY_LIT(x)

#ifdef __cplusplus
extern "C" {
#endif

size_t c_strnlen(const char *s, size_t maxlen);
int c_snprintf(char *buf, size_t buf_size, const char *format, ...)
    PRINTF_LIKE(3, 4);
int c_vsnprintf(char *buf, size_t buf_size, const char *format, va_list ap);
const char *c_strnstr(const char *s, const char *find, size_t slen);
void BAIK_EM_to_hex(char *to, const unsigned char *p, size_t len);
void BAIK_EM_from_hex(char *to, const char *p, size_t len);

#if BAIK_EM_ENABLE_STRDUP
char *strdup(const char *src);
#endif

#if BAIK_EM_ENABLE_TO64
#include <stdint.h>
int64_t BAIK_EM_to64(const char *s);
#endif
int baik_generic_ncasecmp(const char *s1, const char *s2, size_t len);
int baik_generic_casecmp(const char *s1, const char *s2);
int baik_generic_asprintf(char **buf, size_t size, const char *fmt, ...)
    PRINTF_LIKE(3, 4);
int baik_generic_avprintf(char **buf, size_t size, const char *fmt, va_list ap);
const char *baik_generic_next_comma_list_entry(const char *list, struct baik_generic_str *val,
                                     struct baik_generic_str *eq_val);
struct baik_generic_str baik_generic_next_comma_list_entry_n(struct baik_generic_str list, struct baik_generic_str *val,
                                         struct baik_generic_str *eq_val);
size_t baik_generic_match_prefix(const char *pattern, int pattern_len, const char *str);
size_t baik_generic_match_prefix_n(const struct baik_generic_str pattern, const struct baik_generic_str str);

#ifdef __cplusplus
}
#endif

#endif

#ifndef BAIK_EM_COMMON_BAIK_EM_FILE_H_
#define BAIK_EM_COMMON_BAIK_EM_FILE_H_

#ifdef __cplusplus
extern "C" {
#endif

char *BAIK_EM_read_file(const char *path, size_t *size);

#ifdef BAIK_EM_MMAP
char *BAIK_EM_mmap_file(const char *path, size_t *size);
#endif

#ifdef __cplusplus
}
#endif

#endif

#ifndef BAIK_EM_COMMON_MBUF_H_
#define BAIK_EM_COMMON_MBUF_H_
#include <stdlib.h>

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef MBUF_SIZE_MULTIPLIER
#define MBUF_SIZE_MULTIPLIER 1.5
#endif

#ifndef MBUF_SIZE_MAX_HEADROOM
#ifdef BUFSIZ
#define MBUF_SIZE_MAX_HEADROOM BUFSIZ
#else
#define MBUF_SIZE_MAX_HEADROOM 1024
#endif
#endif

struct mbuf {
  char *buf;  
  size_t len; 
  size_t size;
};

void mbuf_init(struct mbuf *, size_t initial_capacity);
void mbuf_free(struct mbuf *);
size_t mbuf_append(struct mbuf *, const void *data, size_t data_size);
size_t mbuf_append_and_free(struct mbuf *, void *data, size_t data_size);
size_t mbuf_insert(struct mbuf *, size_t, const void *, size_t);
void mbuf_remove(struct mbuf *, size_t data_size);
void mbuf_resize(struct mbuf *, size_t new_size);
void mbuf_move(struct mbuf *from, struct mbuf *to);
void mbuf_clear(struct mbuf *);
void mbuf_trim(struct mbuf *);

#if defined(__cplusplus)
}
#endif

#endif

#ifndef BAIK_EM_COMMON_GENERIC_MEM_H_
#define BAIK_EM_COMMON_GENERIC_MEM_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GENERIC_MALLOC
#define GENERIC_MALLOC malloc
#endif

#ifndef GENERIC_CALLOC
#define GENERIC_CALLOC calloc
#endif

#ifndef GENERIC_REALLOC
#define GENERIC_REALLOC realloc
#endif

#ifndef GENERIC_FREE
#define GENERIC_FREE free
#endif

#ifdef __cplusplus
}
#endif

#endif

#ifndef BAIK_EM_FROZEN_FROZEN_H_
#define BAIK_EM_FROZEN_FROZEN_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#if defined(_WIN32) && _MSC_VER < 1700
typedef int bool;
enum { false = 0, true = 1 };
#else
#include <stdbool.h>
#endif

enum json_token_type {
  JSON_TYPE_INVALID = 0,
  JSON_TYPE_STRING,
  JSON_TYPE_NUMBER,
  JSON_TYPE_TRUE,
  JSON_TYPE_FALSE,
  JSON_TYPE_NULL,
  JSON_TYPE_OBJECT_START,
  JSON_TYPE_OBJECT_END,
  JSON_TYPE_ARRAY_START,
  JSON_TYPE_ARRAY_END,
  JSON_TYPES_CNT
};

struct json_token {
  const char *ptr;          
  int len;                  
  enum json_token_type type;
};

#define JSON_INVALID_TOKEN \
  { 0, 0, JSON_TYPE_INVALID }

#define JSON_STRING_INVALID -1
#define JSON_STRING_INCOMPLETE -2

typedef void (*json_walk_callback_t)(void *callback_data, const char *name,
                                     size_t name_len, const char *path,
                                     const struct json_token *token);
int json_walk(const char *json_string, int json_string_length,
              json_walk_callback_t callback, void *callback_data);
struct json_out {
  int (*printer)(struct json_out *, const char *str, size_t len);
  union {
    struct {
      char *buf;
      size_t size;
      size_t len;
    } buf;
    void *data;
    FILE *fp;
  } u;
};

extern int json_printer_buf(struct json_out *, const char *, size_t);
extern int json_printer_file(struct json_out *, const char *, size_t);

#define JSON_OUT_BUF(buf, len) \
  {                            \
    json_printer_buf, {        \
      { buf, len, 0 }          \
    }                          \
  }
#define JSON_OUT_FILE(fp)   \
  {                         \
    json_printer_file, {    \
      { (char *) fp, 0, 0 } \
    }                       \
  }
typedef int (*json_printf_callback_t)(struct json_out *, va_list *ap);

int json_printf(struct json_out *, const char *fmt, ...);
int json_vprintf(struct json_out *, const char *fmt, va_list ap);
int json_fprintf(const char *file_name, const char *fmt, ...);
int json_vfprintf(const char *file_name, const char *fmt, va_list ap);
char *json_asprintf(const char *fmt, ...);
char *json_vasprintf(const char *fmt, va_list ap);
int json_printf_array(struct json_out *, va_list *ap);
int json_scanf(const char *str, int str_len, const char *fmt, ...);
int json_vscanf(const char *str, int str_len, const char *fmt, va_list ap);
typedef void (*json_scanner_t)(const char *str, int len, void *user_data);
int json_scanf_array_elem(const char *s, int len, const char *path, int index,
                          struct json_token *token);
int json_unescape(const char *src, int slen, char *dst, int dlen);
int json_escape(struct json_out *out, const char *str, size_t str_len);
char *json_fread(const char *file_name);
int json_setf(const char *s, int len, struct json_out *out,
              const char *json_path, const char *json_fmt, ...);
int json_vsetf(const char *s, int len, struct json_out *out,
               const char *json_path, const char *json_fmt, va_list ap);
int json_prettify(const char *s, int len, struct json_out *out);
int json_prettify_file(const char *file_name);
void *json_next_key(const char *s, int len, void *handle, const char *path,
                    struct json_token *key, struct json_token *val);
void *json_next_elem(const char *s, int len, void *handle, const char *path,
                     int *idx, struct json_token *val);
#ifndef JSON_MAX_PATH_LEN
#define JSON_MAX_PATH_LEN 256
#endif

#ifndef JSON_MINIMAL
#define JSON_MINIMAL 0
#endif

#ifndef JSON_ENABLE_BASE64
#define JSON_ENABLE_BASE64 !JSON_MINIMAL
#endif

#ifndef JSON_ENABLE_HEX
#define JSON_ENABLE_HEX !JSON_MINIMAL
#endif

#ifdef __cplusplus
}
#endif

#endif

#ifndef BAIK_FFI_FFI_H_
#define BAIK_FFI_FFI_H_

#if defined(__cplusplus)
extern "C" {
#endif

#define FFI_MAX_ARGS_CNT 6

typedef void(ffi_fn_t)(void);
typedef intptr_t ffi_word_t;

enum ffi_ctype {
  FFI_CTYPE_WORD,
  FFI_CTYPE_BOOL,
  FFI_CTYPE_FLOAT,
  FFI_CTYPE_DOUBLE,
};

struct ffi_arg {
  enum ffi_ctype ctype;
  union {
    uint64_t i;
    double d;
    float f;
  } v;
};

int ffi_call(ffi_fn_t *func, int nargs, struct ffi_arg *res,
             struct ffi_arg *args);

void ffi_set_word(struct ffi_arg *arg, ffi_word_t v);
void ffi_set_bool(struct ffi_arg *arg, bool v);
void ffi_set_ptr(struct ffi_arg *arg, void *v);
void ffi_set_double(struct ffi_arg *arg, double v);
void ffi_set_float(struct ffi_arg *arg, float v);

/* Dideklarasikan `extern`, DIDEFINISIKAN sekali saja di baik_ffi.c.
 *
 * Dulu baris ini berupa definisi tentatif di dalam header. Selama interpreter
 * masih satu berkas, hal itu tidak kelihatan. Setelah dipecah, ke-17 modul
 * masing-masing ikut mendefinisikannya sehingga penaut menolak dengan
 * "multiple definition" pada GCC 10+ (yang berbawaan -fno-common). Toolchain
 * xtensa GCC 8 kebetulan lolos karena masih berbawaan -fcommon - jadi bug ini
 * hanya muncul pada build host. */
extern const char *baik_dlsym_file;

#if defined(__cplusplus)
}
#endif

#endif

#ifndef BAIK_INTERNAL_H_
#define BAIK_INTERNAL_H_

#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifndef FAST
#define FAST
#endif

#ifndef STATIC
#define STATIC
#endif

#ifndef ENDL
#define ENDL "\n"
#endif

#ifdef BAIK_EXPOSE_PRIVATE
#define BAIK_PRIVATE
#define BAIK_EXTERN extern
#else
#define BAIK_PRIVATE static
#define BAIK_EXTERN static
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#if !defined(WEAK)
#if (defined(__GNUC__) || defined(__TI_COMPILER_VERSION__)) && !defined(_WIN32)
#define WEAK __attribute__((weak))
#else
#define WEAK
#endif
#endif

#ifndef BAIK_EM_ENABLE_STDIO
#define BAIK_EM_ENABLE_STDIO 1
#endif

#if defined(_WIN32) && _MSC_VER < 1700
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef __int64 int64_t;
typedef unsigned long uintptr_t;
#define STRX(x) #x
#define STR(x) STRX(x)
#define __func__ __FILE__ ":" STR(__LINE__)

#define vsnprintf _vsnprintf
#define isnan(x) _isnan(x)
#define va_copy(x, y) (x) = (y)
#define BAIK_EM_DEFINE_DIRENT
#include <windows.h>
#else
#if defined(__unix__) || defined(__APPLE__)
#include <dlfcn.h>
#endif
#endif

#ifndef BAIK_INIT_OFFSET_SIZE
#define BAIK_INIT_OFFSET_SIZE 1
#endif

#endif
#ifndef BAIK_FEATURES_H_
#define BAIK_FEATURES_H_

#if !defined(BAIK_AGGRESSIVE_GC)
#define BAIK_AGGRESSIVE_GC 0
#endif

#if !defined(BAIK_MEMORY_STATS)
#define BAIK_MEMORY_STATS 0
#endif


#if !defined(BAIK_GENERATE_INAC)
#if defined(BAIK_EM_MMAP)
#define BAIK_GENERATE_INAC 1
#else
#define BAIK_GENERATE_INAC 0
#endif
#endif

#endif

#ifndef BAIK_CORE_PUBLIC_H_
#define BAIK_CORE_PUBLIC_H_

#if !defined(_MSC_VER) || _MSC_VER >= 1700
#include <stdint.h>
#else
typedef unsigned __int64 uint64_t;
typedef int int32_t;
typedef unsigned char uint8_t;
#endif
#include <stdio.h>
#include <stddef.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define BAIK_ENABLE_DEBUG 1
typedef uint64_t baik_val_t;

#if 0
struct baik {
 
};
#endif

struct baik;

typedef enum baik_err {
  BAIK_OK,
  BAIK_SYNTAX_ERROR,
  BAIK_REFERENCE_ERROR,
  BAIK_TYPE_ERROR,
  BAIK_OUT_OF_MEMORY,
  BAIK_INTERNAL_ERROR,
  BAIK_NOT_IMPLEMENTED_ERROR,
  BAIK_FILE_READ_ERROR,
  BAIK_BAD_ARGS_ERROR,
  BAIK_ERRS_CNT
} baik_err_t;
struct baik;
struct baik *baik_create();
struct baik_create_opts {
  const struct bf_code *code;
};

struct baik *baik_create_opt(struct baik_create_opts opts);
void baik_destroy(struct baik *baik);
baik_val_t baik_get_global(struct baik *baik);
void baik_own(struct baik *baik, baik_val_t *v);
int baik_disown(struct baik *baik, baik_val_t *v);
baik_err_t baik_set_errorf(struct baik *baik, baik_err_t err, const char *fmt, ...);
baik_err_t baik_prepend_errorf(struct baik *baik, baik_err_t err, const char *fmt,
                             ...);
void baik_print_error(struct baik *baik, FILE *fp, const char *msg,
                     int print_stack_trace);
const char *baik_strerror(struct baik *baik, enum baik_err err);
void baik_set_generate_jsc(struct baik *baik, int generate_jsc);
int baik_nargs(struct baik *baik);
baik_val_t baik_arg(struct baik *baik, int n);
void baik_return(struct baik *baik, baik_val_t v);
#if defined(__cplusplus)
}
#endif

#endif

#ifndef BAIK_ARRAY_PUBLIC_H_
#define BAIK_ARRAY_PUBLIC_H_

#if defined(__cplusplus)
extern "C" {
#endif

baik_val_t baik_mk_array(struct baik *baik);
unsigned long baik_array_length(struct baik *baik, baik_val_t arr);
baik_err_t baik_array_push(struct baik *baik, baik_val_t arr, baik_val_t v);
baik_val_t baik_array_get(struct baik *, baik_val_t arr, unsigned long index);
baik_err_t baik_array_set(struct baik *baik, baik_val_t arr, unsigned long index,
                        baik_val_t v);
int baik_is_array(baik_val_t v);
void baik_array_del(struct baik *baik, baik_val_t arr, unsigned long index);

#if defined(__cplusplus)
}
#endif

#endif

#ifndef BAIK_ARRAY_H_
#define BAIK_ARRAY_H_

#if defined(__cplusplus)
extern "C" {
#endif

BAIK_PRIVATE baik_val_t
baik_array_get2(struct baik *baik, baik_val_t arr, unsigned long index, int *has);
BAIK_PRIVATE void baik_array_splice(struct baik *baik);
BAIK_PRIVATE void baik_array_push_internal(struct baik *baik);

#if defined(__cplusplus)
}
#endif

#endif

#ifndef BAIK_FFI_PUBLIC_H_
#define BAIK_FFI_PUBLIC_H_

#if defined(__cplusplus)
extern "C" {
#endif

enum baik_ffi_ctype {
  BAIK_FFI_CTYPE_NONE,
  BAIK_FFI_CTYPE_USERDATA,
  BAIK_FFI_CTYPE_CALLBACK,
  BAIK_FFI_CTYPE_INT,
  BAIK_FFI_CTYPE_BOOL,
  BAIK_FFI_CTYPE_DOUBLE,
  BAIK_FFI_CTYPE_FLOAT,
  BAIK_FFI_CTYPE_CHAR_PTR,
  BAIK_FFI_CTYPE_VOID_PTR,
  BAIK_FFI_CTYPE_STRUCT_GENERIC_STR_PTR,
  BAIK_FFI_CTYPE_STRUCT_GENERIC_STR,
  BAIK_FFI_CTYPE_INVALID,
};

typedef void *(baik_ffi_resolver_t)(void *handle, const char *symbol);
void baik_set_ffi_resolver(struct baik *baik, baik_ffi_resolver_t *dlsym);

#if defined(__cplusplus)
}
#endif

#ifndef BAIK_FFI_H_
#define BAIK_FFI_H_

#if defined(__cplusplus)
extern "C" {
#endif

baik_ffi_resolver_t dlsym;

#define BAIK_CB_ARGS_MAX_CNT 6
#define BAIK_CB_SIGNATURE_MAX_SIZE (BAIK_CB_ARGS_MAX_CNT + 1)

typedef uint8_t baik_ffi_ctype_t;

enum ffi_sig_type {
  FFI_SIG_FUNC,
  FFI_SIG_CALLBACK,
};

struct baik_ffi_sig {
  struct baik_ffi_sig *cb_sig;
  baik_ffi_ctype_t val_types[BAIK_CB_SIGNATURE_MAX_SIZE];
  ffi_fn_t *fn;
  int8_t args_cnt;
  unsigned is_callback : 1;
  unsigned is_valid : 1;
};
typedef struct baik_ffi_sig baik_ffi_sig_t;

BAIK_PRIVATE void baik_ffi_sig_init(baik_ffi_sig_t *sig);
BAIK_PRIVATE void baik_ffi_sig_copy(baik_ffi_sig_t *to, const baik_ffi_sig_t *from);
BAIK_PRIVATE void baik_ffi_sig_free(baik_ffi_sig_t *sig);
BAIK_PRIVATE baik_val_t baik_mk_ffi_sig(struct baik *baik);
BAIK_PRIVATE int baik_is_ffi_sig(baik_val_t v);
BAIK_PRIVATE baik_val_t baik_ffi_sig_to_value(struct baik_ffi_sig *psig);
BAIK_PRIVATE struct baik_ffi_sig *baik_get_ffi_sig_struct(baik_val_t v);
BAIK_PRIVATE void baik_ffi_sig_destructor(struct baik *baik, void *psig);
BAIK_PRIVATE int baik_ffi_sig_set_val_type(baik_ffi_sig_t *sig, int idx,
                                         baik_ffi_ctype_t type);
BAIK_PRIVATE int baik_ffi_sig_validate(struct baik *baik, baik_ffi_sig_t *sig,
                                     enum ffi_sig_type sig_type);
BAIK_PRIVATE int baik_ffi_is_regular_word(baik_ffi_ctype_t type);
BAIK_PRIVATE int baik_ffi_is_regular_word_or_void(baik_ffi_ctype_t type);

struct baik_ffi_cb_args {
  struct baik_ffi_cb_args *next;
  struct baik *baik;
  baik_ffi_sig_t sig;
  baik_val_t func;
  baik_val_t userdata;
};
typedef struct baik_ffi_cb_args ffi_cb_args_t;
BAIK_PRIVATE baik_err_t baik_ffi_call(struct baik *baik);
BAIK_PRIVATE baik_err_t baik_ffi_call2(struct baik *baik);
BAIK_PRIVATE void baik_ffi_cb_free(struct baik *);
BAIK_PRIVATE void baik_ffi_args_free_list(struct baik *baik);

#if defined(__cplusplus)
}
#endif

#endif
#endif

#ifndef BAIK_MM_H_
#define BAIK_MM_H_

#if defined(__cplusplus)
extern "C" {
#endif

struct baik;

typedef void (*gc_cell_destructor_t)(struct baik *baik, void *);

struct gc_block {
  struct gc_block *next;
  struct gc_cell *base;
  size_t size;
};

struct gc_arena {
  struct gc_block *blocks;
  size_t size_increment;
  struct gc_cell *free;
  size_t cell_size;

#if BAIK_MEMORY_STATS
  unsigned long allocations;
  unsigned long garbage;    
  unsigned long alive;      
#endif

  gc_cell_destructor_t destructor;
};

#if defined(__cplusplus)
}
#endif

#endif

#ifndef BAIK_GC_H_
#define BAIK_GC_H_

#if defined(__cplusplus)
extern "C" {
#endif

#define GC_CELL_OP(arena, cell, op, arg) \
  ((struct gc_cell *) (((char *) (cell)) op((arg) * (arena)->cell_size)))

struct gc_cell {
  union {
    struct gc_cell *link;
    uintptr_t word;
  } head;
};

void baik_gc(struct baik *baik, int full);
BAIK_PRIVATE int gc_strings_is_gc_needed(struct baik *baik);
BAIK_PRIVATE int maybe_gc(struct baik *baik);
BAIK_PRIVATE struct baik_object *new_object(struct baik *);
BAIK_PRIVATE struct baik_property *new_property(struct baik *);
BAIK_PRIVATE struct baik_ffi_sig *new_ffi_sig(struct baik *baik);
BAIK_PRIVATE void gc_mark(struct baik *baik, baik_val_t *val);
BAIK_PRIVATE void gc_arena_init(struct gc_arena *, size_t, size_t, size_t);
BAIK_PRIVATE void gc_arena_destroy(struct baik *, struct gc_arena *a);
BAIK_PRIVATE void gc_sweep(struct baik *, struct gc_arena *, size_t);
BAIK_PRIVATE void *gc_alloc_cell(struct baik *, struct gc_arena *);
BAIK_PRIVATE uint64_t gc_string_baik_val_to_offset(baik_val_t v);
BAIK_PRIVATE int gc_check_val(struct baik *baik, baik_val_t v);
BAIK_PRIVATE int gc_check_ptr(const struct gc_arena *a, const void *p);

#if defined(__cplusplus)
}
#endif

#endif

#ifndef BAIK_CORE_H
#define BAIK_CORE_H

#if defined(__cplusplus)
extern "C" {
#endif

#define JUMP_INSTRUCTION_SIZE 2

enum baik_type {
  BAIK_TYPE_UNDEFINED,
  BAIK_TYPE_NULL,
  BAIK_TYPE_BOOLEAN,
  BAIK_TYPE_NUMBER,
  BAIK_TYPE_STRING,
  BAIK_TYPE_FOREIGN,
  BAIK_TYPE_OBJECT_GENERIC,
  BAIK_TYPE_OBJECT_ARRAY,
  BAIK_TYPE_OBJECT_FUNCTION,
  BAIK_TYPES_CNT
};

enum baik_call_stack_frame_item {
  CALL_STACK_FRAME_ITEM_RETVAL_STACK_IDX,
  CALL_STACK_FRAME_ITEM_LOOP_ADDR_IDX,
  CALL_STACK_FRAME_ITEM_SCOPE_IDX,
  CALL_STACK_FRAME_ITEM_RETURN_ADDR,
  CALL_STACK_FRAME_ITEM_THIS,
  CALL_STACK_FRAME_ITEMS_CNT
};


#define MAKE_TAG(s, t) \
  ((uint64_t)(s) << 63 | (uint64_t) 0x7ff0 << 48 | (uint64_t)(t) << 48)

#define BAIK_TAG_OBJECT MAKE_TAG(1, 1)
#define BAIK_TAG_FOREIGN MAKE_TAG(1, 2)
#define BAIK_TAG_UNDEFINED MAKE_TAG(1, 3)
#define BAIK_TAG_BOOLEAN MAKE_TAG(1, 4)
#define BAIK_TAG_NAN MAKE_TAG(1, 5)
#define BAIK_TAG_STRING_I MAKE_TAG(1, 6) 
#define BAIK_TAG_STRING_5 MAKE_TAG(1, 7) 
#define BAIK_TAG_STRING_O MAKE_TAG(1, 8) 
#define BAIK_TAG_STRING_F MAKE_TAG(1, 9) 
#define BAIK_TAG_STRING_C MAKE_TAG(1, 10)
#define BAIK_TAG_STRING_D MAKE_TAG(1, 11)
#define BAIK_TAG_ARRAY MAKE_TAG(1, 12)
#define BAIK_TAG_FUNCTION MAKE_TAG(1, 13)
#define BAIK_TAG_FUNCTION_FFI MAKE_TAG(1, 14)
#define BAIK_TAG_NULL MAKE_TAG(1, 15)

#define BAIK_TAG_MASK MAKE_TAG(1, 15)

struct baik_vals {
  baik_val_t this_obj;
  baik_val_t dataview_proto;
  baik_val_t last_getprop_obj;
};

struct baik_bcode_part {
 
  size_t start_idx;

  struct {
    const char *p;
    size_t len;   
  } data;

  baik_err_t exec_res : 4;
  unsigned in_rom : 1;
};

struct baik {
  struct mbuf bcode_gen;
  struct mbuf bcode_parts;
  size_t bcode_len;
  struct mbuf stack;
  struct mbuf call_stack;
  struct mbuf arg_stack;
  struct mbuf scopes;         
  struct mbuf loop_addresses; 
  struct mbuf owned_strings;  
  struct mbuf foreign_strings;
  struct mbuf owned_values;
  struct mbuf json_visited_stack;
  struct baik_vals vals;
  char *error_msg;
  char *stack_trace;
  enum baik_err error;
  // baik_ffi_resolver_t *dlsym; 
  // ffi_cb_args_t *ffi_cb_args;
  size_t cur_bcode_offset;

  struct gc_arena object_arena;
  struct gc_arena property_arena;
  struct gc_arena ffi_sig_arena;

  unsigned inhibit_gc : 1;
  unsigned need_gc : 1;
  unsigned generate_jsc : 1;
};


typedef uint32_t baik_header_item_t;
enum baik_header_items {
  BAIK_HDR_ITEM_TOTAL_SIZE,  
  BAIK_HDR_ITEM_BCODE_OFFSET,
  BAIK_HDR_ITEM_MAP_OFFSET,  
  BAIK_HDR_ITEMS_CNT
};

BAIK_PRIVATE size_t baik_get_func_addr(baik_val_t v);
BAIK_PRIVATE int baik_getretvalpos(struct baik *baik);
BAIK_PRIVATE enum baik_type baik_get_type(baik_val_t v);
BAIK_PRIVATE void baik_gen_stack_trace(struct baik *baik, size_t offset);
BAIK_PRIVATE baik_val_t vtop(struct mbuf *m);
BAIK_PRIVATE size_t baik_stack_size(const struct mbuf *m);
BAIK_PRIVATE baik_val_t *vptr(struct mbuf *m, int idx);
BAIK_PRIVATE void push_baik_val(struct mbuf *m, baik_val_t v);
BAIK_PRIVATE baik_val_t baik_pop_val(struct mbuf *m);
BAIK_PRIVATE baik_val_t baik_pop(struct baik *baik);
BAIK_PRIVATE void baik_push(struct baik *baik, baik_val_t v);
BAIK_PRIVATE void baik_die(struct baik *baik);

#if defined(__cplusplus)
}
#endif

#endif

#ifndef BAIK_CONVERSION_H_
#define BAIK_CONVERSION_H_

#if defined(__cplusplus)
extern "C" {
#endif

BAIK_PRIVATE baik_err_t baik_to_string(struct baik *baik, baik_val_t *v, char **p,
                                    size_t *sizep, int *need_free);
BAIK_PRIVATE baik_val_t baik_to_boolean_v(struct baik *baik, baik_val_t v);
BAIK_PRIVATE int baik_is_truthy(struct baik *baik, baik_val_t v);

#if defined(__cplusplus)
}
#endif

#endif

#ifndef BAIK_OBJECT_PUBLIC_H_
#define BAIK_OBJECT_PUBLIC_H_

#include <stddef.h>

#if defined(__cplusplus)
extern "C" {
#endif

int baik_is_object(baik_val_t v);
baik_val_t baik_mk_object(struct baik *baik);

enum baik_struct_field_type {
  BAIK_STRUCT_FIELD_TYPE_INVALID,
  BAIK_STRUCT_FIELD_TYPE_STRUCT,    
  BAIK_STRUCT_FIELD_TYPE_STRUCT_PTR,
  BAIK_STRUCT_FIELD_TYPE_INT,
  BAIK_STRUCT_FIELD_TYPE_BOOL,
  BAIK_STRUCT_FIELD_TYPE_DOUBLE,
  BAIK_STRUCT_FIELD_TYPE_FLOAT,
  BAIK_STRUCT_FIELD_TYPE_CHAR_PTR,  
  BAIK_STRUCT_FIELD_TYPE_VOID_PTR,  
  BAIK_STRUCT_FIELD_TYPE_GENERIC_STR_PTR,
  BAIK_STRUCT_FIELD_TYPE_GENERIC_STR,    
  BAIK_STRUCT_FIELD_TYPE_DATA,      
  BAIK_STRUCT_FIELD_TYPE_INT8,
  BAIK_STRUCT_FIELD_TYPE_INT16,
  BAIK_STRUCT_FIELD_TYPE_UINT8,
  BAIK_STRUCT_FIELD_TYPE_UINT16,
  BAIK_STRUCT_FIELD_TYPE_CUSTOM,
};

struct baik_c_struct_member {
  const char *name;
  int offset;
  enum baik_struct_field_type type;
  const void *arg;
};

baik_val_t baik_struct_to_obj(struct baik *baik, const void *base,
                            const struct baik_c_struct_member *members);
baik_val_t baik_get(struct baik *baik, baik_val_t obj, const char *name,
                  size_t name_len);
baik_val_t baik_get_v(struct baik *baik, baik_val_t obj, baik_val_t name);
baik_val_t baik_get_v_proto(struct baik *baik, baik_val_t obj, baik_val_t key);
baik_err_t baik_set(struct baik *baik, baik_val_t obj, const char *name, size_t len,
                  baik_val_t val);
baik_err_t baik_set_v(struct baik *baik, baik_val_t obj, baik_val_t name,
                    baik_val_t val);
int baik_del(struct baik *baik, baik_val_t obj, const char *name, size_t len);
baik_val_t baik_next(struct baik *baik, baik_val_t obj, baik_val_t *iterator);

#if defined(__cplusplus)
}
#endif

#endif

#ifndef BAIK_OBJECT_H_
#define BAIK_OBJECT_H_

#if defined(__cplusplus)
extern "C" {
#endif

struct baik;
struct baik_property {
  struct baik_property *next;
  baik_val_t name;           
  baik_val_t value;          
};

struct baik_object {
  struct baik_property *properties;
};

BAIK_PRIVATE struct baik_object *get_object_struct(baik_val_t v);
BAIK_PRIVATE struct baik_property *baik_get_own_property(struct baik *baik,
                                                      baik_val_t obj,
                                                      const char *name,
                                                      size_t len);
BAIK_PRIVATE struct baik_property *baik_get_own_property_v(struct baik *baik,
                                                        baik_val_t obj,
                                                        baik_val_t key);
BAIK_PRIVATE baik_err_t baik_set_internal(struct baik *baik, baik_val_t obj,
                                       baik_val_t name_v, char *name,
                                       size_t name_len, baik_val_t val);
BAIK_PRIVATE void baik_op_create_object(struct baik *baik);

#define BAIK_PROTO_PROP_NAME "__p"

#if defined(__cplusplus)
}
#endif

#endif

#ifndef BAIK_PRIMITIVE_PUBLIC_H_
#define BAIK_PRIMITIVE_PUBLIC_H_

#if defined(__cplusplus)
extern "C" {
#endif

#define BAIK_NULL BAIK_TAG_NULL
#define BAIK_UNDEFINED BAIK_TAG_UNDEFINED

typedef void (*baik_func_ptr_t)(void);
baik_val_t baik_mk_null(void);
int baik_is_null(baik_val_t v);
baik_val_t baik_mk_undefined(void);
int baik_is_undefined(baik_val_t v);
baik_val_t baik_mk_number(struct baik *baik, double num);
double baik_get_double(struct baik *baik, baik_val_t v);
int baik_get_int(struct baik *baik, baik_val_t v);
int32_t baik_get_int32(struct baik *baik, baik_val_t v);
int baik_is_number(baik_val_t v);
baik_val_t baik_mk_foreign(struct baik *baik, void *ptr);
baik_val_t baik_mk_foreign_func(struct baik *baik, baik_func_ptr_t fn);
void *baik_get_ptr(struct baik *baik, baik_val_t v);
int baik_is_foreign(baik_val_t v);
baik_val_t baik_mk_boolean(struct baik *baik, int v);
int baik_get_bool(struct baik *baik, baik_val_t v);
int baik_is_boolean(baik_val_t v);

baik_val_t baik_mk_function(struct baik *baik, size_t off);
int baik_is_function(baik_val_t v);

#if defined(__cplusplus)
}
#endif

#endif

#ifndef BAIK_PRIMITIVE_H
#define BAIK_PRIMITIVE_H

#if defined(__cplusplus)
extern "C" {
#endif

BAIK_PRIVATE baik_val_t baik_legit_pointer_to_value(void *p);
BAIK_PRIVATE baik_val_t baik_pointer_to_value(struct baik *baik, void *p);
BAIK_PRIVATE void *get_ptr(baik_val_t v);
BAIK_PRIVATE void baik_op_isnan(struct baik *baik);

#if defined(__cplusplus)
}
#endif

#endif

#ifndef BAIK_STRING_PUBLIC_H_
#define BAIK_STRING_PUBLIC_H_
#define BAIK_STRING_LITERAL_MAX_LEN 128

#if defined(__cplusplus)
extern "C" {
#endif

baik_val_t baik_mk_string(struct baik *baik, const char *str, size_t len, int copy);
int baik_is_string(baik_val_t v);
const char *baik_get_string(struct baik *baik, baik_val_t *v, size_t *len);
const char *baik_get_cstring(struct baik *baik, baik_val_t *v);
int baik_strcmp(struct baik *baik, baik_val_t *a, const char *b, size_t len);

#if defined(__cplusplus)
}
#endif

#endif

#ifndef BAIK_STRING_H_
#define BAIK_STRING_H_

#if defined(__cplusplus)
extern "C" {
#endif
#define _BAIK_STRING_BUF_RESERVE 100

BAIK_PRIVATE unsigned long cstr_to_ulong(const char *s, size_t len, int *ok);
BAIK_PRIVATE baik_err_t
str_to_ulong(struct baik *baik, baik_val_t v, int *ok, unsigned long *res);
BAIK_PRIVATE int s_cmp(struct baik *baik, baik_val_t a, baik_val_t b);
BAIK_PRIVATE baik_val_t s_concat(struct baik *baik, baik_val_t a, baik_val_t b);
BAIK_PRIVATE void embed_string(struct mbuf *m, size_t offset, const char *p,
                              size_t len, uint8_t flags);
BAIK_PRIVATE void baik_mkstr(struct baik *baik);
BAIK_PRIVATE void baik_string_slice(struct baik *baik);
BAIK_PRIVATE void baik_string_index_of(struct baik *baik);
BAIK_PRIVATE void baik_string_char_code_at(struct baik *baik);
#define EMBSTR_ZERO_TERM 1
#define EMBSTR_UNESCAPE 2

#if defined(__cplusplus)
}
#endif

#endif

#ifndef BAIK_UTIL_PUBLIC_H_
#define BAIK_UTIL_PUBLIC_H_


#include <stdio.h>

#if defined(__cplusplus)
extern "C" {
#endif

const char *baik_typeof(baik_val_t v);
void baik_fprintf(baik_val_t v, struct baik *baik, FILE *fp);
void baik_sprintf(baik_val_t v, struct baik *baik, char *buf, size_t buflen);

#if BAIK_ENABLE_DEBUG
void baik_disasm(const uint8_t *code, size_t len);
void baik_dump(struct baik *baik, int do_disasm);
#endif


const char *baik_get_bcode_filename_by_offset(struct baik *baik, int offset);
int baik_get_lineno_by_offset(struct baik *baik, int offset);


int baik_get_offset_by_call_frame_num(struct baik *baik, int cf_num);

#if defined(__cplusplus)
}
#endif

#endif

#ifndef BAIK_UTIL_H_
#define BAIK_UTIL_H_

#if defined(__cplusplus)
extern "C" {
#endif

struct baik_bcode_part;

BAIK_PRIVATE const char *opcodetostr(uint8_t opcode);
BAIK_PRIVATE size_t baik_disasm_single(const uint8_t *code, size_t i);
BAIK_PRIVATE const char *baik_stringify_type(enum baik_type t);
BAIK_PRIVATE int baik_check_arg(struct baik *baik, int arg_num,
                              const char *arg_name, enum baik_type expected_type,
                              baik_val_t *parg);
BAIK_PRIVATE int baik_normalize_idx(int idx, int size);
BAIK_PRIVATE const char *baik_get_bcode_filename(struct baik *baik,
                                               struct baik_bcode_part *bp);
void baik_jprintf(baik_val_t v, struct baik *baik, struct json_out *out);

#if defined(__cplusplus)
}
#endif

#endif

#ifndef BAIK_EM_COMMON_BAIK_EM_VARINT_H_
#define BAIK_EM_COMMON_BAIK_EM_VARINT_H_

#if defined(_WIN32) && _MSC_VER < 1700
typedef unsigned char uint8_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


size_t BAIK_EM_varint_llen(uint64_t num);
size_t BAIK_EM_varint_encode(uint64_t num, uint8_t *buf, size_t buf_size);
bool BAIK_EM_varint_decode(const uint8_t *buf, size_t buf_size, uint64_t *num,
                      size_t *llen);
uint64_t BAIK_EM_varint_decode_unsafe(const uint8_t *buf, int *llen);

#ifdef __cplusplus
}
#endif

#endif

#ifndef BAIK_BCODE_H_
#define BAIK_BCODE_H_

#if defined(__cplusplus)
extern "C" {
#endif

enum baik_opcode {
  OP_NOP,              
  OP_DROP,             
  OP_DUP,              
  OP_SWAP,             
  OP_JMP,              
  OP_JMP_TRUE,         
  OP_JMP_NEUTRAL_TRUE, 
  OP_JMP_FALSE,        
  OP_JMP_NEUTRAL_FALSE,
  OP_FIND_SCOPE,       
  OP_PUSH_SCOPE,       
  OP_PUSH_STR,         
  OP_PUSH_TRUE,        
  OP_PUSH_FALSE,       
  OP_PUSH_INT,         
  OP_PUSH_DBL,         
  OP_PUSH_NULL,        
  OP_PUSH_UNDEF,       
  OP_PUSH_OBJ,         
  OP_PUSH_ARRAY,       
  OP_PUSH_FUNC,        
  OP_PUSH_THIS,        
  OP_GET,              
  OP_CREATE,           
  OP_EXPR,             
  OP_APPEND,           
  OP_SET_ARG,          
  OP_NEW_SCOPE,        
  OP_DEL_SCOPE,        
  OP_CALL,             
  OP_RETURN,           
  OP_LOOP,        
  OP_BREAK,       
  OP_CONTINUE,    
  OP_SETRETVAL,   
  OP_EXIT,        
  OP_BCODE_HEADER,
  OP_ARGS,        
  OP_FOR_IN_NEXT, 
  OP_MAX
};

struct pstate;
struct baik;

BAIK_PRIVATE void emit_byte(struct pstate *pstate, uint8_t byte);
BAIK_PRIVATE void emit_int(struct pstate *pstate, int64_t n);
BAIK_PRIVATE void emit_str(struct pstate *pstate, const char *ptr, size_t len);
BAIK_PRIVATE int baik_bcode_insert_offset(struct pstate *p, struct baik *baik,
                                        size_t offset, size_t v);
BAIK_PRIVATE void baik_bcode_part_add(struct baik *baik,
                                    const struct baik_bcode_part *bp);
BAIK_PRIVATE struct baik_bcode_part *baik_bcode_part_get(struct baik *baik, int num);
BAIK_PRIVATE struct baik_bcode_part *baik_bcode_part_get_by_offset(struct baik *baik,
                                                                size_t offset);
BAIK_PRIVATE int baik_bcode_parts_cnt(struct baik *baik);
BAIK_PRIVATE void baik_bcode_commit(struct baik *baik);

#if defined(__cplusplus)
}
#endif

#endif

#ifndef BAIK_INTERNAL_H_
#define BAIK_INTERNAL_H_

#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifndef FAST
#define FAST
#endif

#ifndef STATIC
#define STATIC
#endif

#ifndef ENDL
#define ENDL "\n"
#endif

#ifdef BAIK_EXPOSE_PRIVATE
#define BAIK_PRIVATE
#define BAIK_EXTERN extern
#else
#define BAIK_PRIVATE static
#define BAIK_EXTERN static
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#if !defined(WEAK)
#if (defined(__GNUC__) || defined(__TI_COMPILER_VERSION__)) && !defined(_WIN32)
#define WEAK __attribute__((weak))
#else
#define WEAK
#endif
#endif

#ifndef BAIK_EM_ENABLE_STDIO
#define BAIK_EM_ENABLE_STDIO 1
#endif

#if defined(_WIN32) && _MSC_VER < 1700
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef __int64 int64_t;
typedef unsigned long uintptr_t;
#define STRX(x) #x
#define STR(x) STRX(x)
#define __func__ __FILE__ ":" STR(__LINE__)

#define vsnprintf _vsnprintf
#define isnan(x) _isnan(x)
#define va_copy(x, y) (x) = (y)
#define BAIK_EM_DEFINE_DIRENT
#include <windows.h>
#else
#if defined(__unix__) || defined(__APPLE__)
#include <dlfcn.h>
#endif
#endif

#ifndef BAIK_INIT_OFFSET_SIZE
#define BAIK_INIT_OFFSET_SIZE 1
#endif

#endif

#ifndef BAIK_TOK_H_
#define BAIK_TOK_H_

#if defined(__cplusplus)
extern "C" {
#endif

struct tok {
  int tok;
  int len;
  const char *ptr;
};

struct pstate {
  const char *file_name;
  const char *buf;      
  const char *pos;      
  int line_no;          
  int last_emitted_line_no;
  struct mbuf offset_lineno_map;
  int prev_tok;  
  struct tok tok;
  struct baik *baik;
  int start_bcode_idx;
  int cur_idx;
  int depth;
};

enum {
  TOK_EOF,
  TOK_INVALID,

  TOK_COLON,
  TOK_SEMICOLON,
  TOK_COMMA,
  TOK_ASSIGN,
  TOK_OPEN_CURLY,
  TOK_CLOSE_CURLY,
  TOK_OPEN_PAREN,
  TOK_CLOSE_PAREN,
  TOK_OPEN_BRACKET,
  TOK_CLOSE_BRACKET,
  TOK_MUL,
  TOK_PLUS,
  TOK_MINUS,
  TOK_DIV,
  TOK_REM,
  TOK_AND,
  TOK_OR,
  TOK_XOR,
  TOK_DOT,
  TOK_QUESTION,
  TOK_NOT,
  TOK_TILDA,
  TOK_LT,
  TOK_GT,
  TOK_LSHIFT,
  TOK_RSHIFT,
  TOK_MINUS_MINUS,
  TOK_PLUS_PLUS,
  TOK_PLUS_ASSIGN,
  TOK_MINUS_ASSIGN,
  TOK_MUL_ASSIGN,
  TOK_DIV_ASSIGN,
  TOK_AND_ASSIGN,
  TOK_OR_ASSIGN,
  TOK_REM_ASSIGN,
  TOK_XOR_ASSIGN,
  TOK_EQ,
  TOK_NE,
  TOK_LE,
  TOK_GE,
  TOK_LOGICAL_AND,
  TOK_LOGICAL_OR,
  TOK_EQ_EQ,
  TOK_NE_NE,
  TOK_LSHIFT_ASSIGN,
  TOK_RSHIFT_ASSIGN,
  TOK_URSHIFT,
  TOK_URSHIFT_ASSIGN,

  TOK_UNARY_PLUS,
  TOK_UNARY_MINUS,
  TOK_POSTFIX_PLUS,
  TOK_POSTFIX_MINUS,

  TOK_NUM = 200,
  TOK_STR,
  TOK_IDENT,
  TOK_KEYWORD_BERHENTI,
  TOK_KEYWORD_SAMA,
  TOK_KEYWORD_CATCH,
  TOK_KEYWORD_TERUSKAN,
  TOK_KEYWORD_DEBUGGER,
  TOK_KEYWORD_STANDAR,
  TOK_KEYWORD_DELETE,
  TOK_KEYWORD_KERJAKAN,
  TOK_KEYWORD_LAINNYA,
  TOK_KEYWORD_FALSE,
  TOK_KEYWORD_FINALLY,
  TOK_KEYWORD_UNTUK,
  TOK_KEYWORD_FUNGSI,
  TOK_KEYWORD_JIKA,
  TOK_KEYWORD_IN,
  TOK_KEYWORD_INSTANCEOF,
  TOK_KEYWORD_NEW,
  TOK_KEYWORD_KOSONG,
  TOK_KEYWORD_BALIK,
  TOK_KEYWORD_PILIH,
  TOK_KEYWORD_THIS,
  TOK_KEYWORD_THROW,
  TOK_KEYWORD_TRUE,
  TOK_KEYWORD_TRY,
  TOK_KEYWORD_TIPE,
  TOK_KEYWORD_VAR,
  TOK_KEYWORD_VOID,
  TOK_KEYWORD_ULANG,
  TOK_KEYWORD_WITH,
  TOK_KEYWORD_ISI,
  TOK_KEYWORD_TAKTERDEFINISI,
  TOK_MAX
};

BAIK_PRIVATE void pinit(const char *file_name, const char *buf, struct pstate *);
BAIK_PRIVATE int pnext(struct pstate *);
BAIK_PRIVATE int baik_is_ident(int c);
BAIK_PRIVATE int baik_is_digit(int c);

#if defined(__cplusplus)
}
#endif

#endif

#ifndef BAIK_DATAVIEW_H_
#define BAIK_DATAVIEW_H_

#if defined(__cplusplus)
extern "C" {
#endif

void *baik_mem_to_ptr(unsigned int val);
void *baik_mem_get_ptr(void *base, int offset);
void baik_mem_set_ptr(void *ptr, void *val);
double baik_mem_get_dbl(void *ptr);
void baik_mem_set_dbl(void *ptr, double val);
double baik_mem_get_uint(void *ptr, int size, int bigendian);
double baik_mem_get_int(void *ptr, int size, int bigendian);
void baik_mem_set_uint(void *ptr, unsigned int val, int size, int bigendian);
void baik_mem_set_int(void *ptr, int val, int size, int bigendian);

#if defined(__cplusplus)
}
#endif

#endif

#ifndef BAIK_EXEC_PUBLIC_H_
#define BAIK_EXEC_PUBLIC_H_


#include <stdio.h>

#if defined(__cplusplus)
extern "C" {
#endif
baik_err_t baik_exec(struct baik *, const char *src, baik_val_t *res);
baik_err_t baik_exec_buf(struct baik *, const char *src, size_t, baik_val_t *res);
baik_err_t baik_exec_file(struct baik *baik, const char *path, baik_val_t *res);
baik_err_t baik_apply(struct baik *baik, baik_val_t *res, baik_val_t func,
                    baik_val_t this_val, int nargs, baik_val_t *args);
baik_err_t baik_call(struct baik *baik, baik_val_t *res, baik_val_t func,
                   baik_val_t this_val, int nargs, ...);
baik_val_t baik_get_this(struct baik *baik);

#if defined(__cplusplus)
}
#endif

#endif

#ifndef BAIK_EXEC_H_
#define BAIK_EXEC_H_
#define BAIK_BCODE_OFFSET_EXIT ((size_t) 0x7fffffff)

#if defined(__cplusplus)
extern "C" {
#endif

BAIK_PRIVATE baik_err_t baik_execute(struct baik *baik, size_t off, baik_val_t *res);

#if defined(__cplusplus)
}
#endif

#endif

#ifndef BAIK_JSON_H_
#define BAIK_JSON_H_

#if defined(__cplusplus)
extern "C" {
#endif

BAIK_PRIVATE baik_err_t to_json_or_debug(struct baik *baik, baik_val_t v, char *buf,
                                       size_t size, size_t *res_len,
                                       uint8_t is_debug);
BAIK_PRIVATE baik_err_t baik_json_stringify(struct baik *baik, baik_val_t v,
                                         char *buf, size_t size, char **res);
BAIK_PRIVATE void baik_op_json_stringify(struct baik *baik);
BAIK_PRIVATE void baik_op_json_parse(struct baik *baik);
BAIK_PRIVATE baik_err_t
baik_json_parse(struct baik *baik, const char *str, size_t len, baik_val_t *res);

#if defined(__cplusplus)
}
#endif

#endif

#ifndef BAIK_BUILTIN_H_
#define BAIK_BUILTIN_H_

#if defined(__cplusplus)
extern "C" {
#endif

void baik_init_builtin(struct baik *baik, baik_val_t obj);

#if defined(__cplusplus)
}
#endif

#endif

#ifndef BAIK_PARSER_H
#define BAIK_PARSER_H

#if defined(__cplusplus)
extern "C" {
#endif

BAIK_PRIVATE baik_err_t
baik_parse(const char *path, const char *buf, struct baik *);

#if defined(__cplusplus)
}
#endif

#endif

/* ---------------------------------------------------------------------------
 * Deklarasi lintas modul.
 *
 * Ketiga fungsi ini didefinisikan di baik_core.c dan dipakai oleh baik_exec.c.
 * Ketika interpreter masih satu berkas, keduanya berada di berkas yang sama
 * sehingga `static` sudah cukup; setelah dipecah, keduanya perlu dideklarasikan
 * di sini.
 * ------------------------------------------------------------------------ */
BAIK_PRIVATE void call_stack_push_frame(struct baik *baik, size_t offset,
                                        baik_val_t retval_stack_idx);
BAIK_PRIVATE size_t call_stack_restore_frame(struct baik *baik);
BAIK_PRIVATE baik_val_t baik_find_scope(struct baik *baik, baik_val_t key);


/* ---------------------------------------------------------------------------
 * Definisi bersama antar modul.
 *
 * Ukuran arena dipakai baik_core.c (saat membuat interpreter) sementara
 * definisinya dahulu berada di tengah wilayah konversi; typedef Rune dipakai
 * baik_string.c sementara definisinya menempel di ujung modul primitif.
 * Keduanya dipindah ke sini agar kepemilikannya jelas dan tidak bergantung
 * pada urutan penggabungan berkas.
 * ------------------------------------------------------------------------ */
#ifndef BAIK_OBJECT_ARENA_SIZE
#define BAIK_OBJECT_ARENA_SIZE 20
#endif
#ifndef BAIK_PROPERTY_ARENA_SIZE
#define BAIK_PROPERTY_ARENA_SIZE 20
#endif
#ifndef BAIK_FUNC_FFI_ARENA_SIZE
#define BAIK_FUNC_FFI_ARENA_SIZE 20
#endif
#ifndef BAIK_OBJECT_ARENA_INC_SIZE
#define BAIK_OBJECT_ARENA_INC_SIZE 10
#endif
#ifndef BAIK_PROPERTY_ARENA_INC_SIZE
#define BAIK_PROPERTY_ARENA_INC_SIZE 10
#endif
#ifndef BAIK_FUNC_FFI_ARENA_INC_SIZE
#define BAIK_FUNC_FFI_ARENA_INC_SIZE 10
#endif

/* Satuan karakter untuk penanganan string. */
typedef unsigned short Rune;

#endif /* BAIK_INTERNAL_MODUL_H_ */
