#include <fs/vfs.h>

#define INFO(fmt, args...) printk("FILESYSTEM: " fmt, ##args)
#define DEBUG(fmt, args...) printk("FILESYSTEM (DEBUG): " fmt, ##args)
#define ERROR(fmt, args...) printk("FILESYSTEM (ERROR): " fmt, ##args)

#ifndef NAUT_CONFIG_DEBUG_FILESYSTEM
#undef DEBUG
#define DEBUG(fmt, args...)
#endif

void test_fs() {
	init_fs();
	DEBUG("Opening files...\n");
	uint32_t fn;
	fn = file_open("/readme", 1);
	fn = file_open("/null", 1);
	fn = file_open("/nothing", 1);
	fn = file_open("/nothing", 1);
	DEBUG("Done opening\n");
	DEBUG("Printing...\n");
	__iterate_opened(__file_print);
	DEBUG("Done printing\n");
	DEBUG("Closing files...\n");
	__iterate_opened(__file_close);
	DEBUG("Done closing\n");
	deinit_fs();
	DEBUG("Done\n");
}

void init_fs(void) {
	INFO("Initing...\n");
	INIT_LIST_HEAD(&open_files.head);
	spinlock_init(&open_files.lock);
	INFO("Done initing.\n");
}

void deinit_fs(void) {
	INFO("Deiniting...\n");
	__iterate_opened(__file_close);
	spinlock_deinit(&open_files.lock);
	INFO("Done deiniting.\n");
}

void __iterate_opened(void (*callback)(struct file_data*)) {
	struct list_head *cur;
	struct list_head *temp;
	struct file_data *fd;

	list_for_each_safe(cur, temp, &open_files.head) {
		fd = (struct file_data*)cur;
		callback(fd);
	}
}

void __file_print(struct file_data* fd) {
	printk("%d %d\n", fd->filenum, fd->fileid);
}

size_t file_open(char *path, int access) {
	static uint32_t n = 1;

	spin_lock(&open_files.lock);

	struct file_data *fd = malloc(sizeof(struct file_data));
	//uint32_t inode_num = ext2_open(&RAMFS_START, path, access);

	// if filetype ext2 then...
	ext2_set_file_op(fd->op);
	//fd->op = ext2_file_options;
	//fd->op->open = ext2_open;
	//fd->op->read = ext2_read;
	//fd->op->write = ext2_write;

	fd->filenum = n++; // allows file to be opened multiple times
	fd->fileid = fd->op->open(&RAMFS_START, path, access);
	fd->access = access;

	// check already opened
	if (!__get_open_file(fd->filenum)) { 
		list_add(&fd->file_node, &open_files.head);
		spinlock_init(&fd->lock);
		DEBUG("Opened %s %d %d\n", path, fd->filenum, fd->fileid);
	}

	spin_unlock(&open_files.lock);

	return fd->filenum;
}

void __file_close(struct file_data* fd) {
	spin_lock(&open_files.lock);
	spinlock_deinit(&fd->lock);
	list_del((struct list_head*)fd);
	free(fd);
	spin_unlock(&open_files.lock);
}

int file_close(uint32_t filenum) {
	struct file_data *fd = __get_open_file(filenum);
	if (!fd) {
		return 0;
	}
	__file_close(fd);
	return 1;
}

struct file_data* __get_open_file(uint32_t filenum) {
	struct list_head *cur;
	struct file_data *fd;

	list_for_each(cur, &open_files.head) {
		fd = (struct file_data*)cur;
		if (fd->filenum == filenum) {
			return fd;
		}
	}
	return NULL;
}

size_t file_read(int filenum, char *buf, size_t num_bytes) {
	struct file_data *target = __get_open_file(filenum);

	//  RDONLY or RDWR will return the same
	if (target == NULL || !__file_has_access(target, O_RDONLY)) {
		return -1;
	}

	size_t n = target->op->read(target->fileid, buf, num_bytes, target->position);
	target->position += n;
	//printk("n: %d pos: %d\n", n, target->position);
	return n;
}

size_t file_write(int filenum, char *buf, size_t num_bytes) {
	struct file_data *target = __get_open_file(filenum);

	//  WRONLY or RDWR will return the same
	if (target == NULL || !__file_has_access(target, O_WRONLY)) {
		return -1;
	}
	if (__file_has_access(target, O_APPEND)) {
		__file_seek(target, 0, 2);
	}

	size_t n = target->op->write(target->fileid, buf, num_bytes, target->position);
	target->position += n;
	//printk("n: %d pos: %d\n", n, target->position);
	return n;
}

int __file_has_access(struct file_data *fd, int access) {
	return ((fd->access & access) == access);
}

size_t __file_seek(struct file_data *target, size_t offset, int pos) {
	if(pos == 0) {
		target->position = offset;
	}
	else if(pos == 1) {
		target->position += offset;
	}
	else if(pos == 2) {
		uint64_t size = ext2_get_file_size((uint32_t)target->filenum);
		//printk("file size = %d\n",size);
		target->position = size + offset-1;
	}
	else {
		return -1;
	}
	return target->position;
}

//pos = 0 -> beginning of file, 1 -> current position, 2 -> end of file
size_t file_seek(int filenum, size_t offset, int pos) {
	struct file_data *target = __get_open_file(filenum);
	if (target == NULL) {
		return -1;
	}
	return __file_seek(target, offset, pos);
}

/*
	 size_t file_append(int filenum,char * write_data, size_t num_bytes) {
	 struct file_data *target = __get_open_file(filenum);
	 uint32_t inode_num = target->filenum;
	 size_t n = file_seek(filenum,0,2);
	 n = ext2_write(inode_num, write_data, num_bytes,target->position);
	 target->position += n;
	 printk("n: %d pos: %d\n", n, target->position);
	 return n;
	 }
	 */

void dir_ls(char* path) {
	char* tempname;
	struct ext2_dir_entry_2* directory_entry;
	struct ext2_inode * dir = get_inode(&RAMFS_START, get_inode_by_path(&RAMFS_START,path));
	//get pointer to block where directory entries reside using inode of directory
	directory_entry = (struct ext2_dir_entry_2*)get_block(&RAMFS_START,dir->i_block[0]);
	//scan only valid files in directory
	while(directory_entry->inode != 0){
		//create temporary name
		tempname = (char*)malloc(directory_entry->name_len*sizeof(char));
		int i;
		//copy file name to temp name, then null terminate it
		for(i = 0; i < directory_entry->name_len; i++){
			tempname[i] = directory_entry->name[i];
		}
		tempname[i] = '\0';
		if(strcmp(tempname,".") && strcmp(tempname,"..")) { //strcmp -> 0 = equal, !0 = not equal
			printk("%s\n", tempname);
		}
		directory_entry = (struct ext2_dir_entry_2*)((void*)directory_entry+directory_entry->rec_len);
		free(tempname);
	}
}

