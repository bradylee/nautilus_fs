#include <fs/ext2/ext2.h>

#define INFO(fmt, args...) printk("EXT2: " fmt "\n", ##args)
#define DEBUG(fmt, args...) printk("EXT2 (DEBUG): " fmt "\n", ##args)
#define ERROR(fmt, args...) printk("EXT2 (ERROR): " fmt "\n", ##args)

#ifndef NAUT_CONFIG_DEBUG_FILESYSTEM
#undef DEBUG
#define DEBUG(fmt, args...)
#endif

int ext2_open(uint8_t *device, char *path, int access) {
	uint32_t inode_num = (uint32_t)get_inode_by_path(device, path);
	return inode_num;
}

ssize_t ext2_read(uint8_t *device, int inode_number, char *buf, size_t num_bytes,size_t offset) {
	struct ext2_inode *inode_pointer = get_inode(device, inode_number);
	unsigned char *blocks = (char *)(get_block(device, inode_pointer->i_block[0]));
	int i = 0;	
	uint8_t * cur = blocks + offset;
	while (i < num_bytes && *cur != 0x0a) {		
		buf[i] = *cur;		
		i++;
		cur++;
	}
	buf[i] = 0x00; //null terminator
	return i;
}

ssize_t ext2_write(uint8_t *device, int inode_number, char *buf, size_t num_bytes, size_t offset) {
	struct ext2_inode *inode_pointer = get_inode(device, inode_number);
	unsigned char *blocks = (char*)(get_block(device, inode_pointer->i_block[0]));
	int i = 0;	
	int end_flag = 0;
	int end_bytes = 0;
	uint8_t * cur = blocks + offset;

	if(ext2_get_file_size(device, inode_number) == 0) {
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
	DEBUG("Writing reached end %d", end_flag);
	if(end_flag) {
		*cur = 0x0A;
		inode_pointer->i_size += end_bytes;
	}
	return i;
}

size_t ext2_get_size(uint8_t *device, int inum) {
	struct ext2_inode *inode = get_inode(device, inum);	
	if (inode->i_mode == EXT2_S_IFREG)
		return ext2_get_file_size(device, inum);
	if (inode->i_mode == EXT2_S_IFDIR)
		return ext2_get_dir_size(device, inum);
	return 0;
}

size_t ext2_get_file_size(uint8_t *device, int inum) {
	struct ext2_inode *inode = get_inode(device, inum);	
	uint64_t temp = inode->i_size_high;
	temp = temp << 32;
	temp = temp | (inode->i_size);
	return temp;
}

size_t ext2_get_dir_size(uint8_t *device, int inum) {
	struct ext2_inode *inode = get_inode(device, inum);	
	void *block = get_block(device, inode->i_block[0]);
	struct ext2_dir_entry_2 *dentry = (struct ext2_dir_entry_2 *)block;
	int count = 0;
	//while (dentry->inode) {
	while (++count < 15) {
		DEBUG("Dir Remove: dentry %p, inode %d, length %d", dentry, dentry->inode, dentry->rec_len);
		dentry = (struct ext2_dir_entry_2*)((uint8_t*)dentry + dentry->rec_len);
	}
	size_t size = (uint64_t)dentry - (uint64_t)block; 
	DEBUG("Dir %d size is %d", inum, size);
	return size;
}

int ext2_file_exists(uint8_t *device, char *path) {
	uint32_t inode_num = (uint32_t)get_inode_by_path(device, path);
	return inode_num != 0;
}

// return new inode number; 0 if failed
uint32_t ext2_file_create(uint8_t *device, char *path) {
	uint32_t free_inode = alloc_inode(device);
	DEBUG("Allocated %d", free_inode);

	if (free_inode) {
		struct ext2_inode *inode_pointer = get_inode(device, free_inode);

		// get directory path
		int num_parts = 0;
		char **parts = split_path(path, &num_parts);
		if (!num_parts) {
			return 0;
		}
		int newstring_size = 0;
		for (int i=0; i < num_parts-1; i++) {
			newstring_size += strlen(parts[i]) + 1;
		}
		char *newstring = malloc(newstring_size);
		strcpy(newstring, "/");
		for (int i=0; i < num_parts-1; i++) {
			strcat(newstring,parts[i]);
			if (i != num_parts-2) {
				strcat(newstring, "/");
			}
		}

		// get inode of directory
		char *name = parts[num_parts-1];
		int dir_num = 0;
		if(strcmp(newstring, "/")) {
			dir_num = get_inode_by_path(device, newstring);
		}
		else {
			dir_num = EXT2_ROOT_INO;
		}

		// create dentry
		if (dir_num) {
			ext2_dir_add_file(device, dir_num, free_inode, name, 1);
			//fill in inode with stuff
			inode_pointer->i_mode = EXT2_S_IFREG;
			inode_pointer->i_size = 0;
			//set bitmap to taken
		}
		else {
			return 0;
		}
	}
	return free_inode;
}

int ext2_inode_has_mode(struct ext2_inode *inode, int mode) {
	return ((inode->i_mode & mode) == mode);
}

int ext2_file_delete(uint8_t *device, char *path) {
	uint32_t inum = get_inode_by_path(device, path);
	if (!inum) {
		DEBUG("Bad path");
		return 0;
	}
	struct ext2_inode *inode = get_inode(device, inum);
	// check if file
	if (!ext2_inode_has_mode(inode, EXT2_S_IFREG)) {
		DEBUG("FILE DELETE: Not a file %x", inode->i_mode);
		return 0;
	}

	// get directory path
	int num_parts = 0;
	char **parts = split_path(path, &num_parts);
	if (!num_parts) {
		DEBUG("No parts");
		return 0;
	}
	int newstring_size = 0;
	for (int i=0; i < num_parts-1; i++) {
		newstring_size += strlen(parts[i]) + 1;
	}
	char *newstring = malloc(newstring_size);
	strcpy(newstring, "/");
	for (int i=0; i < num_parts-1; i++) {
		strcat(newstring,parts[i]);
		if (i != num_parts-2) {
			strcat(newstring, "/");
		}
	}

	// get inode of directory
	int dir_num = 0;
	if(strcmp(newstring, "/")) {
		dir_num = get_inode_by_path(device, newstring);
	}
	else {
		dir_num = EXT2_ROOT_INO;
	}

	// create dentry
	if (dir_num) {
		ext2_dir_remove_file(device, dir_num, inum);
	}
	else {
		DEBUG("Bad dir path");
		return 0;
	}

	return 1;
}

uint16_t ext2_dentry_find_len(struct ext2_dir_entry_2 *dentry) {
	uint16_t len = (uint16_t)(sizeof(dentry->inode) + sizeof(dentry->name_len) + sizeof(dentry->file_type) + dentry->name_len + sizeof(uint16_t));
	DEBUG("Dentry Find Len: %s", dentry->name);
	return (len += (DENTRY_ALIGN - len % DENTRY_ALIGN));
}

int ext2_dir_add_file(uint8_t *device, int dir_inum, int target_inum, char *name, uint8_t file_type) {
	struct ext2_inode *dir = get_inode(device,dir_inum);

	//create directory entry
	struct ext2_dir_entry_2 new_entry;
	new_entry.inode = target_inum;
	new_entry.name_len = (uint8_t) strlen(name);
	new_entry.file_type = file_type;
	strcpy(new_entry.name, name);
	new_entry.rec_len = (uint16_t)(sizeof(new_entry.inode) + sizeof(new_entry.name_len) + sizeof(new_entry.file_type) + strlen(name) + sizeof(uint16_t));
	DEBUG("Dir Add: %s, reclen %d", new_entry.rec_len);

	size_t dir_size = ext2_get_dir_size(device, dir_inum);
	void *block = get_block(device, dir->i_block[0]) + dir_size;
	*(struct ext2_dir_entry_2 *)block = new_entry;
	dir->i_size += dir_size; 
	return 1;
}

int ext2_dir_remove_file(uint8_t *device, int dir_inum, int target_inum) {
	struct ext2_inode *dir = get_inode(device, dir_inum);
	void *block = get_block(device, dir->i_block[0]);
	ssize_t blocksize = get_block_size(device);

	// find target dentry
	struct ext2_dir_entry_2 *dentry = (struct ext2_dir_entry_2 *)block;
	struct ext2_dir_entry_2 *target = NULL;
	struct ext2_dir_entry_2 *prev = NULL;
	int count = 0;
	blocksize -= dentry-> rec_len;
	while (blocksize > 0) { 
		prev = dentry;
		dentry = (struct ext2_dir_entry_2*)((uint8_t*)dentry + dentry->rec_len);
		blocksize -= dentry->rec_len;
		DEBUG("Dir Remove: dentry %p, inode %d, length %d, lenex %d, cursize %d", dentry, dentry->inode, dentry->rec_len, ext2_dentry_find_len(dentry), blocksize);
		if (dentry->inode == target_inum) {
			target = dentry;
			DEBUG("Dir Remove: Found dentry");
		}
	}
	if (blocksize) {
		ERROR("DIR REMOVE FILE: rec_len values not aligned %d", blocksize);
	}
	DEBUG("Dir Remove: dentry %p, inode %d, length %d, lenex %d, cursize %d", dentry, dentry->inode, dentry->rec_len, ext2_dentry_find_len(dentry), blocksize);
	int is_target_end = 0;
	if (target == dentry) {
		is_target_end = 1;
		DEBUG("DIR REMOVE: End target");
	}
	DEBUG("Dir Remove: size %d", blocksize);
	if (target == NULL) {
		ERROR("Dir Remove: Did not find dentry!");
		return 0;
	}
	size_t dir_size = (uint64_t)dentry - (uint64_t)block + ext2_dentry_find_len(dentry); 
	DEBUG("Dir Remove: last reclen %d", ext2_dentry_find_len(dentry)); 
	DEBUG("Dir Remove: dir_size %d, %x", dir_size, dir_size); 

	if (is_target_end) {
		prev->rec_len += target->rec_len;
	}
	else {
		// move remaining dentries back
		dentry->rec_len += target->rec_len;
		void *move_dst = (void*)((uint8_t*)target);
		void *move_src = (void*)((uint8_t*)target + target->rec_len);
		size_t move_len = (uint64_t)block + dir_size - (uint64_t)move_src;
		DEBUG("Dir Remove: move %p %p", move_dst, move_src);
		DEBUG("Dir Remove: move len %d", move_len);
		memmove(move_dst, move_src, move_len);
	}

	// update terminator
	dir_size -= target->rec_len;
	dir->i_size -= dir_size;
	free_inode(device, target_inum);
	return 1;
}
