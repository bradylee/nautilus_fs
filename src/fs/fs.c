#include <fs/fs.h>

#define INFO(fmt, args...) printk("FILESYSTEM: " fmt "\n", ##args)
#define DEBUG(fmt, args...) printk("FILESYSTEM (DEBUG): " fmt "\n", ##args)
#define ERROR(fmt, args...) printk("FILESYSTEM (ERROR): " fmt "\n", ##args)

#ifndef NAUT_CONFIG_DEBUG_FILESYSTEM
#undef DEBUG
#define DEBUG(fmt, args...)
#endif

void test_fs() {

	init_fs();
	DEBUG("Opening files...");
  char *buf;
	int fn;
	int num_parts = 0;
	char **parts = split_path("/readme", &num_parts);
	int i;
	DEBUG("NUM_PARTS %d", num_parts);
	for (i = 0; i < num_parts; i++) {
		DEBUG("Part %d: %s", i, parts[i]);
	}

	/*
	fn = open("/readme", O_RDWR);
	fn = open("/null", O_RDWR);
	fn = open("/nothing", O_RDWR);
	fn = open("/nothing", O_RDWR);
	DEBUG("Done opening");
	DEBUG("Printing...");
	iterate_opened(file_print);
	DEBUG("Done printing");
	DEBUG("Closing files...");
	iterate_opened(__close);
	DEBUG("Done closing");
	DEBUG("");
	*/

	/*
	buf = malloc(15);
	DEBUG("FILE CREATE TEST");
	char path[] = "/a";
	DEBUG("Creating file %d", file_create(path));
  DEBUG("Wrote %d", write(fn, "Testing file /a", 15));
	DEBUG("Seeking...");
  lseek(fn, 0, 0);
	DEBUG("Read %d", read(fn, buf, 15));
	int inum = get_inode_by_path(&RAMFS_START, path);
	DEBUG("Inode %d", inum); 
	DEBUG("Size %d", ext2_get_file_size(&RAMFS_START, inum)); 
	DEBUG("Done creating");
	DEBUG("");
	free(buf);
	*/

	DEBUG("FILE REMOVE TEST");
	DEBUG("%d", get_block_size(&RAMFS_START));
	uint32_t rootnum = get_inode_by_path(&RAMFS_START, "/");
	DEBUG("Root inode %d", rootnum);
	fn = open("/", 1);
	//size_t rootsize = ext2_get_directory_size(&RAMFS_START, rootnum);
	size_t rootsize = 1024;
	//DEBUG("Root dentry size %d", rootsize);
	buf = malloc(1024);
	DEBUG("Read %d", read(fn, buf, rootsize));
	char path[] = "/readme";
	DEBUG("Removing file %d", ext2_file_delete(&RAMFS_START, path));
	DEBUG("Removing file %d", ext2_file_delete(&RAMFS_START, path));
	//DEBUG("Root dentry size %d", ext2_get_directory_size(&RAMFS_START, rootnum));
	//DEBUG("Removing file %d", ext2_file_delete(&RAMFS_START, path));
	//DEBUG("Root dentry size %d", ext2_get_file_size(&RAMFS_START, rootnum));

	deinit_fs();
	DEBUG("Done");
}

void init_fs(void) {
	INFO("Initing...");
	INIT_LIST_HEAD(&open_files.head);
	spinlock_init(&open_files.lock);
	INFO("Done initing.");
}

void deinit_fs(void) {
	INFO("Deiniting...");
	iterate_opened(__close);
	spinlock_deinit(&open_files.lock);
	INFO("Done deiniting.");
}

void iterate_opened(void (*callback)(struct file*)) {
	struct list_head *cur;
	struct list_head *temp;
	struct file *fd;

	list_for_each_safe(cur, temp, &open_files.head) {
		fd = (struct file*)cur;
		callback(fd);
	}
}

void file_print(struct file* fd) {
	printk("%d %d\n", fd->filenum, fd->fileid);
}

void set_file_interface(struct file_int *fi, enum Filesystem fs) {
	if (fs == ext2) {
		fi->open = ext2_open;
		fi->read = ext2_read;
		fi->write = ext2_write;
		fi->get_size = ext2_get_file_size;
	}
}

int open(char *path, int access) {
	static uint32_t n = 1;

	spin_lock(&open_files.lock);

	struct file *fd = malloc(sizeof(struct file));

	// if filesystem of path is ext2 then...
	set_file_interface(&fd->interface, ext2);

	if (file_exists(path)) {
		fd->fileid = file_open(fd, path, access);
		//fd->fileid = fd->interface.open(&RAMFS_START, path, access);
	}
	else if (file_has_access(fd, O_WRONLY) && file_has_access(fd, O_CREAT)) {
		int id = file_create(path);
		if (!id)
			return -1;
		DEBUG("Created %s %d", path, id);
		fd->fileid = id;
	}
	else {
		return -1;
	}

	fd->filenum = n++; // allows file to be opened multiple times
	fd->access = access;
	fd->position = 0;

	// check already opened
	if (!get_open_file(fd->filenum)) { 
		list_add(&fd->file_node, &open_files.head);
		spinlock_init(&fd->lock);
		DEBUG("Opened %s %d %d", path, fd->filenum, fd->fileid);
	}

	spin_unlock(&open_files.lock);
	return fd->filenum;
}

void __close(struct file *fd) {
	spin_lock(&open_files.lock);
	spinlock_deinit(&fd->lock);
	list_del((struct list_head*)fd);
	free(fd);
	spin_unlock(&open_files.lock);
}

int close(uint32_t filenum) {
	struct file *fd = get_open_file(filenum);
	if (!fd) {
		return 0;
	}
	__close(fd);
	return 1;
}

struct file* get_open_file(uint32_t filenum) {
	struct list_head *cur;
	struct file *fd;

	list_for_each(cur, &open_files.head) {
		fd = (struct file*)cur;
		if (fd->filenum == filenum) {
			return fd;
		}
	}
	return NULL;
}

ssize_t read(int filenum, char *buf, size_t num_bytes) {
	struct file *fd = get_open_file(filenum);

	//  RDONLY or RDWR will return the same
	if (fd == NULL || !file_has_access(fd, O_RDONLY)) {
		return -1;
	}

	size_t n = file_read(fd, buf, num_bytes);
	//size_t n = fd->interface.read(&RAMFS_START, fd->fileid, buf, num_bytes, fd->position);
	fd->position += n;
	//printk("n: %d pos: %d\n", n, fd->position);
	return n;
}

ssize_t write(int filenum, char *buf, size_t num_bytes) {
	struct file *fd = get_open_file(filenum);

	//  WRONLY or RDWR will return the same
	if (fd == NULL || !file_has_access(fd, O_WRONLY)) {
		return -1;
	}
	if (file_has_access(fd, O_APPEND)) {
		__lseek(fd, 0, 2);
	}

	size_t n = file_write(fd, buf, num_bytes);
	fd->position += n;
	return n;
}

int file_has_access(struct file *fd, int access) {
	return ((fd->access & access) == access);
}

ssize_t __lseek(struct file *fd, size_t offset, int whence) {
	if(whence == 0) {
		fd->position = offset;
	}
	else if(whence == 1) {
		fd->position += offset;
	}
	else if(whence == 2) {
		size_t size = file_get_size(fd);
		//size_t size = fd->interface.get_size(&RAMFS_START, fd->fileid);
		//uint64_t size = ext2_get_file_size((uint32_t)fd->filenum);
		//printk("file size = %d\n",size);
		fd->position = size + offset-1;
	}
	else {
		return -1;
	}
	return fd->position;
}

//pos = 0 -> beginning of file, 1 -> current position, 2 -> end of file
ssize_t lseek(int filenum, size_t offset, int whence) {
	struct file *fd = get_open_file(filenum);
	if (fd == NULL) {
		return -1;
	}
	return __lseek(fd, offset, whence);
}

ssize_t tell(int filenum) {
	struct file *fd = get_open_file(filenum);
	if (fd == NULL) {
		return -1;
	}
	return fd->position; 
}

// returns 1 if file exists, 0 other
int file_exists(char *path) {
  return ext2_file_exists(&RAMFS_START,path);
}

// creates file with the given path
uint32_t file_create(char* path) {
  return ext2_file_create(&RAMFS_START, path);
}

void directory_list(char* path) {
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

