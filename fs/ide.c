/*
 * operations on IDE disk.
 */

#include "serv.h"
#include <drivers/dev_disk.h>
#include <lib.h>
#include <mmu.h>

unsigned int sector_setting;
#define ide_set_location(secno, high32, offset) ({\
	sector_setting = offset;\
	msyscall(SYS_write_dev, &sector_setting, DEV_DISK_ADDRESS | DEV_DISK_OFFSET, 4);\
	sector_setting = high32;\
	msyscall(SYS_write_dev, &sector_setting, DEV_DISK_ADDRESS | DEV_DISK_OFFSET_HIGH32, 4);\
	sector_setting = secno;\
	msyscall(SYS_write_dev, &sector_setting, DEV_DISK_ADDRESS | DEV_DISK_ID, 4);\
})

// Overview:
//  read data from IDE disk. First issue a read request through
//  disk register and then copy data from disk buffer
//  (512 bytes, a sector) to destination array.
//
// Parameters:
//  diskno: disk number.
//  secno: start sector number.
//  dst: destination for data read from IDE disk.
//  nsecs: the number of sectors to read.
//
// Post-Condition:
//  Panic if any error occurs. (you may want to use 'panic_on')
//
// Hint: Use syscalls to access device registers and buffers.
// Hint: Use the physical address and offsets defined in 'include/drivers/dev_disk.h':
//  'DEV_DISK_ADDRESS', 'DEV_DISK_ID', 'DEV_DISK_OFFSET', 'DEV_DISK_OPERATION_READ',
//  'DEV_DISK_START_OPERATION', 'DEV_DISK_STATUS', 'DEV_DISK_BUFFER'
void ide_read(u_int diskno, u_int secno, void *dst, u_int nsecs) {
	u_int begin = secno * BY2SECT;
	u_int end = begin + nsecs * BY2SECT;

	for (u_int off = 0; begin + off < end; off += BY2SECT) {
		/* Exercise 5.3: Your code here. (1/2) */
		ide_set_location(diskno, 0, begin + off);

		sector_setting = DEV_DISK_OPERATION_READ;
		msyscall(SYS_write_dev, &sector_setting, DEV_DISK_ADDRESS | DEV_DISK_START_OPERATION, 4);

		msyscall(SYS_read_dev, &sector_setting, DEV_DISK_ADDRESS | DEV_DISK_STATUS, 4);
		panic_on(sector_setting == 0);

		panic_on(msyscall(SYS_read_dev, dst + off, DEV_DISK_ADDRESS | DEV_DISK_BUFFER, BY2SECT));
	}
}

// Overview:
//  write data to IDE disk.
//
// Parameters:
//  diskno: disk number.
//  secno: start sector number.
//  src: the source data to write into IDE disk.
//  nsecs: the number of sectors to write.
//
// Post-Condition:
//  Panic if any error occurs.
//
// Hint: Use syscalls to access device registers and buffers.
// Hint: Use the physical address and offsets defined in 'include/drivers/dev_disk.h':
//  'DEV_DISK_ADDRESS', 'DEV_DISK_ID', 'DEV_DISK_OFFSET', 'DEV_DISK_BUFFER',
//  'DEV_DISK_OPERATION_WRITE', 'DEV_DISK_START_OPERATION', 'DEV_DISK_STATUS'
void ide_write(u_int diskno, u_int secno, void *src, u_int nsecs) {
	u_int begin = secno * BY2SECT;
	u_int end = begin + nsecs * BY2SECT;

	for (u_int off = 0; begin + off < end; off += BY2SECT) {
		/* Exercise 5.3: Your code here. (2/2) */
		ide_set_location(diskno, 0, begin + off);
		panic_on(msyscall(SYS_write_dev, src + off, DEV_DISK_ADDRESS | DEV_DISK_BUFFER, BY2SECT));
		sector_setting = DEV_DISK_OPERATION_WRITE;
		msyscall(SYS_write_dev, &sector_setting, DEV_DISK_ADDRESS | DEV_DISK_START_OPERATION, 4);
		msyscall(SYS_read_dev, &sector_setting, DEV_DISK_ADDRESS | DEV_DISK_STATUS, 4);
		panic_on(sector_setting == 0);
	}
}
