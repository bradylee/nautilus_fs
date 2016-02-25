#include <fs/fs.h>
#include <fs/ext2/ext2.h>

uint32_t ext2_open(uint8_t *device, char *path, int access) {
	uint32_t inode_num = (uint32_t) get_inode_by_path(device, path);
	return inode_num;
}

size_t ext2_read(int inode_number, char * dst, size_t num_bytes,size_t offset) {
	struct ext2_inode * inode_pointer = get_inode(&RAMFS_START, inode_number);
	unsigned char * blocks = (char *)(get_block(&RAMFS_START, inode_pointer->i_block[0]));
	int i = 0;	
	uint8_t * cur = blocks + offset;
	while (i<num_bytes && *cur != 0x0a) {		
		dst[i] = *cur;		
		i++;
		cur++;
	}
	dst[i] = 0x00; //null terminator
	return i;
}

size_t ext2_write(int inode_number, char* write_data, size_t num_bytes, size_t offset) {
	struct ext2_inode * inode_pointer = get_inode(&RAMFS_START, inode_number);
	unsigned char * blocks = (char *)(get_block(&RAMFS_START, inode_pointer->i_block[0]));
	int i = 0;	
	int end_flag = 0;
	int end_bytes = 0;
	uint8_t * cur = blocks + offset;
	while (i<num_bytes) {		
		if(*cur == 0x0A) {
			end_flag = 1;
			end_bytes = num_bytes-i;
		}
		*cur = write_data[i];		
		i++;
		cur++;
	}
	if(end_flag) {
		*cur = 0x0A;
		inode_pointer->i_size += end_bytes;
	}
	return i;
}

uint64_t get_ext2_file_size(int inode_number) {
	struct ext2_inode * inode_pointer = get_inode(&RAMFS_START, inode_number);	
	uint64_t temp = inode_pointer->i_size_high;
	temp = temp << 32;
	temp = temp | (inode_pointer->i_size);
	return temp;
}
