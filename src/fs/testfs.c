#include <fs/testfs.h>
#include <fs/fs.h>
#define DEBUG(fmt, args...) printk("TESTING (DEBUG): " fmt "\n", ##args)

void run_all() {
	test_1();
	test_2();
}

//open, then read more than length of file
void test_1() {
	int fn = open("/readme",O_RDWR);
	char* buf = malloc(15);
	ssize_t bytes = read(fn, buf, 15);
	DEBUG("Check: %d %d", bytes, fn);
	if(bytes == 11 && !strcmp(buf,"hello world")) {
		DEBUG("Test 1: PASSED");
	}
	else {
		DEBUG("Test 1: FAILED");
	}
}

//open, write, seek to beginning, then read more than length of file
void test_2() {
	int fn = open("/readme",O_RDWR);
	char* wr_buf = "adios";
	char* rd_buf = malloc(15);
	int wr_bytes = write(fn, wr_buf, 5);
	lseek(fn,0,0);
	int rd_bytes = read(fn, rd_buf, 15);
	if(rd_bytes == 11 && wr_bytes == 5 && !strcmp(rd_buf,"adios world")) {
		DEBUG("Test 2: PASSED");
	}
	else {
		DEBUG("Test 2: FAILED");
	}
}

//open, seek to end, write to end, seek to beginning, read all
