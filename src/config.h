#ifndef __CONFIG_H__
#define __CONFIG_H__

/*
 * Size definitions
 */
#define B (sizeof(uint8_t))
#define KB (1024 * B)
#define MB (1024 * KB)
#define GB (1024 * MB)
#define TB (1024 * GB)
#define PB (1024 * TB)
#define EB (1024 * PB)
#define ZB (1024 * EB)
#define YB (1024 * ZB)


constexpr auto DISK_SIZE = 64 * MB + 3 * KB;
constexpr auto BLOCK_SIZE = 1 * KB;

#endif