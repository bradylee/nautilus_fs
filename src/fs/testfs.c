#include <fs/testfs.h>
#include <fs/fs.h>
#define DEBUG(fmt, args...) printk("TESTING (DEBUG): " fmt "\n", ##args)

void run_all() {
	test_1();
	test_2();
	test_3();
	test_4();
	test_5();
	//test_6();
}

//open, then read more than length of file
void test_1() {
	int fn = open("/readme",O_RDWR);
	char* buf = malloc(15);
	ssize_t bytes = read(fn, buf, 15);
	if(bytes == 15 && !strcmp(buf,"hello world\n")) {
		DEBUG("Test 1: PASSED");
	}
	else {
		DEBUG("Test 1: FAILED");
	}
	free(buf);
}

//open, write, seek to beginning, then read more than length of file
void test_2() {
	int fn = open("/readme",O_RDWR);
	char* wr_buf = "adios";
	char* rd_buf = malloc(15);
	int wr_bytes = write(fn, wr_buf, 5);
	lseek(fn,0,0);
	int rd_bytes = read(fn, rd_buf, 15);
	if(rd_bytes == 15 && wr_bytes == 5 && !strcmp(rd_buf,"adios world\n")) {
		DEBUG("Test 2: PASSED");
	}
	else {
		DEBUG("Test 2: FAILED");
	}
	free(rd_buf);
}

//open, seek to end, write to end, seek to beginning, read all
void test_3() {
	int fn = open("/readme",O_RDWR);
	char* wr_buf = "adios";
	char* rd_buf = malloc(20);
	lseek(fn,0,2);
	int wr_bytes = write(fn, wr_buf, 5);
	lseek(fn,0,0);
	int rd_bytes = read(fn, rd_buf, 20); 
	if(rd_bytes == 20 && wr_bytes == 5 && !strcmp(rd_buf,"adios world\nadios")) {
		DEBUG("Test 3: PASSED");
	}
	else {
		DEBUG("Test 3: FAILED");
	}
	free(rd_buf);
}
//open a blank file, write to it, read it
void test_4() {
	int fn = open("/null",O_RDWR);
	char* wr_buf = "this used to be empty";
	char* rd_buf = malloc(21);
	int wr_bytes = write(fn, wr_buf, 21);
	lseek(fn,0,0);
	int rd_bytes = read(fn, rd_buf, 21); 
	if(rd_bytes == 21 && wr_bytes == 21 && !strcmp(rd_buf,"this used to be empty")) {
		DEBUG("Test 4: PASSED");
	}
	else {
		DEBUG("Test 4: FAILED");
	}
	free(rd_buf);
}
//open a file that doesnt exist, write to it, read it
void test_5() {
	int fn = open("/new_file",O_RDWR|O_CREAT);
	char* wr_buf = "this used to not exist";
	char* rd_buf = malloc(22);
	int wr_bytes = write(fn, wr_buf, 22);
	lseek(fn,0,0);
	int rd_bytes = read(fn, rd_buf, 22); 
	if(rd_bytes == 22 && wr_bytes == 22 && !strcmp(rd_buf,"this used to not exist")) {
		DEBUG("Test 5: PASSED");
	}
	else {
		DEBUG("Test 5: FAILED");
	}
	free(rd_buf);
}
//open a file that doesn't exist, write to it, delete it, create it again, read it
void test_6() {
	int fn = open("/new_file2",O_RDWR|O_CREAT);
	char* wr_buf = "this used to not exist";
	char* rd_buf = malloc(22);
	int wr_bytes = write(fn, wr_buf, 22);
	file_delete("/new_file2");
	signed int testfn = open("/new_file2",O_RDWR);
	DEBUG("HERE-- %x", testfn);
	fn = open("/new_file2",O_RDWR|O_CREAT);
	
	int rd_bytes = read(fn, rd_buf, 22); 
	if(rd_bytes == 0 && wr_bytes == 22 && strcmp(rd_buf,"this used to not exist") && testfn == -1) {
		DEBUG("Test 6: PASSED");
	}
	else {
		DEBUG("Test 6: FAILED");
	}
	free(rd_buf);
}


