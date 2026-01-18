#include "fat32.h"
#include "ide.h"
#include "vga.h"

/* Global filesystem state */
static fat32_fs_t fs;

/* Sector buffer */
static uint16_t sector_buffer[256];  /* 512 bytes */

/* Memory functions */
static void* memcpy(void* dest, const void* src, uint32_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    while (n--) *d++ = *s++;
    return dest;
}

static void* memset(void* s, int c, uint32_t n) {
    uint8_t* p = (uint8_t*)s;
    while (n--) *p++ = (uint8_t)c;
    return s;
}

static int strncmp(const char* s1, const char* s2, uint32_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(uint8_t*)s1 - *(uint8_t*)s2;
}

/* Read a cluster chain */
static uint32_t fat32_get_next_cluster(uint32_t cluster) {
    if (cluster < 2 || cluster >= FAT32_CLUSTER_RESERVED) {
        return FAT32_CLUSTER_EOC;
    }

    /* Calculate FAT sector and offset */
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = fs.fat_start_sector + (fat_offset / FAT32_SECTOR_SIZE);
    uint32_t entry_offset = (fat_offset % FAT32_SECTOR_SIZE) / 4;

    /* Read FAT sector */
    if (ide_read_sectors(fs.drive, fat_sector, 1, sector_buffer) != 0) {
        return FAT32_CLUSTER_EOC;
    }

    /* Get next cluster */
    uint32_t next_cluster = ((uint32_t*)sector_buffer)[entry_offset] & FAT32_CLUSTER_MASK;

    if (next_cluster >= FAT32_CLUSTER_EOC) {
        return FAT32_CLUSTER_EOC;
    }

    return next_cluster;
}

/* Convert cluster number to sector */
static uint32_t fat32_cluster_to_sector(uint32_t cluster) {
    if (cluster < 2) {
        return 0;
    }
    return fs.data_start_sector + ((cluster - 2) * fs.bpb.sectors_per_cluster);
}

/* Initialize FAT32 filesystem */
int fat32_init(uint8_t drive) {
    fs.drive = drive;
    fs.initialized = 0;

    /* Read boot sector */
    if (ide_read_sectors(drive, 0, 1, sector_buffer) != 0) {
        return -1;
    }

    /* Copy BPB */
    memcpy(&fs.bpb, sector_buffer, sizeof(fat32_bpb_t));

    /* Validate FAT32 */
    if (fs.bpb.bytes_per_sector != 512) {
        return -1;  /* Only 512-byte sectors supported */
    }

    if (fs.bpb.fat_size_16 != 0) {
        return -1;  /* Not FAT32 (FAT12/16 has non-zero fat_size_16) */
    }

    /* Calculate important values */
    fs.fat_start_sector = fs.bpb.reserved_sectors;
    fs.data_start_sector = fs.bpb.reserved_sectors +
                           (fs.bpb.num_fats * fs.bpb.fat_size_32);
    fs.root_dir_cluster = fs.bpb.root_cluster;

    fs.initialized = 1;
    return 0;
}

/* Convert 8.3 name to regular string */
static void fat32_name_to_string(const uint8_t *fat_name, char *output) {
    int i, j = 0;

    /* Copy name part */
    for (i = 0; i < 8 && fat_name[i] != ' '; i++) {
        output[j++] = fat_name[i];
    }

    /* Add extension if present */
    if (fat_name[8] != ' ') {
        output[j++] = '.';
        for (i = 8; i < 11 && fat_name[i] != ' '; i++) {
            output[j++] = fat_name[i];
        }
    }

    output[j] = '\0';
}

/* Convert string to 8.3 name */
static void fat32_string_to_name(const char *input, uint8_t *fat_name) {
    int i;

    /* Initialize with spaces */
    memset(fat_name, ' ', 11);

    /* Copy name part */
    for (i = 0; i < 8 && input[i] && input[i] != '.'; i++) {
        fat_name[i] = input[i];
    }

    /* Find extension */
    const char *ext = input;
    while (*ext && *ext != '.') ext++;

    if (*ext == '.') {
        ext++;
        for (i = 0; i < 3 && ext[i]; i++) {
            fat_name[8 + i] = ext[i];
        }
    }
}

/* List files in root directory */
int fat32_list_root(void) {
    if (!fs.initialized) {
        return -1;
    }

    vga_puts("Files in root directory:\n");
    vga_puts("------------------------\n");

    uint32_t cluster = fs.root_dir_cluster;
    int file_count = 0;

    while (cluster < FAT32_CLUSTER_EOC) {
        uint32_t sector = fat32_cluster_to_sector(cluster);

        /* Read all sectors in this cluster */
        for (uint32_t i = 0; i < fs.bpb.sectors_per_cluster; i++) {
            if (ide_read_sectors(fs.drive, sector + i, 1, sector_buffer) != 0) {
                return -1;
            }

            fat32_direntry_t *entries = (fat32_direntry_t *)sector_buffer;

            /* Process all directory entries in this sector */
            for (int j = 0; j < 16; j++) {  /* 512 / 32 = 16 entries per sector */
                fat32_direntry_t *entry = &entries[j];

                /* End of directory */
                if (entry->name[0] == 0x00) {
                    vga_puts("\nTotal files: ");
                    char num[16];
                    int k = 0;
                    int n = file_count;
                    if (n == 0) {
                        num[k++] = '0';
                    } else {
                        char temp[16];
                        int t = 0;
                        while (n > 0) {
                            temp[t++] = '0' + (n % 10);
                            n /= 10;
                        }
                        while (t > 0) {
                            num[k++] = temp[--t];
                        }
                    }
                    num[k] = '\0';
                    vga_puts(num);
                    vga_puts("\n");
                    return file_count;
                }

                /* Skip deleted entries */
                if (entry->name[0] == 0xE5) {
                    continue;
                }

                /* Skip long name entries */
                if (entry->attr == FAT32_ATTR_LONG_NAME) {
                    continue;
                }

                /* Skip volume label */
                if (entry->attr & FAT32_ATTR_VOLUME_ID) {
                    continue;
                }

                /* Convert and print filename */
                char filename[13];
                fat32_name_to_string(entry->name, filename);

                if (entry->attr & FAT32_ATTR_DIRECTORY) {
                    vga_puts("[DIR]  ");
                } else {
                    vga_puts("[FILE] ");
                }

                vga_puts(filename);
                vga_puts(" (");
                vga_puthex(entry->file_size);
                vga_puts(" bytes)\n");

                file_count++;
            }
        }

        /* Get next cluster */
        cluster = fat32_get_next_cluster(cluster);
    }

    vga_puts("\nTotal files: ");
    char num[16];
    int k = 0;
    int n = file_count;
    if (n == 0) {
        num[k++] = '0';
    } else {
        char temp[16];
        int t = 0;
        while (n > 0) {
            temp[t++] = '0' + (n % 10);
            n /= 10;
        }
        while (t > 0) {
            num[k++] = temp[--t];
        }
    }
    num[k] = '\0';
    vga_puts(num);
    vga_puts("\n");

    return file_count;
}

/* Open a file */
fat32_file_t* fat32_open(const char *filename) {
    static fat32_file_t file;

    if (!fs.initialized) {
        return NULL;
    }

    uint8_t search_name[11];
    fat32_string_to_name(filename, search_name);

    uint32_t cluster = fs.root_dir_cluster;

    while (cluster < FAT32_CLUSTER_EOC) {
        uint32_t sector = fat32_cluster_to_sector(cluster);

        for (uint32_t i = 0; i < fs.bpb.sectors_per_cluster; i++) {
            if (ide_read_sectors(fs.drive, sector + i, 1, sector_buffer) != 0) {
                return NULL;
            }

            fat32_direntry_t *entries = (fat32_direntry_t *)sector_buffer;

            for (int j = 0; j < 16; j++) {
                fat32_direntry_t *entry = &entries[j];

                if (entry->name[0] == 0x00) {
                    return NULL;  /* File not found */
                }

                if (entry->name[0] == 0xE5) {
                    continue;  /* Deleted */
                }

                if (entry->attr == FAT32_ATTR_LONG_NAME) {
                    continue;  /* Long name */
                }

                if (strncmp((char*)entry->name, (char*)search_name, 11) == 0) {
                    /* Found it! */
                    file.first_cluster = ((uint32_t)entry->first_cluster_high << 16) |
                                        entry->first_cluster_low;
                    file.current_cluster = file.first_cluster;
                    file.size = entry->file_size;
                    file.position = 0;
                    file.attr = entry->attr;
                    file.valid = 1;
                    return &file;
                }
            }
        }

        cluster = fat32_get_next_cluster(cluster);
    }

    return NULL;  /* File not found */
}

/* Read from file */
int fat32_read(fat32_file_t *file, uint8_t *buffer, uint32_t size) {
    if (!file || !file->valid || !fs.initialized) {
        return -1;
    }

    uint32_t bytes_read = 0;
    uint32_t bytes_to_read = size;

    /* Don't read past end of file */
    if (file->position + bytes_to_read > file->size) {
        bytes_to_read = file->size - file->position;
    }

    while (bytes_to_read > 0 && file->current_cluster < FAT32_CLUSTER_EOC) {
        uint32_t cluster_size = fs.bpb.sectors_per_cluster * FAT32_SECTOR_SIZE;
        uint32_t offset_in_cluster = file->position % cluster_size;
        uint32_t bytes_in_cluster = cluster_size - offset_in_cluster;

        if (bytes_in_cluster > bytes_to_read) {
            bytes_in_cluster = bytes_to_read;
        }

        /* Read sector(s) */
        uint32_t sector = fat32_cluster_to_sector(file->current_cluster) +
                         (offset_in_cluster / FAT32_SECTOR_SIZE);
        uint32_t offset_in_sector = offset_in_cluster % FAT32_SECTOR_SIZE;

        if (ide_read_sectors(fs.drive, sector, 1, sector_buffer) != 0) {
            return bytes_read;
        }

        /* Copy data */
        uint32_t bytes_in_sector = FAT32_SECTOR_SIZE - offset_in_sector;
        if (bytes_in_sector > bytes_in_cluster) {
            bytes_in_sector = bytes_in_cluster;
        }

        memcpy(buffer, (uint8_t*)sector_buffer + offset_in_sector, bytes_in_sector);

        buffer += bytes_in_sector;
        bytes_read += bytes_in_sector;
        bytes_to_read -= bytes_in_sector;
        file->position += bytes_in_sector;

        /* Move to next cluster if needed */
        if ((file->position % cluster_size) == 0) {
            file->current_cluster = fat32_get_next_cluster(file->current_cluster);
        }
    }

    return bytes_read;
}

/* Close file */
void fat32_close(fat32_file_t *file) {
    if (file) {
        file->valid = 0;
    }
}

/* Get filesystem info */
void fat32_get_info(uint32_t *total_sectors, uint32_t *free_clusters) {
    if (total_sectors) {
        *total_sectors = fs.bpb.total_sectors_32;
    }

    if (free_clusters) {
        *free_clusters = 0;  /* Would need to scan entire FAT */
    }
}
