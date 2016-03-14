#include <fs/ext2/ext2.h>

int ext2_open(uint8_t *device, char *path, int access) {
	uint32_t inode_num = (uint32_t) get_inode_by_path(device, path);
	return inode_num;
}

ssize_t ext2_read(int inode_number, char *buf, size_t num_bytes,size_t offset) {
	struct ext2_inode * inode_pointer = get_inode(&RAMFS_START, inode_number);
	unsigned char * blocks = (char *)(get_block(&RAMFS_START, inode_pointer->i_block[0]));
	int i = 0;	
	uint8_t * cur = blocks + offset;
	while (i<num_bytes && *cur != 0x0a) {		
		buf[i] = *cur;		
		i++;
		cur++;
	}
	buf[i] = 0x00; //null terminator
	return i;
}

ssize_t ext2_write(int inode_number, char *buf, size_t num_bytes, size_t offset) {
	struct ext2_inode * inode_pointer = get_inode(&RAMFS_START, inode_number);
	unsigned char * blocks = (char *)(get_block(&RAMFS_START, inode_pointer->i_block[0]));
	int i = 0;	
	int end_flag = 0;
	int end_bytes = 0;
	uint8_t * cur = blocks + offset;

	if(ext2_get_file_size(inode_number) == 0) {
		end_flag = 1;
		end_bytes = num_bytes;
	}
	while (i<num_bytes) {		
		if(*cur == 0x0A) {
			end_flag = 1;
			end_bytes = num_bytes-i;
		}
		*cur = buf[i];		
		i++;
		cur++;
	}
	printk("end: %d\n",end_flag);
	if(end_flag) {
		*cur = 0x0A;
		inode_pointer->i_size += end_bytes;
	}
	return i;
}

uint64_t ext2_get_file_size(int inode_number) {
	struct ext2_inode * inode_pointer = get_inode(&RAMFS_START, inode_number);	
	uint64_t temp = inode_pointer->i_size_high;
	temp = temp << 32;
	temp = temp | (inode_pointer->i_size);
	return temp;
}

int ext2_file_exists(uint8_t *device, char *path) {
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
			directory_add_file(device, dir_num, found_inode, name, 1);
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

/*
	 uint32_t ext2_file_delete(uint8_t *device, char *path {
	 }
	 */

uint32_t directory_add_file(uint8_t *device, int dir_inode_number, int target_inode_number, char* name, uint8_t file_type) {
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

