#ifndef IDE_H
#define IDE_H

#include <stdint.h>

/* IDE Ports (Primary Bus) */
#define IDE_PRIMARY_BASE    0x1F0
#define IDE_PRIMARY_CTRL    0x3F6

/* IDE Registers */
#define IDE_REG_DATA        0x00
#define IDE_REG_ERROR       0x01
#define IDE_REG_FEATURES    0x01
#define IDE_REG_SECTOR_CNT  0x02
#define IDE_REG_LBA_LOW     0x03
#define IDE_REG_LBA_MID     0x04
#define IDE_REG_LBA_HIGH    0x05
#define IDE_REG_DRIVE       0x06
#define IDE_REG_STATUS      0x07
#define IDE_REG_COMMAND     0x07

/* IDE Commands */
#define IDE_CMD_READ_SECTORS  0x20
#define IDE_CMD_WRITE_SECTORS 0x30
#define IDE_CMD_IDENTIFY      0xEC

/* IDE Status Bits */
#define IDE_STATUS_ERR   0x01  /* Error */
#define IDE_STATUS_DRQ   0x08  /* Data Request */
#define IDE_STATUS_SRV   0x10  /* Service */
#define IDE_STATUS_DF    0x20  /* Drive Fault */
#define IDE_STATUS_RDY   0x40  /* Ready */
#define IDE_STATUS_BSY   0x80  /* Busy */

/* Drive selection */
#define IDE_DRIVE_MASTER 0xE0
#define IDE_DRIVE_SLAVE  0xF0

/* Initialize IDE controller */
int ide_init(void);

/* Read sectors from disk */
int ide_read_sectors(uint8_t drive, uint32_t lba, uint8_t sector_count, uint16_t *buffer);

/* Write sectors to disk */
int ide_write_sectors(uint8_t drive, uint32_t lba, uint8_t sector_count, uint16_t *buffer);

/* Identify drive */
int ide_identify(uint8_t drive, uint16_t *buffer);

#endif /* IDE_H */
