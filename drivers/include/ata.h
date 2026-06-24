/*
 * AcharyaOS - ata.h
 * -----------------
 * Phase 1, Feature 12: raw disk read/write through legacy ATA PIO.
 */

#ifndef ACHARYAOS_ATA_H
#define ACHARYAOS_ATA_H

#include <stdint.h>

#define ATA_SECTOR_SIZE 512

typedef struct {
    uint8_t present;
    uint8_t lba48_supported;
    uint32_t total_sectors;
    char model[41];
} ata_device_info_t;

void ata_init(void);
void ata_get_info(ata_device_info_t *info);
int ata_read_sectors(uint32_t lba, uint8_t count, void *buffer);
int ata_write_sectors(uint32_t lba, uint8_t count, const void *buffer);
int ata_disk_self_test(void);

#endif /* ACHARYAOS_ATA_H */
