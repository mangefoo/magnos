#include "ide.h"

/* Port I/O functions */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void inw_buffer(uint16_t port, uint16_t *buffer, uint32_t count) {
    __asm__ volatile ("cld; rep insw" : "+D"(buffer), "+c"(count) : "d"(port) : "memory");
}

static inline void outw_buffer(uint16_t port, uint16_t *buffer, uint32_t count) {
    __asm__ volatile ("cld; rep outsw" : "+S"(buffer), "+c"(count) : "d"(port) : "memory");
}

/* Wait for IDE controller to be ready */
static int ide_wait_ready(void) {
    uint8_t status;
    int timeout = 100000;

    while (timeout--) {
        status = inb(IDE_PRIMARY_BASE + IDE_REG_STATUS);
        if (!(status & IDE_STATUS_BSY) && (status & IDE_STATUS_RDY)) {
            return 0;
        }
    }
    return -1;  /* Timeout */
}

/* Wait for data request */
static int ide_wait_drq(void) {
    uint8_t status;
    int timeout = 100000;

    while (timeout--) {
        status = inb(IDE_PRIMARY_BASE + IDE_REG_STATUS);
        if (status & IDE_STATUS_DRQ) {
            return 0;
        }
        if (status & IDE_STATUS_ERR) {
            return -1;  /* Error */
        }
    }
    return -1;  /* Timeout */
}

/* Delay for 400ns (read alternate status 4 times) */
static void ide_400ns_delay(void) {
    for (int i = 0; i < 4; i++) {
        inb(IDE_PRIMARY_CTRL);
    }
}

/* Initialize IDE controller */
int ide_init(void) {
    /* Disable interrupts first */
    outb(IDE_PRIMARY_CTRL, 0x02);
    ide_400ns_delay();

    /* Check if drive exists by reading status */
    uint8_t status = inb(IDE_PRIMARY_BASE + IDE_REG_STATUS);

    /* If status is 0xFF, no drive present */
    if (status == 0xFF) {
        return -1;
    }

    /* Select drive 0 (master) */
    outb(IDE_PRIMARY_BASE + IDE_REG_DRIVE, 0xA0);
    ide_400ns_delay();

    /* Wait for controller to be ready */
    int timeout = 100000;
    while (timeout--) {
        status = inb(IDE_PRIMARY_BASE + IDE_REG_STATUS);
        if (!(status & IDE_STATUS_BSY)) {
            break;
        }
    }

    if (timeout <= 0) {
        return -1;
    }

    return 0;
}

/* Read sectors from disk */
int ide_read_sectors(uint8_t drive, uint32_t lba, uint8_t sector_count, uint16_t *buffer) {
    if (sector_count == 0) {
        return -1;
    }

    /* Wait for ready */
    if (ide_wait_ready() != 0) {
        return -1;
    }

    /* Select drive and set LBA mode */
    outb(IDE_PRIMARY_BASE + IDE_REG_DRIVE, (drive & 0x10) | 0xE0 | ((lba >> 24) & 0x0F));
    ide_400ns_delay();

    /* Set sector count and LBA */
    outb(IDE_PRIMARY_BASE + IDE_REG_SECTOR_CNT, sector_count);
    outb(IDE_PRIMARY_BASE + IDE_REG_LBA_LOW, (uint8_t)lba);
    outb(IDE_PRIMARY_BASE + IDE_REG_LBA_MID, (uint8_t)(lba >> 8));
    outb(IDE_PRIMARY_BASE + IDE_REG_LBA_HIGH, (uint8_t)(lba >> 16));

    /* Send read command */
    outb(IDE_PRIMARY_BASE + IDE_REG_COMMAND, IDE_CMD_READ_SECTORS);

    /* Read sectors */
    for (int i = 0; i < sector_count; i++) {
        if (ide_wait_drq() != 0) {
            return -1;
        }

        /* Read 256 words (512 bytes) */
        inw_buffer(IDE_PRIMARY_BASE + IDE_REG_DATA, buffer, 256);
        buffer += 256;
    }

    return 0;
}

/* Write sectors to disk */
int ide_write_sectors(uint8_t drive, uint32_t lba, uint8_t sector_count, uint16_t *buffer) {
    if (sector_count == 0) {
        return -1;
    }

    /* Wait for ready */
    if (ide_wait_ready() != 0) {
        return -1;
    }

    /* Select drive and set LBA mode */
    outb(IDE_PRIMARY_BASE + IDE_REG_DRIVE, (drive & 0x10) | 0xE0 | ((lba >> 24) & 0x0F));
    ide_400ns_delay();

    /* Set sector count and LBA */
    outb(IDE_PRIMARY_BASE + IDE_REG_SECTOR_CNT, sector_count);
    outb(IDE_PRIMARY_BASE + IDE_REG_LBA_LOW, (uint8_t)lba);
    outb(IDE_PRIMARY_BASE + IDE_REG_LBA_MID, (uint8_t)(lba >> 8));
    outb(IDE_PRIMARY_BASE + IDE_REG_LBA_HIGH, (uint8_t)(lba >> 16));

    /* Send write command */
    outb(IDE_PRIMARY_BASE + IDE_REG_COMMAND, IDE_CMD_WRITE_SECTORS);

    /* Write sectors */
    for (int i = 0; i < sector_count; i++) {
        if (ide_wait_drq() != 0) {
            return -1;
        }

        /* Write 256 words (512 bytes) */
        outw_buffer(IDE_PRIMARY_BASE + IDE_REG_DATA, buffer, 256);
        buffer += 256;

        /* Flush cache */
        ide_400ns_delay();
    }

    /* Wait for completion */
    if (ide_wait_ready() != 0) {
        return -1;
    }

    return 0;
}

/* Identify drive */
int ide_identify(uint8_t drive, uint16_t *buffer) {
    /* Wait for ready */
    if (ide_wait_ready() != 0) {
        return -1;
    }

    /* Select drive */
    outb(IDE_PRIMARY_BASE + IDE_REG_DRIVE, drive & 0x10);
    ide_400ns_delay();

    /* Send identify command */
    outb(IDE_PRIMARY_BASE + IDE_REG_COMMAND, IDE_CMD_IDENTIFY);

    /* Check if drive exists */
    uint8_t status = inb(IDE_PRIMARY_BASE + IDE_REG_STATUS);
    if (status == 0) {
        return -1;  /* Drive doesn't exist */
    }

    /* Wait for data */
    if (ide_wait_drq() != 0) {
        return -1;
    }

    /* Read identify data (256 words) */
    inw_buffer(IDE_PRIMARY_BASE + IDE_REG_DATA, buffer, 256);

    return 0;
}
