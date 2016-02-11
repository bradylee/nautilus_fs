#include <dev/fs.h>

/*
	void setup_disk(struct Disk *disk) {
		(disk->contents) = &ramdisk_start;
		(disk->size) = (uint64_t)(&ramdisk_end - &ramdisk_start);
}

void print_disk_contents(struct Disk *disk, uint64_t offset, uint64_t size) {
	uint8_t *content = disk->contents + offset;

	for (int i = 0; i < size; i++) {
		printk("%c", content[i]);
	}
	printk("\n");
}

void dump_disk_contents(struct Disk *disk) {
	print_disk_contents(disk, 0, disk->size);
	disk->size = 6;
	printk("%d", disk->size);
}
*/
