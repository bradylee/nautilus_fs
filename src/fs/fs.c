#include <fs/fs.h>

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
	fn = open("/readme", 1);
	fn = open("/null", 1);
	fn = open("/nothing", 1);
	fn = open("/nothing", 1);
	DEBUG("Done opening\n");
	DEBUG("Printing...\n");
	iterate_opened(file_print);
	DEBUG("Done printing\n");
	DEBUG("Closing files...\n");
	iterate_opened(__close);
	DEBUG("Done closing\n");

  printk("-----------------------------------\n");
  file_create("/a");
  fn = open("/a",O_RDWR);
  write(fn, "Testing file /a", 15);
  char *buf = malloc(15);
  lseek(fn,0,0);
  read(fn,buf,15);
  printk("buffer: %s size: %d\n",buf, ext2_get_file_size(get_inode_by_path(&RAMFS_START,"/a")));

	deinit_fs();
	printk("Done\n");
}

void init_fs(void) {
	INFO("Initing...\n");
	INIT_LIST_HEAD(&open_files.head);
	spinlock_init(&open_files.lock);
	INFO("Done initing.\n");
}

void deinit_fs(void) {
	INFO("Deiniting...\n");
	iterate_opened(__close);
	spinlock_deinit(&open_files.lock);
	INFO("Done deiniting.\n");
}

void iterate_opened(void (*callback)(struct file_data*)) {
	struct list_head *cur;
	struct list_head *temp;
	struct file_data *fd;

	list_for_each_safe(cur, temp, &open_files.head) {
		fd = (struct file_data*)cur;
		callback(fd);
	}
}

void file_print(struct file_data* fd) {
	printk("%d %d\n", fd->filenum, fd->fileid);
}

void file_set_interface(struct file_int *fi, enum Filesystem fs) {
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

	struct file_data *fd = malloc(sizeof(struct file_data));

	// if filesystem of path is ext2 then...
	file_set_interface(&fd->interface, ext2);

	if (file_exists(path)) {
		fd->fileid = fd->interface.open(&RAMFS_START, path, access);
	}
	else if (file_has_access(fd, O_WRONLY) && file_has_access(fd, O_CREAT)) {
		int id = file_create(path);
		if (!id)
			return -1;
		DEBUG("Created %s %d\n", path, id);
		fd->fileid = id;
	}
	else {
		return -1;
	}

	fd->filenum = n++; // allows file to be opened multiple times
	fd->access = access;
	fd->position = 0;

	// check already opened
	if (!file_get_open(fd->filenum)) { 
		list_add(&fd->file_node, &open_files.head);
		spinlock_init(&fd->lock);
		DEBUG("Opened %s %d %d\n", path, fd->filenum, fd->fileid);
	}

	spin_unlock(&open_files.lock);
	return fd->filenum;
}

void __close(struct file_data* fd) {
	spin_lock(&open_files.lock);
	spinlock_deinit(&fd->lock);
	list_del((struct list_head*)fd);
	free(fd);
	spin_unlock(&open_files.lock);
}

int close(uint32_t filenum) {
	struct file_data *fd = file_get_open(filenum);
	if (!fd) {
		return 0;
	}
	__close(fd);
	return 1;
}

struct file_data* file_get_open(uint32_t filenum) {
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

ssize_t read(int filenum, char *buf, size_t num_bytes) {
	struct file_data *target = file_get_open(filenum);

	//  RDONLY or RDWR will return the same
	if (target == NULL || !file_has_access(target, O_RDONLY)) {
		return -1;
	}

	size_t n = target->interface.read(target->fileid, buf, num_bytes, target->position);
	target->position += n;
	//printk("n: %d pos: %d\n", n, target->position);
	return n;
}

ssize_t write(int filenum, char *buf, size_t num_bytes) {
	struct file_data *target = file_get_open(filenum);

	//  WRONLY or RDWR will return the same
	if (target == NULL || !file_has_access(target, O_WRONLY)) {
		return -1;
	}
	if (file_has_access(target, O_APPEND)) {
		__lseek(target, 0, 2);
	}

	size_t n = target->interface.write(target->fileid, buf, num_bytes, target->position);
	target->position += n;
	//printk("n: %d pos: %d\n", n, target->position);
	return n;
}

int file_has_access(struct file_data *fd, int access) {
	return ((fd->access & access) == access);
}

ssize_t __lseek(struct file_data *target, size_t offset, int whence) {
	if(whence == 0) {
		target->position = offset;
	}
	else if(whence == 1) {
		target->position += offset;
	}
	else if(whence == 2) {
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
ssize_t lseek(int filenum, size_t offset, int whence) {
	struct file_data *target = file_get_open(filenum);
	if (target == NULL) {
		return -1;
	}
	return __lseek(target, offset, whence);
}

ssize_t tell(int filenum) {
	struct file_data *target = file_get_open(filenum);
	if (target == NULL) {
		return -1;
	}
	return target->position; 
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

