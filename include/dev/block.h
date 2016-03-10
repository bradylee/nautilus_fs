#ifndef __BLOCK_H__
#define __BLOCK_H__

#include <nautilus/naut_types.h>
#include <nautilus/list.h>
#include <nautilus/printk.h>

extern uint8_t RAMFS_START, RAMFS_END;

struct block_dev_int {
	uint64_t (*get_block_size)(void *state);
	uint64_t (*get_num_blocks)(void *state);
	int (*read_block)(void *state, uint64_t blocknum, uint8_t *dest);
	int (*write_block)(void *state, uint64_t blocknum, uint8_t *src);
};

struct block_dev {
	struct list_head block_node;
	char name[32];
	struct block_dev_int interface;
	void *state;
};

struct block_dev * block_dev_register(char *name, struct block_dev_int *interface, void *state);
int block_dev_unregister(struct block_dev *bd);

struct block_dev * block_dev_find(char *name);

///*
static inline uint64_t block_dev_get_block_size(struct block_dev *bd) {
	return bd->interface.get_block_size(bd->state);
}
//*/

#endif
