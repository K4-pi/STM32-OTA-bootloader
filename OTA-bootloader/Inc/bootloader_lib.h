#ifndef __BOOTLOADER_LIB_H
#define __BOOTLOADER_LIB_H

#include <stdint.h>

/*
 * Flash sectors
 *
 * 0 -> 0x08000000 = K(16)   #BOOTLOADER
 * 1 -> 0x08004000 = K(16)   #BOOTLOADER
 * 2 -> 0x08008000 = K(16)   #BOOTLOADER
 * 3 -> 0x0800C000 = K(16)   #META
 * 4 -> 0x08010000 = K(64)   #BLOCK A
 * 5 -> 0x08020000 = K(128)  #BLOCK A
 * 6 -> 0x08040000 = K(128)  #BLOCK B
 * 7 -> 0x08060000 = K(128)  #BLOCK B
 */

typedef enum {
    BLOCK_A,
    BLOCK_B
} flash_block_t;

#define META_SECTOR 3
#define BLOCK_A_SECTOR 4
#define BLOCK_B_SECTOR 6

#define BANKSN_SECTORS 2

#define K(n) ((n) * 1024U) // KB
#define OTA_MAX_IMAGE_SIZE K(192)

#define CHUNK_SIZE K(1)

#define META_BEGIN 0x800C000
#define META_END   0x08010000 // META_BEGIN + 16KB

#define BLOCK_A_BEGIN 0x08010000
#define BLOCK_A_END   0x08040000

#define BLOCK_B_BEGIN 0x08040000
#define BLOCK_B_END   0x08060000

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t flag;
    uint32_t size;
    uint32_t crc;
} app_header_t;

#endif /* __BOOTLOADER_LIB_H */
