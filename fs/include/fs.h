/*
 * AcharyaOS - fs.h
 * ----------------
 * Phase 1, Feature 13: simple filesystem.
 */

#ifndef ACHARYAOS_FS_H
#define ACHARYAOS_FS_H

#include <stddef.h>
#include <stdint.h>

#define FS_NAME_MAX 16
#define FS_FILES_MAX 32
#define FS_BLOCK_SIZE 512

typedef struct {
    uint8_t present;
    uint8_t mounted;
    uint32_t total_files;
    uint32_t used_files;
    uint32_t data_start_lba;
    char volume_label[17];
} fs_info_t;

typedef struct {
    char name[FS_NAME_MAX];
    uint32_t start_lba;
    uint32_t size_bytes;
    uint8_t used;
} fs_file_entry_t;

void fs_init(void);
int fs_mounted(void);
void fs_get_info(fs_info_t *info);
size_t fs_list_files(fs_file_entry_t *out, size_t max_files);
int fs_read_file(const char *name, void *buffer, size_t buffer_len, size_t *out_size);
int fs_write_file(const char *name, const void *data, size_t size);
int fs_delete_file(const char *name);
int fs_fsck(void);

#endif /* ACHARYAOS_FS_H */
