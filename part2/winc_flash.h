// ATWINC1500/1510 WiFi module flash memory interface for the Pi Pico
//
// Copyright (c) 2021 Jeremy P Bentham
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __WINC_FLASH_H__
#define __WINC_FLASH_H__

#include <stdint.h>
#include <stdbool.h>
#include "winc_wifi.h"

// Global functions
int8_t spi_flash_enable(int fd, uint8_t enable);
int8_t spi_flash_read(int fd, uint8_t *pu8Buf, uint32_t u32offset, uint32_t u32Sz);
int8_t spi_flash_write(int fd, uint8_t* pu8Buf, uint32_t u32Offset, uint32_t u32Sz);
int8_t spi_flash_erase(int fd, uint32_t u32Offset, uint32_t u32Sz);
uint32_t spi_flash_get_size(int fd);

// Redefine BSP_MIN to MIN
#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef BSP_MIN
#define BSP_MIN(x, y) (MIN(x, y))
#endif

// Dummy M2M_PRINT definition to avoid compilation errors
#ifndef M2M_PRINT
#define M2M_PRINT(...)
#endif

// Dummy REV definition to avoid compilation errors
#ifndef REV
#define REV(x) (x)
#endif

#endif
