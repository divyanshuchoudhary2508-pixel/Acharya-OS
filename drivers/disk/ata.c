/*
 * AcharyaOS - ata.c
 * -----------------
 * Primary-master ATA PIO driver. This is the thinest useful raw-disk layer:
 * it can detect a drive, read sectors, write sectors, and expose a tiny
 * identification block for diagnostics.
 *
 * The implementation intentionally uses polling rather than interrupts.
 * Disk interrupts are a fine later improvement, but for early educational
 * storage code polling is easier to debug and fits the single-drive QEMU
 * target well.
 */

#include "ata.h"
#include "port_io.h"
#include "kstring.h"

#define ATA_DATA_PORT        0x1F0
#define ATA_ERROR_PORT       0x1F1
#define ATA_SECTOR_COUNT     0x1F2
#define ATA_LBA_LOW          0x1F3
#define ATA_LBA_MID          0x1F4
#define ATA_LBA_HIGH         0x1F5
#define ATA_DRIVE_HEAD       0x1F6
#define ATA_STATUS_PORT      0x1F7
#define ATA_COMMAND_PORT     0x1F7
#define ATA_CONTROL_PORT     0x3F6

#define ATA_CMD_IDENTIFY     0xEC
#define ATA_CMD_READ_PIO     0x20
#define ATA_CMD_WRITE_PIO    0x30

#define ATA_STATUS_ERR       0x01
#define ATA_STATUS_DRQ       0x08
#define ATA_STATUS_DF        0x20
#define ATA_STATUS_BSY       0x80

#define ATA_DEVICE_MASTER    0
#define ATA_LBA28_MASK       0x0FFFFFFF

static ata_device_info_t ata_info;

static inline void ata_io_wait(void) {
    outb(0x80, 0);
}

static inline void ata_insw(uint16_t port, void *addr, uint32_t word_count) {
    __asm__ volatile (
        "cld\n\t"
        "rep insw"
        : "+D"(addr), "+c"(word_count)
        : "d"(port)
        : "memory"
    );
}

static inline void ata_outsw(uint16_t port, const void *addr, uint32_t word_count) {
    __asm__ volatile (
        "cld\n\t"
        "rep outsw"
        : "+S"(addr), "+c"(word_count)
        : "d"(port)
        : "memory"
    );
}

static int ata_wait_not_busy(uint32_t timeout) {
    while (timeout-- > 0) {
        uint8_t status = inb(ATA_STATUS_PORT);
        if ((status & ATA_STATUS_BSY) == 0) {
            if (status & (ATA_STATUS_ERR | ATA_STATUS_DF)) {
                return -1;
            }
            return 0;
        }
        ata_io_wait();
    }
    return -1;
}

static int ata_wait_drq(uint32_t timeout) {
    while (timeout-- > 0) {
        uint8_t status = inb(ATA_STATUS_PORT);
        if (status & (ATA_STATUS_ERR | ATA_STATUS_DF)) {
            return -1;
        }
        if (status & ATA_STATUS_DRQ) {
            return 0;
        }
        ata_io_wait();
    }
    return -1;
}

static void ata_select_lba(uint32_t lba, uint8_t count, uint8_t command) {
    outb(ATA_DRIVE_HEAD, (uint8_t)(0xE0 | ATA_DEVICE_MASTER | ((lba >> 24) & 0x0F)));
    outb(ATA_SECTOR_COUNT, count);
    outb(ATA_LBA_LOW, (uint8_t)(lba & 0xFF));
    outb(ATA_LBA_MID, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));
    outb(ATA_COMMAND_PORT, command);
}

static void ata_decode_string(const uint16_t *src, size_t word_count, char *dest, size_t dest_len) {
    size_t out = 0;
    for (size_t i = 0; i < word_count && out + 1 < dest_len; i++) {
        uint16_t word = src[i];
        char first = (char)(word >> 8);
        char second = (char)(word & 0xFF);
        if (first != ' ' && out + 1 < dest_len) {
            dest[out++] = first;
        }
        if (second != ' ' && out + 1 < dest_len) {
            dest[out++] = second;
        }
    }
    dest[out] = '\0';
}

static int ata_identify(void) {
    uint16_t identify_data[256];

    outb(ATA_DRIVE_HEAD, 0xE0);
    outb(ATA_SECTOR_COUNT, 0);
    outb(ATA_LBA_LOW, 0);
    outb(ATA_LBA_MID, 0);
    outb(ATA_LBA_HIGH, 0);
    outb(ATA_COMMAND_PORT, ATA_CMD_IDENTIFY);
    ata_io_wait();

    uint8_t status = inb(ATA_STATUS_PORT);
    if (status == 0) {
        return 0;
    }

    if (ata_wait_not_busy(100000) != 0) {
        return 0;
    }
    if (ata_wait_drq(100000) != 0) {
        return 0;
    }

    ata_insw(ATA_DATA_PORT, identify_data, 256);

    ata_info.present = 1;
    ata_info.total_sectors = (uint32_t) identify_data[60] | ((uint32_t) identify_data[61] << 16);
    ata_info.lba48_supported = (identify_data[83] & (1u << 10)) ? 1 : 0;
    ata_decode_string(&identify_data[27], 20, ata_info.model, sizeof(ata_info.model));
    if (ata_info.model[0] == '\0') {
        const char *fallback = "ATA device";
        size_t i = 0;
        for (; fallback[i] != '\0' && i + 1 < sizeof(ata_info.model); i++) {
            ata_info.model[i] = fallback[i];
        }
        ata_info.model[i] = '\0';
    }

    return 1;
}

void ata_init(void) {
    memset(&ata_info, 0, sizeof(ata_info));
    outb(ATA_CONTROL_PORT, 0x02);
    (void) ata_identify();
}

void ata_get_info(ata_device_info_t *info) {
    if (!info) {
        return;
    }
    *info = ata_info;
}

int ata_read_sectors(uint32_t lba, uint8_t count, void *buffer) {
    if (!ata_info.present || !buffer || count == 0) {
        return -1;
    }
    if (lba > ATA_LBA28_MASK) {
        return -1;
    }

    if (ata_wait_not_busy(100000) != 0) {
        return -1;
    }

    ata_select_lba(lba, count, ATA_CMD_READ_PIO);
    if (ata_wait_drq(100000) != 0) {
        return -1;
    }

    ata_insw(ATA_DATA_PORT, buffer, (uint32_t) count * (ATA_SECTOR_SIZE / 2));
    return 0;
}

int ata_write_sectors(uint32_t lba, uint8_t count, const void *buffer) {
    if (!ata_info.present || !buffer || count == 0) {
        return -1;
    }
    if (lba > ATA_LBA28_MASK) {
        return -1;
    }

    if (ata_wait_not_busy(100000) != 0) {
        return -1;
    }

    ata_select_lba(lba, count, ATA_CMD_WRITE_PIO);
    if (ata_wait_drq(100000) != 0) {
        return -1;
    }

    ata_outsw(ATA_DATA_PORT, buffer, (uint32_t) count * (ATA_SECTOR_SIZE / 2));
    outb(ATA_COMMAND_PORT, 0xE7);
    if (ata_wait_not_busy(100000) != 0) {
        return -1;
    }
    return 0;
}

int ata_disk_self_test(void) {
    uint8_t original[ATA_SECTOR_SIZE];
    uint8_t pattern[ATA_SECTOR_SIZE];
    uint8_t verify[ATA_SECTOR_SIZE];

    if (!ata_info.present || ata_info.total_sectors <= 2048) {
        return -1;
    }

    if (ata_read_sectors(2048, 1, original) != 0) {
        return -1;
    }

    for (size_t i = 0; i < ATA_SECTOR_SIZE; i++) {
        pattern[i] = (uint8_t)(0xA5u ^ (uint8_t) i);
    }

    if (ata_write_sectors(2048, 1, pattern) != 0) {
        return -1;
    }
    if (ata_read_sectors(2048, 1, verify) != 0) {
        (void) ata_write_sectors(2048, 1, original);
        return -1;
    }
    if (memcmp(pattern, verify, ATA_SECTOR_SIZE) != 0) {
        (void) ata_write_sectors(2048, 1, original);
        return -1;
    }

    return ata_write_sectors(2048, 1, original);
}
