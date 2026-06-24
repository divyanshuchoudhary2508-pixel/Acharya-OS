/*
 * AcharyaOS - fs.c
 * ----------------
 * Tiny contiguous-file filesystem for Phase 1, Feature 13.
 */

#include "fs.h"
#include "ata.h"
#include "kstring.h"
#include "kernel.h"

#define FS_MAGIC_VALUE 0x53485341u
#define FS_VERSION 1u
#define FS_SUPERBLOCK_LBA 4096u
#define FS_TABLE_LBA 4097u
#define FS_DATA_LBA 4098u
#define FS_MAX_FILE_BYTES (256u * 512u)

typedef struct PACKED {
    uint32_t magic;
    uint32_t version;
    char volume_label[17];
    uint32_t total_files;
    uint32_t data_start_lba;
    uint32_t reserved[122];
} fs_superblock_t;

typedef struct PACKED {
    char name[FS_NAME_MAX];
    uint32_t start_lba;
    uint32_t size_bytes;
    uint8_t used;
    uint8_t reserved[11];
} fs_disk_entry_t;

static fs_info_t fs_info;
static fs_file_entry_t fs_cache[FS_FILES_MAX];
static fs_disk_entry_t fs_disk_entries[FS_FILES_MAX];

static void fs_clear_cache(void) {
    memset(&fs_info, 0, sizeof(fs_info));
    for (size_t i = 0; i < FS_FILES_MAX; i++) {
        memset(&fs_cache[i], 0, sizeof(fs_cache[i]));
        memset(&fs_disk_entries[i], 0, sizeof(fs_disk_entries[i]));
    }
}

static void fs_copy_name(char dest[FS_NAME_MAX], const char *src) {
    size_t i = 0;
    for (; i + 1 < FS_NAME_MAX && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

static int fs_name_equals(const char a[FS_NAME_MAX], const char *b) {
    for (size_t i = 0; i < FS_NAME_MAX; i++) {
        if (a[i] != b[i]) {
            return 0;
        }
        if (a[i] == '\0') {
            return 1;
        }
    }
    return b[FS_NAME_MAX - 1] == '\0';
}

static int fs_load_superblock(fs_superblock_t *sb) {
    return ata_read_sectors(FS_SUPERBLOCK_LBA, 1, sb);
}

static int fs_load_table(void) {
    return ata_read_sectors(FS_TABLE_LBA, 1, fs_disk_entries);
}

static int fs_store_superblock(const fs_superblock_t *sb) {
    return ata_write_sectors(FS_SUPERBLOCK_LBA, 1, sb);
}

static int fs_store_table(void) {
    return ata_write_sectors(FS_TABLE_LBA, 1, fs_disk_entries);
}

static void fs_sync_cache(void) {
    for (size_t i = 0; i < FS_FILES_MAX; i++) {
        fs_cache[i].used = fs_disk_entries[i].used;
        fs_cache[i].start_lba = fs_disk_entries[i].start_lba;
        fs_cache[i].size_bytes = fs_disk_entries[i].size_bytes;
        memcpy(fs_cache[i].name, fs_disk_entries[i].name, FS_NAME_MAX);
        fs_cache[i].name[FS_NAME_MAX - 1] = '\0';
    }
}

static uint32_t fs_blocks_for_bytes(uint32_t size_bytes) {
    return (size_bytes + FS_BLOCK_SIZE - 1u) / FS_BLOCK_SIZE;
}

static void fs_write_label(char dest[17]) {
    const char *label = "ACHARYAOS_FS";
    size_t i = 0;
    for (; i + 1 < 17 && label[i] != '\0'; i++) {
        dest[i] = label[i];
    }
    dest[i] = '\0';
}

static int fs_format(void) {
    fs_superblock_t sb;

    memset(&sb, 0, sizeof(sb));
    sb.magic = FS_MAGIC_VALUE;
    sb.version = FS_VERSION;
    fs_write_label(sb.volume_label);
    sb.total_files = FS_FILES_MAX;
    sb.data_start_lba = FS_DATA_LBA;

    memset(fs_disk_entries, 0, sizeof(fs_disk_entries));

    if (fs_store_superblock(&sb) != 0) {
        return -1;
    }
    if (fs_store_table() != 0) {
        return -1;
    }

    fs_info.present = 1;
    fs_info.mounted = 1;
    fs_info.total_files = FS_FILES_MAX;
    fs_info.used_files = 0;
    fs_info.data_start_lba = FS_DATA_LBA;
    memcpy(fs_info.volume_label, sb.volume_label, 16);
    fs_info.volume_label[16] = '\0';
    fs_sync_cache();
    return 0;
}

void fs_init(void) {
    fs_superblock_t sb;
    ata_device_info_t ata;

    fs_clear_cache();
    ata_get_info(&ata);
    if (!ata.present) {
        fs_info.present = 0;
        fs_info.mounted = 0;
        return;
    }

    if (fs_load_superblock(&sb) != 0 || sb.magic != FS_MAGIC_VALUE || sb.version != FS_VERSION) {
        (void) fs_format();
        return;
    }

    if (fs_load_table() != 0) {
        fs_info.present = 0;
        fs_info.mounted = 0;
        return;
    }

    fs_info.present = 1;
    fs_info.mounted = 1;
    fs_info.total_files = sb.total_files;
    fs_info.data_start_lba = sb.data_start_lba;
    memcpy(fs_info.volume_label, sb.volume_label, 16);
    fs_info.volume_label[16] = '\0';
    fs_info.used_files = 0;
    for (size_t i = 0; i < FS_FILES_MAX; i++) {
        if (fs_disk_entries[i].used) {
            fs_info.used_files++;
        }
    }
    fs_sync_cache();
}

int fs_mounted(void) {
    return fs_info.mounted;
}

void fs_get_info(fs_info_t *info) {
    if (!info) {
        return;
    }
    *info = fs_info;
}

size_t fs_list_files(fs_file_entry_t *out, size_t max_files) {
    if (!out) {
        return 0;
    }
    size_t count = 0;
    for (size_t i = 0; i < FS_FILES_MAX && count < max_files; i++) {
        if (!fs_disk_entries[i].used) {
            continue;
        }
        out[count++] = fs_cache[i];
    }
    return count;
}

static int fs_find_entry(const char *name) {
    for (size_t i = 0; i < FS_FILES_MAX; i++) {
        if (fs_disk_entries[i].used && fs_name_equals(fs_disk_entries[i].name, name)) {
            return (int) i;
        }
    }
    return -1;
}

static int fs_find_free_entry(void) {
    for (size_t i = 0; i < FS_FILES_MAX; i++) {
        if (!fs_disk_entries[i].used) {
            return (int) i;
        }
    }
    return -1;
}

static uint32_t fs_next_free_lba(void) {
    uint32_t next = fs_info.data_start_lba;
    for (size_t i = 0; i < FS_FILES_MAX; i++) {
        if (!fs_disk_entries[i].used) {
            continue;
        }
        uint32_t end = fs_disk_entries[i].start_lba + fs_blocks_for_bytes(fs_disk_entries[i].size_bytes);
        if (end > next) {
            next = end;
        }
    }
    return next;
}

int fs_read_file(const char *name, void *buffer, size_t buffer_len, size_t *out_size) {
    if (!fs_info.mounted || !name || !buffer) {
        return -1;
    }
    int idx = fs_find_entry(name);
    if (idx < 0) {
        return -1;
    }

    uint32_t size_bytes = fs_disk_entries[idx].size_bytes;
    uint32_t blocks = fs_blocks_for_bytes(size_bytes);
    if (buffer_len < size_bytes) {
        return -1;
    }
    if (blocks == 0) {
        if (out_size) {
            *out_size = 0;
        }
        return 0;
    }

    if (ata_read_sectors(fs_disk_entries[idx].start_lba, (uint8_t) blocks, buffer) != 0) {
        return -1;
    }
    if (out_size) {
        *out_size = size_bytes;
    }
    return 0;
}

int fs_write_file(const char *name, const void *data, size_t size) {
    if (!fs_info.mounted || !name || !data) {
        return -1;
    }
    if (size > FS_MAX_FILE_BYTES) {
        return -1;
    }

    int idx = fs_find_entry(name);
    if (idx < 0) {
        idx = fs_find_free_entry();
        if (idx < 0) {
            return -1;
        }
        fs_copy_name(fs_disk_entries[idx].name, name);
        fs_disk_entries[idx].used = 1;
    }

    uint32_t blocks = fs_blocks_for_bytes((uint32_t) size);
    uint32_t start_lba = fs_next_free_lba();
    uint8_t sector[FS_BLOCK_SIZE];

    fs_disk_entries[idx].start_lba = start_lba;
    fs_disk_entries[idx].size_bytes = (uint32_t) size;

    for (uint32_t i = 0; i < blocks; i++) {
        uint32_t chunk = FS_BLOCK_SIZE;
        const uint8_t *src = (const uint8_t *) data + (i * FS_BLOCK_SIZE);
        if ((i + 1) * FS_BLOCK_SIZE > size) {
            chunk = (uint32_t)(size - (i * FS_BLOCK_SIZE));
        }
        memset(sector, 0, FS_BLOCK_SIZE);
        memcpy(sector, src, chunk);
        if (ata_write_sectors(start_lba + i, 1, sector) != 0) {
            return -1;
        }
    }

    if (fs_store_table() != 0) {
        return -1;
    }

    fs_info.used_files = 0;
    for (size_t i = 0; i < FS_FILES_MAX; i++) {
        if (fs_disk_entries[i].used) {
            fs_info.used_files++;
        }
    }
    fs_sync_cache();
    return 0;
}

int fs_delete_file(const char *name) {
    int idx = fs_find_entry(name);
    if (idx < 0) {
        return -1;
    }
    memset(&fs_disk_entries[idx], 0, sizeof(fs_disk_entries[idx]));
    if (fs_store_table() != 0) {
        return -1;
    }
    if (fs_info.used_files > 0) {
        fs_info.used_files--;
    }
    fs_sync_cache();
    return 0;
}

int fs_fsck(void) {
    fs_superblock_t sb;
    if (fs_load_superblock(&sb) != 0) {
        return -1;
    }
    if (sb.magic != FS_MAGIC_VALUE || sb.version != FS_VERSION) {
        return -1;
    }
    if (fs_load_table() != 0) {
        return -1;
    }
    return 0;
}
