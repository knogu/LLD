#pragma once

#include "common.h"

void spi_init();
void spi_send_recv(u8 chip_select, u8 *sbuffer, u8 *rbuffer, u32 size);

void spi_send_recv_(unsigned char *sbuffer, unsigned char *rbuffer, unsigned int size);

void spi_send(u8 chip_select, u8 *data, u32 size);
void spi_send_(u8 *data, u32 size);
void spi_recv(u8 chip_select, u8 *data, u32 size);

void spi_chip_select(unsigned char chip_select);
