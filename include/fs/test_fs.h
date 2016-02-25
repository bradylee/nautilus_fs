#ifndef __TEST_FS_H
#define __TEST_FS_H

static uint8_t EOF = 255;

void test_fs(void);
uint32_t ext2_open(char * path, int access);
size_t ext2_read(int file_number, char * dst, size_t num_bytes, size_t offset);
size_t file_open(char *path, int access);
size_t file_read(int file_number, char *dst, size_t num_bytes);
void dir_ls(char* path);
size_t file_write(int file_number,char * write_data, size_t num_bytes);
size_t ext2_write(int inode_number, char* write_data, size_t num_bytes, size_t offset);
uint64_t get_ext2_file_size(int inode_number);
size_t file_seek(int file_number, size_t offset, int pos);
size_t file_append(int file_number, char* write_data, size_t num_bytes);

struct file_data {
	int status; //closed = -1, opened = thread_id
	size_t position;
	uint32_t inode;
};

static struct file_data open_files[10];

#endif
