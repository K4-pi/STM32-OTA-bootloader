#include "bootloader_lib.h"

#include "stm32f407xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_def.h"
#include "stm32f4xx_hal_flash.h"
// #include "stm32f4xx_hal_flash.h"
// #include "stm32f4xx_hal_flash_ex.h"

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <stdbool.h>

bool verify_firmware_block(flash_block_t block, uint32_t firmware_size, uint32_t expected_crc);
bool verify_firmware_chunk(uint8_t *data, uint32_t size, uint32_t expected_crc);
HAL_StatusTypeDef write_flash(flash_block_t block, uint8_t *data, uint32_t size);
inline HAL_StatusTypeDef write_header(const app_header_t *header);

/**
 * @brief Read data at given address
 *
 * @param address address to read from
 *
 * @return value at address
 */
static inline uint32_t read_flash_word(uint32_t address) {
    return *(volatile uint32_t*)address;
}

/*
TODO:
    * MQTT server sends firmware by chunks, with CRC checksum of this chunk.
    We calculate CRC checksum and compare it to the send one, if correct we 
    write chunk to a flash region of a memory. After whole firmware send 
    MQTT serwer sends image size and it's final checksum of a whole image,
    we compare it and if no mistach we can start booting to a new firmware.

*/

/**
 * @brief Verify firmware CRC checksum by block
 *
 * @param block which block verify (A/B)
 * @param firmware_size size of a firmware to check
 * @param expected_crc correct CRC checksum
 *
 * @return bool
 */
bool verify_firmware_block(flash_block_t block, uint32_t firmware_size, uint32_t expected_crc) {
    uint32_t address = (block == BLOCK_A) ? BLOCK_A_BEGIN : BLOCK_B_BEGIN;

    CRC->CR = CRC_CR_RESET; // Reset before work

    uint32_t i = 0;
    while (i < firmware_size) { 
        uint32_t value = read_flash_word(address + i);

        for (uint8_t j = 0; j < 4; j++) {
            CRC->DR = (uint8_t)(value & 0xFF);
            value >>= 8U;
        }

        i += 4;
    }

    uint32_t crc_value = CRC->DR;
    CRC->CR = CRC_CR_RESET; // Reset after work

    return (crc_value == expected_crc) ? true : false;
}

/**
 * @brief Verify firmware CRC checksum by 1 KB chunks (chunk size = CHUNK_SIZE)
 *
 * @param data chunk data of a firmware 
 * @param size size of a chunk
 * @param expected_crc correct CRC checksum
 *
 * @return bool
 */
bool verify_firmware_chunk(uint8_t *data, uint32_t size, uint32_t expected_crc) {
    CRC->CR = CRC_CR_RESET;

    for (uint32_t i = 0; i < size; i++) CRC->DR = data[i];

    uint32_t crc_value = CRC->DR;
    CRC->CR = CRC_CR_RESET; // Reset after work

    return (crc_value == expected_crc) ? true : false;
}   

/**
 * @brief Write firmware data to flash memory
 *
 * @param block which block write to (A/B)
 * @param data pointer to a data buffer
 * @param size size of a data buffer
 *
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef write_flash(flash_block_t block, uint8_t *data, uint32_t size) {
    if (size >= OTA_MAX_IMAGE_SIZE) return HAL_ERROR; // Buffer bigger than block

    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef erase_init;
    erase_init.TypeErase    = FLASH_TYPEERASE_SECTORS;
    erase_init.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    erase_init.NbSectors    = BANKSN_SECTORS;

    erase_init.Sector = (block == BLOCK_A) ? BLOCK_A_SECTOR : BLOCK_B_SECTOR;

    // Erasing block before writeing new firmware to it
    uint32_t sector_err;
    if (HAL_FLASHEx_Erase(&erase_init, &sector_err) == HAL_ERROR) {
        HAL_FLASH_Lock();
        return HAL_FLASH_GetError();
    }

    // Writeing to block checking the size
    uint32_t address = (block == BLOCK_A) ? BLOCK_A_BEGIN : BLOCK_B_BEGIN;
    uint32_t i = 0;
    while (i < size && (address + i) - address < OTA_MAX_IMAGE_SIZE) {
        HAL_StatusTypeDef s = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address + i, data[i]);
        i += 4;

        if (s == HAL_ERROR) {
            HAL_FLASH_Lock();
            return HAL_FLASH_GetError();
        }
    }

    HAL_FLASH_Lock();
    return HAL_OK;
}

inline HAL_StatusTypeDef write_header(const app_header_t *header) {
    return write_flash(META_SECTOR, (uint8_t *)header, sizeof *header);
}