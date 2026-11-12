#include "bootloader_lib.h"

#include "stm32f407xx.h"
#include "stm32f4xx_hal_def.h"
#include "stm32f4xx_hal_flash.h"
#include "stm32f4xx_hal_flash_ex.h"

#include <stddef.h>
#include <stdint.h>

uint32_t read_flash(uint32_t address) {
    return *(volatile uint32_t*)address;
}

void write_flash(uint32_t address, uint8_t *data, uint64_t size) {
    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef erase_init;
    erase_init.TypeErase    = FLASH_TYPEERASE_SECTORS;
    erase_init.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    erase_init.Sector       = 0x0;
    erase_init.NbSectors    = 1;

    uint32_t sector_err;
    HAL_FLASHEx_Erase(&erase_init, &sector_err);

    HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, address++, 0xff);

    HAL_FLASH_Unlock();
}
