#ifndef __TEST_FS_H
#define __TEST_FS_H

#include <nautilus/list.h>
#include <nautilus/printk.h>

static uint8_t EOF = 255;
static struct list_head open_files;

struct file_data {
	struct list_head file_node;
	int status; //closed = -1, opened = thread_id
	size_t position;
	uint32_t filenum;
};

void test_fs(void);
void init_fs(void);
void deinit_fs(void);

uint32_t ext2_open(uint8_t *device, char *path, int access);
size_t ext2_read(int file_number, char * dst, size_t num_bytes, size_t offset);

size_t file_open(char *path, int access);
int file_close(uint32_t filenum);
size_t file_read(int file_number, char *dst, size_t num_bytes);

struct file_data* get_opened_file(uint32_t filenum);
void print_opened_files(void);

void dir_ls(char *path);

#endif
