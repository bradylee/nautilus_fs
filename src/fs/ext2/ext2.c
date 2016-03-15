#include <fs/ext2/ext2.h>

uint32_t ext2_open(uint8_t *device, char *path, int access) {
	
	uint32_t inode_num = (uint32_t) get_inode_by_path(device, path);
	return inode_num;
}

size_t ext2_read(int inode_number, char *buf, size_t num_bytes,size_t offset) {
	struct ext2_inode * inode_pointer = get_inode(&RAMFS_START, inode_number);
	
	int block_offset = offset/EXT2_MIN_BLOCK_SIZE;
	int offset_remainder = offset-(EXT2_MIN_BLOCK_SIZE*block_offset);
	int bytes_from_first = EXT2_MIN_BLOCK_SIZE-offset_remainder;
	int blocks_to_read = 0;
	int remainder_to_read = 0;
	if (num_bytes > bytes_from_first) {
		int temp = num_bytes;
		temp -= bytes_from_first;
		blocks_to_read = temp/EXT2_MIN_BLOCK_SIZE;
		remainder_to_read = temp-(EXT2_MIN_BLOCK_SIZE*blocks_to_read);
	}
	else {
		bytes_from_first = num_bytes;
	}

	unsigned char * blocks = (char *)(get_block(&RAMFS_START, inode_pointer->i_block[block_offset]));
	int total_bytes_read = 0;	
	uint8_t * cur = blocks + offset_remainder;
	int i = 0;
	//If trying to read past the first 12 blocks, end early
	if ((block_offset + blocks_to_read + (remainder_to_read != 0)) >=12) {
		if(block_offset < 12) {
			blocks_to_read = 11-block_offset;
		}
		else {
			blocks_to_read = 0;
			bytes_from_first = 0;
		}
		remainder_to_read = 0;
	}
	printk("read info: %d %d %d %d %d\n", block_offset, offset_remainder, bytes_from_first, blocks_to_read, remainder_to_read);
	//Read to end of first partial block
	while (i<bytes_from_first && *cur != 0x0a) {		
		buf[total_bytes_read] = *cur;		
		i++;
		total_bytes_read++;
		cur++;
	}
	//Read as many full blocks as needed
	while (blocks_to_read>0) {
		block_offset++;
		cur = (char*)(get_block(&RAMFS_START,inode_pointer->i_block[block_offset]));
		i = 0;	
		while (i<EXT2_MIN_BLOCK_SIZE && *cur != 0x0a) {		
			buf[total_bytes_read] = *cur;		
			i++;
			cur++;
			total_bytes_read++;
		}
		blocks_to_read--;
		
	}
	i = 0;
	block_offset++;
	cur = (char*)(get_block(&RAMFS_START,inode_pointer->i_block[block_offset]));
	//Read last partial block
	while (i<remainder_to_read && *cur != 0x0a) {			
		//printk("Reading Block: %d\n",block_offset);
		buf[total_bytes_read] = *cur;		
		i++;
		total_bytes_read++;
		cur++;
	}
	buf[total_bytes_read] = 0x00; //null terminator
	return total_bytes_read;
}

size_t ext2_write(int inode_number, char *buf, size_t num_bytes, size_t offset) {
	struct ext2_inode * inode_pointer = get_inode(&RAMFS_START, inode_number);

	int block_offset = offset/EXT2_MIN_BLOCK_SIZE;
	int offset_remainder = offset-(EXT2_MIN_BLOCK_SIZE*block_offset);
	int bytes_from_first = EXT2_MIN_BLOCK_SIZE-offset_remainder;
	int blocks_to_write = 0;
	int remainder_to_write = 0;
	if (num_bytes > bytes_from_first) {
		int temp = num_bytes;
		temp -= bytes_from_first;
		blocks_to_write = temp/EXT2_MIN_BLOCK_SIZE;
		remainder_to_write = temp-(EXT2_MIN_BLOCK_SIZE*blocks_to_write);
	}
	else {
		bytes_from_first = num_bytes;
	}
	unsigned char * blocks = (char *)(get_block(&RAMFS_START, inode_pointer->i_block[block_offset]));
	int i = 0;	
	int total_bytes_written = 0;
	int end_flag = 0;
	int end_bytes = 0;
	uint8_t * cur = blocks + offset_remainder;
	if(ext2_get_file_size(inode_number) == 0) {
		end_flag = 1;
		end_bytes = num_bytes;
	}
	
	//If trying to write past the first 12 blocks, end early
	if ((block_offset + blocks_to_write + (remainder_to_write != 0)) >=12) {
		if(block_offset < 12) {
			blocks_to_write = 11-block_offset;
		}
		else {
			blocks_to_write = 0;
			bytes_from_first = 0;
		}
		remainder_to_write = 0;
	}
printk("write info: %d %d %d %d %d\n", block_offset, offset_remainder, bytes_from_first, blocks_to_write, remainder_to_write);

	//Write to end of first partial block
	while (total_bytes_written<bytes_from_first) {		
		if(*cur == 0x0A && !end_flag) {
			end_flag = 1;
			end_bytes = num_bytes-total_bytes_written;
		}
		*cur = buf[total_bytes_written];		
		total_bytes_written++;
		cur++;
	}
	//Write as many full blocks as needed
	while (blocks_to_write>0) {
		block_offset++;
		cur = (char*)(get_block(&RAMFS_START,inode_pointer->i_block[block_offset]));
		i = 0;	
		while (i<EXT2_MIN_BLOCK_SIZE) {		
			if(*cur == 0x0A && !end_flag) {
				end_flag = 1;
				end_bytes = num_bytes-total_bytes_written;
			}
			*cur = buf[total_bytes_written];		
			i++;
			cur++;
			total_bytes_written++;
		}
		blocks_to_write--;
	}
	i = 0;
	block_offset++;
	cur = (char*)(get_block(&RAMFS_START,inode_pointer->i_block[block_offset]));
	//Write last partial block
	while (i<remainder_to_write) {			
		//printk("Writing Block: %d\n",block_offset);
		if(*cur == 0x0A && !end_flag) {
			end_flag = 1;
			end_bytes = num_bytes-total_bytes_written;
		}
		*cur = buf[total_bytes_written];	
		i++;
		total_bytes_written++;
		cur++;
	}

	//printk("end: %d\n",end_flag);
	if(end_flag) {
		*cur = 0x0A;
		inode_pointer->i_size += end_bytes;
	}
	return total_bytes_written;
}

uint64_t ext2_get_file_size(int inode_number) {
	struct ext2_inode * inode_pointer = get_inode(&RAMFS_START, inode_number);	
	uint64_t temp = inode_pointer->i_size_high;
	temp = temp << 32;
	temp = temp | (inode_pointer->i_size);
	return temp;
}

uint32_t ext2_file_exist(uint8_t *device, char *path) {
	uint32_t inode_num = (uint32_t) get_inode_by_path(device, path);
	if(inode_num) {
		return 1;
	}
	return 0;
}

uint32_t ext2_file_create(uint8_t *device, char *path) {
	uint32_t found_inode = get_free_inode(device);
	
	if(found_inode) {
		struct ext2_inode * inode_pointer = get_inode(device, found_inode);
		
		//go to directory file and create directory entry
		char** parts = split_path(path);
		int num_parts = 0;
    for (char * slash = path; slash != NULL; slash = strchr(slash + 1, '/')) {       
			num_parts++;
    }
		
		int newstring_size = 0;
		int i;
		for (i=0; i<num_parts-1; i++) {
				newstring_size += strlen(parts[i]) + 1;
		}
		
		char* newstring = malloc(newstring_size);
		strcpy(newstring,"/");
		for (i=0; i<num_parts-1; i++) {
				strcat(newstring,parts[i]);
				if(i != num_parts-2) {
						strcat(newstring,"/");
				}
		}

		char* name = parts[num_parts-1];
		int dir_num = 0;
		if(strcmp(newstring,"/")) {
			dir_num = get_inode_by_path(device, newstring);
		}
		else {
			dir_num = EXT2_ROOT_INO;
		}
		if (dir_num) {
				add_to_dir(device, dir_num, found_inode, name, 1);
				//fill in inode with stuff
				inode_pointer->i_mode = 0;
				inode_pointer->i_size = 0;
				//set bitmap to taken
				
		}
		else {
			return 0;
		}
		

	}
	return found_inode;
}

uint32_t add_to_dir(uint8_t *device, int dir_inode_number, int target_inode_number, char* name, uint8_t file_type) {
				struct ext2_inode * dir = get_inode(device,dir_inode_number);
				//create directory entry
				struct ext2_dir_entry_2 new_entry;
				new_entry.inode = target_inode_number;
				new_entry.name_len = (uint8_t) strlen(name);
				new_entry.file_type = file_type;
				strcpy(new_entry.name,name);
				new_entry.rec_len = (uint16_t) (sizeof(new_entry.inode) + sizeof(new_entry.name_len) + sizeof(new_entry.file_type) + strlen(name) + sizeof(uint16_t));

				int dir_size = ext2_get_file_size(dir_inode_number);
				void* loc = get_block(device,dir->i_block[0]);
				loc += dir_size;
				*(struct ext2_dir_entry_2 *)loc = new_entry;
				return 1;
}

