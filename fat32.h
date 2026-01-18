#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>

/* FAT32 Filesystem Driver */

#define FAT32_MAX_FILENAME 256
#define FAT32_SECTOR_SIZE 512

/* FAT32 Boot Sector / BPB (BIOS Parameter Block) */
typedef struct {
    uint8_t  jump[3];               /* Jump instruction */
    uint8_t  oem_name[8];           /* OEM name */
    uint16_t bytes_per_sector;      /* Bytes per sector (usually 512) */
    uint8_t  sectors_per_cluster;   /* Sectors per cluster */
    uint16_t reserved_sectors;      /* Reserved sectors (including boot sector) */
    uint8_t  num_fats;              /* Number of FATs (usually 2) */
    uint16_t root_entry_count;      /* Root entries (0 for FAT32) */
    uint16_t total_sectors_16;      /* Total sectors (0 for FAT32) */
    uint8_t  media_type;            /* Media descriptor */
    uint16_t fat_size_16;           /* Sectors per FAT (0 for FAT32) */
    uint16_t sectors_per_track;     /* Sectors per track */
    uint16_t num_heads;             /* Number of heads */
    uint32_t hidden_sectors;        /* Hidden sectors */
    uint32_t total_sectors_32;      /* Total sectors */

    /* FAT32 Extended BPB */
    uint32_t fat_size_32;           /* Sectors per FAT */
    uint16_t ext_flags;             /* Extended flags */
    uint16_t fs_version;            /* Filesystem version */
    uint32_t root_cluster;          /* Root directory cluster */
    uint16_t fs_info;               /* FSInfo sector */
    uint16_t backup_boot_sector;    /* Backup boot sector */
    uint8_t  reserved[12];          /* Reserved */
    uint8_t  drive_number;          /* Drive number */
    uint8_t  reserved1;             /* Reserved */
    uint8_t  boot_signature;        /* Boot signature (0x29) */
    uint32_t volume_id;             /* Volume ID */
    uint8_t  volume_label[11];      /* Volume label */
    uint8_t  fs_type[8];            /* Filesystem type "FAT32   " */
} __attribute__((packed)) fat32_bpb_t;

/* FAT32 Directory Entry (32 bytes) */
typedef struct {
    uint8_t  name[11];              /* 8.3 filename */
    uint8_t  attr;                  /* File attributes */
    uint8_t  nt_reserved;           /* Reserved for NT */
    uint8_t  create_time_tenth;     /* Creation time (10ms units) */
    uint16_t create_time;           /* Creation time */
    uint16_t create_date;           /* Creation date */
    uint16_t last_access_date;      /* Last access date */
    uint16_t first_cluster_high;    /* High word of first cluster */
    uint16_t write_time;            /* Write time */
    uint16_t write_date;            /* Write date */
    uint16_t first_cluster_low;     /* Low word of first cluster */
    uint32_t file_size;             /* File size in bytes */
} __attribute__((packed)) fat32_direntry_t;

/* File attributes */
#define FAT32_ATTR_READ_ONLY 0x01
#define FAT32_ATTR_HIDDEN    0x02
#define FAT32_ATTR_SYSTEM    0x04
#define FAT32_ATTR_VOLUME_ID 0x08
#define FAT32_ATTR_DIRECTORY 0x10
#define FAT32_ATTR_ARCHIVE   0x20
#define FAT32_ATTR_LONG_NAME 0x0F

/* Special cluster values */
#define FAT32_CLUSTER_FREE      0x00000000
#define FAT32_CLUSTER_RESERVED  0x0FFFFFF0
#define FAT32_CLUSTER_BAD       0x0FFFFFF7
#define FAT32_CLUSTER_EOC       0x0FFFFFF8  /* End of chain */
#define FAT32_CLUSTER_MASK      0x0FFFFFFF

/* FAT32 Filesystem State */
typedef struct {
    uint8_t drive;
    fat32_bpb_t bpb;
    uint32_t fat_start_sector;
    uint32_t data_start_sector;
    uint32_t root_dir_cluster;
    uint8_t initialized;
} fat32_fs_t;

/* File handle */
typedef struct {
    uint32_t first_cluster;
    uint32_t current_cluster;
    uint32_t size;
    uint32_t position;
    uint8_t attr;
    uint8_t valid;
} fat32_file_t;

/* Initialize FAT32 filesystem */
int fat32_init(uint8_t drive);

/* List files in directory */
int fat32_list_root(void);

/* Open file */
fat32_file_t* fat32_open(const char *filename);

/* Read from file */
int fat32_read(fat32_file_t *file, uint8_t *buffer, uint32_t size);

/* Close file */
void fat32_close(fat32_file_t *file);

/* Get filesystem info */
void fat32_get_info(uint32_t *total_sectors, uint32_t *free_clusters);

#endif /* FAT32_H */
