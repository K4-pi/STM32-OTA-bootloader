#include "bootloader_lib.h"

#include "stm32f407xx.h"
#include "stm32f4xx_hal.h"
// #include "stm32f4xx_hal_flash.h"
// #include "stm32f4xx_hal_flash_ex.h"

#include <stddef.h>
#include <stdint.h>

HAL_StatusTypeDef verify_firmware(flash_block_t block, uint32_t expected_crc);
HAL_StatusTypeDef write_flash(flash_block_t block, uint8_t *data, uint64_t size);

/**
 * @brief Read data at given address
 *
 * @param address -> address to read from
 *
 * @return value at address
 */
static inline uint32_t read_flash_word(uint32_t address) {
    return *(volatile uint32_t*)address;
}

/**
 * @brief Verify firmware CRC checksum
 *
 * @param block        -> which block verify (A/B)
 * @param expected_crc -> correct CRC checksum
 *
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef verify_firmware(flash_block_t block, uint32_t expected_crc) {
    uint32_t address = (block == BLOCK_A) ? BLOCK_A_BEGIN : BLOCK_B_BEGIN;

    CRC->CR = CRC_CR_RESET; // Reset before work

    uint32_t i = 0;
    while (i < OTA_MAX_IMAGE_SIZE) { //TODO: Change size
        uint32_t value = read_flash_word(address + i);

        for (uint8_t j = 0; j < 4; j++) {
            CRC->DR = (uint8_t)(value & 0xFF);
            value >>= 8U;
        }

        i += 4;
    }

    uint32_t crc_value = CRC->DR;
    CRC->CR = CRC_CR_RESET; // Reset after work

    return (crc_value == expected_crc) ? HAL_OK : HAL_ERROR;
}

/**
 * @brief Write data to flash memory
 *
 * @param block -> which block write to (A/B)
 * @param data  -> pointer to a data buffer
 * @param size  -> size of a data buffer
 *
 * @return HAL_StatusTypeDef
 */
HAL_StatusTypeDef write_flash(flash_block_t block, uint8_t *data, uint64_t size) {
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
