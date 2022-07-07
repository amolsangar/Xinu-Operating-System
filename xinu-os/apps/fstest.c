#include <xinu.h>
#include <fs.h>
#include <string.h>

#ifdef FS

/**
 * TEST
 * MACRO to run a function and print whether it was successful or not
 */
#define TEST(function)                                         \
  if (nargs == 1 || strcmp(args[1], #function) == 0) {         \
    if ((function)() != SYSERR) {                              \
      printf("%-35s: [\033[32mPASS\033[39m]\n", #function);    \
    } else {                                                   \
      printf("%-35s: [\033[31mFAIL\033[39m]\n", #function);    \
    }                                                          \
  }


/* #define FSTEST_DEBUG */

/**
 * MACROs for unit testing
 */

/**
 * ASSERT_TEST
 * Run a function and report the return code
 */
#define ASSERT_TEST(function)                             printf("\033[33mTEST  %20s:%-3d %30s:\033[39m %d\n", __FILE__, __LINE__, #function, (function));

#ifdef FSTEST_DEBUG

#define ASSERT_TRUE(expr)     if (!(expr))              { printf("\033[31mERROR %20s:%-3d %30s()\033[39m '%s' != TRUE\n", __FILE__, __LINE__, __func__, #expr);       return SYSERR; }
#define ASSERT_PASS(function) if ((function) == SYSERR) { printf("\033[31mERROR %20s:%-3d %30s()\033[39m '%s' == SYSERR\n", __FILE__, __LINE__, __func__, #function); return SYSERR; }
#define ASSERT_FAIL(function) if ((function) != SYSERR) { printf("\033[31mERROR %20s:%-3d %30s()\033[39m '%s' != SYSERR\n", __FILE__, __LINE__, __func__, #function); return SYSERR; }

#else

/**
 * ASSERT_TRUE
 * Evaluate an expression and return SYSERR if false
 */
#define ASSERT_TRUE(expr)     if (!(expr))              { return SYSERR; }

/**
 * ASSERT_PASS
 * Run a function and return SYSERR if its return code is SYSERR
 */
#define ASSERT_PASS(function) if ((function) == SYSERR) { return SYSERR; }

/**
 * ASSERT_FAIL
 * Run a function and return SYSERR if its return code is not SYSERR
 */
#define ASSERT_FAIL(function) if ((function) != SYSERR) { return SYSERR; }

#endif


int fstest_testbitmask(void) {

  bs_mkdev(0, MDEV_BLOCK_SIZE, MDEV_NUM_BLOCKS);
  fs_mkfs(0, DEFAULT_NUM_INODES);

  fs_setmaskbit(31);
  fs_setmaskbit(95);
  fs_setmaskbit(159);
  fs_setmaskbit(223);
  fs_setmaskbit(287);
  fs_setmaskbit(351);
  fs_setmaskbit(415);
  fs_setmaskbit(479);
  fs_setmaskbit(90);
  fs_setmaskbit(154);
  fs_setmaskbit(218);
  fs_setmaskbit(282);
  fs_setmaskbit(346);
  fs_setmaskbit(347);
  fs_setmaskbit(348);
  fs_setmaskbit(349);
  fs_setmaskbit(350);
  fs_setmaskbit(100);
  fs_setmaskbit(164);
  fs_setmaskbit(228);
  fs_setmaskbit(292);
  fs_setmaskbit(356);
  fs_setmaskbit(355);
  fs_setmaskbit(354);
  fs_setmaskbit(353);
  fs_setmaskbit(352);

  fs_printfreemask();

  fs_clearmaskbit(31);
  fs_clearmaskbit(95);
  fs_clearmaskbit(159);
  fs_clearmaskbit(223);
  fs_clearmaskbit(287);
  fs_clearmaskbit(351);
  fs_clearmaskbit(415);
  fs_clearmaskbit(479);
  fs_clearmaskbit(90);
  fs_clearmaskbit(154);
  fs_clearmaskbit(218);
  fs_clearmaskbit(282);
  fs_clearmaskbit(346);
  fs_clearmaskbit(347);
  fs_clearmaskbit(348);
  fs_clearmaskbit(349);
  fs_clearmaskbit(350);
  fs_clearmaskbit(100);
  fs_clearmaskbit(164);
  fs_clearmaskbit(228);
  fs_clearmaskbit(292);
  fs_clearmaskbit(356);
  fs_clearmaskbit(355);
  fs_clearmaskbit(354);
  fs_clearmaskbit(353);
  fs_clearmaskbit(352);

  fs_printfreemask();

  fs_freefs(0);
  bs_freedev(0);

  return OK;
}

/**
 * Try to
 * - (re-)create and free the block device,
 * - (re-)create and free the file system, and
 * - create your first file "test"
 */
int fstest_mkdev() {

  int i;

  for (i = 0; i < 1; i++) {
    ASSERT_PASS(bs_mkdev(0, MDEV_BLOCK_SIZE, MDEV_NUM_BLOCKS))
    ASSERT_PASS(fs_mkfs(0, DEFAULT_NUM_INODES))
    ASSERT_PASS(fs_create("test", O_CREAT))
    ASSERT_PASS(fs_freefs(0))
    ASSERT_PASS(bs_freedev(0))
  }

  return OK;
}
#endif

// =====================================================================
int fstest_create_multiple() {
  int i;
  for (i = 0; i < 1; i++) {
    ASSERT_PASS(bs_mkdev(0, MDEV_BLOCK_SIZE, MDEV_NUM_BLOCKS))
    ASSERT_PASS(fs_mkfs(0, DEFAULT_NUM_INODES))
    ASSERT_PASS(fs_create("test", O_CREAT))
    ASSERT_PASS(fs_open("test2", O_CREAT))
    ASSERT_PASS(fs_create("test3", O_CREAT))
    ASSERT_PASS(fs_freefs(0))
    ASSERT_PASS(bs_freedev(0))
  }

  return OK;
}

// =====================================================================
int fstest_close() {
  int i;
  for (i = 0; i < 1; i++) {
    ASSERT_PASS(bs_mkdev(0, MDEV_BLOCK_SIZE, MDEV_NUM_BLOCKS))
    ASSERT_PASS(fs_mkfs(0, DEFAULT_NUM_INODES))
    
    int fd = fs_create("test", O_CREAT);
    ASSERT_PASS(fs_close(fd))
    ASSERT_PASS(fs_freefs(0))
    ASSERT_PASS(bs_freedev(0))
  }

  return OK;
}

// =====================================================================
int fs_write_max_then_read() {
  int i;
  char *buf1, *buf2;
  int buf_size = 5120;
  int fd;

  buf1 = getmem(sizeof(char) * buf_size);
  buf2 = getmem(sizeof(char) * buf_size);

  for (i = 0; i < buf_size; i++) {
    buf1[i] = (char) i;
    buf2[i] = (char) 0;
  }

  ASSERT_PASS(bs_mkdev(0, MDEV_BLOCK_SIZE, MDEV_NUM_BLOCKS))
  ASSERT_PASS(fs_mkfs(0, DEFAULT_NUM_INODES))

  ASSERT_PASS(fd = fs_create("file", O_CREAT))

  ASSERT_TRUE(fs_write(fd, buf1, buf_size) == buf_size)
  printf("AFTER WRITE\n");
  fs_print_inode(fd);
  fs_print_oft();

  ASSERT_PASS(fs_seek(fd, 0))
  fs_print_inode(fd);
  fs_print_oft();
  
  ASSERT_TRUE(fs_read(fd, buf2, buf_size) == buf_size)
  printf("AFTER READ\n");
  fs_print_inode(fd);
  fs_print_oft();

  for (i = 0; i < buf_size; i++) {
    // printf("%d-",buf1[i]);
    // printf("%d\n",buf2[i]);
    ASSERT_TRUE(buf1[i] == buf2[i])
  }

  ASSERT_PASS(fs_close(fd))

  ASSERT_PASS(freemem(buf1, sizeof(char) * buf_size))
  ASSERT_PASS(freemem(buf2, sizeof(char) * buf_size))

  ASSERT_PASS(fs_freefs(0));
  ASSERT_PASS(bs_freedev(0));
}

// =====================================================================
int fs_write_max_plus_one() {
  int i;
  char *buf1, *buf2, *buf3;
  int buf_size = 5120;
  int fd;

  buf1 = getmem(sizeof(char) * buf_size);
  buf2 = getmem(sizeof(char) * buf_size);
  buf3 = getmem(sizeof(char) * 1);

  for (i = 0; i < buf_size; i++) {
    buf1[i] = (char) i;
    buf2[i] = (char) 0;
  }

  buf3[0] = (char) 0;

  ASSERT_PASS(bs_mkdev(0, MDEV_BLOCK_SIZE, MDEV_NUM_BLOCKS))
  ASSERT_PASS(fs_mkfs(0, DEFAULT_NUM_INODES))

  ASSERT_PASS(fd = fs_create("file", O_CREAT))

  ASSERT_TRUE(fs_write(fd, buf1, buf_size) == buf_size)
  fs_print_inode(fd);
  fs_print_oft();
  ASSERT_TRUE(fs_write(fd, buf3, 1) == 0)
  ASSERT_PASS(fs_seek(fd, 0))
  ASSERT_TRUE(fs_read(fd, buf2, buf_size) == buf_size)

  for (i = 0; i < buf_size; i++) {
    ASSERT_TRUE(buf1[i] == buf2[i])
  }

  ASSERT_PASS(fs_close(fd))

  ASSERT_PASS(freemem(buf1, sizeof(char) * buf_size))
  ASSERT_PASS(freemem(buf2, sizeof(char) * buf_size))

  ASSERT_PASS(fs_freefs(0));
  ASSERT_PASS(bs_freedev(0));
}

// =====================================================================
int fs_write_chunks_then_read() {
  int i;
  char *buf1, *buf2, *buf3, *buf4;
  int buf_size = 150;
  int fd;

  buf1 = getmem(sizeof(char) * buf_size);
  buf2 = getmem(sizeof(char) * buf_size);
  buf3 = getmem(sizeof(char) * 100);
  buf4 = getmem(sizeof(char) * 100);

  for (i = 0; i < buf_size; i++) {
    buf1[i] = (char)i;
    buf2[i] = (char)0;
  }

  for (i = 50; i < 150; i++) {
    buf3[i-50] = (char)i;
  }
  for (i = 0; i < 100; i++) {
    buf4[i] = (char)i;
  }

  ASSERT_PASS(bs_mkdev(0, MDEV_BLOCK_SIZE, MDEV_NUM_BLOCKS))
  ASSERT_PASS(fs_mkfs(0, DEFAULT_NUM_INODES))

  ASSERT_PASS(fd = fs_create("file2", O_CREAT))

  printf("INITIAL INODE\n");
  fs_print_inode(fd);

  ASSERT_TRUE(fs_write(fd, buf4, 100) == 100)  
  printf("AFTER WRITE\n");
  fs_print_inode(fd);
  fs_print_oft();
  
  ASSERT_PASS(fs_seek(fd, 50))
  printf("AFTER SEEK\n");
  fs_print_inode(fd);

  ASSERT_TRUE(fs_write(fd, buf3, 100) == 100)
  fs_print_inode(fd);
  
  ASSERT_PASS(fs_seek(fd, 0))
  printf("AFTER SEEK\n");
  fs_print_inode(fd);
  
  ASSERT_TRUE(fs_read(fd, buf2, buf_size) == buf_size)
  printf("AFTER READ\n");
  fs_print_inode(fd);
  fs_print_oft();
  fs_printfreemask();

  for (i = 0; i < buf_size; i++) {
    // printf("%d-",buf1[i]);
    // printf("%d\n",buf2[i]);
  }
  for (i = 0; i < buf_size; i++) {
    ASSERT_TRUE(buf1[i] == buf2[i])
  }

  ASSERT_PASS(fs_close(fd))

  ASSERT_PASS(freemem(buf1, sizeof(char) * buf_size))
  ASSERT_PASS(freemem(buf2, sizeof(char) * buf_size))

  ASSERT_PASS(fs_freefs(0));
  ASSERT_PASS(bs_freedev(0));
}

// =====================================================================
int fs_write_then_read_chunks() {
  int i;
  char *buf1, *buf2, *buf3, *buf4;
  int buf_size = 200;
  int fd;

  buf1 = getmem(sizeof(char) * buf_size);
  buf2 = getmem(sizeof(char) * buf_size);
  buf3 = getmem(sizeof(char) * 100);
  buf4 = getmem(sizeof(char) * 100);

  for (i = 0; i < buf_size; i++) {
    buf1[i] = (char)i;
    buf2[i] = (char)0;
  }

  for (i = 0; i < 100; i++) {
    buf3[i] = (char)0;
    buf4[i] = (char)0;
  }
  
  ASSERT_PASS(bs_mkdev(0, MDEV_BLOCK_SIZE, MDEV_NUM_BLOCKS))
  ASSERT_PASS(fs_mkfs(0, DEFAULT_NUM_INODES))

  ASSERT_PASS(fd = fs_create("file2", O_CREAT))

  printf("INITIAL INODE\n");
  fs_print_inode(fd);

  ASSERT_TRUE(fs_write(fd, buf1, buf_size) == buf_size)  
  printf("AFTER WRITE\n");
  fs_print_inode(fd);
  fs_print_oft();
  
  ASSERT_PASS(fs_seek(fd, 0))
  printf("AFTER SEEK\n");
  fs_print_inode(fd);
  
  ASSERT_TRUE(fs_read(fd, buf3, 100) == 100)
  printf("AFTER READ 1\n");
  fs_print_inode(fd);
  fs_print_oft();
  fs_printfreemask();

  ASSERT_TRUE(fs_read(fd, buf4, 100) == 100)
  printf("AFTER READ 1\n");
  fs_print_inode(fd);
  fs_print_oft();
  fs_printfreemask();

  // for (i = 0; i < 100; i++) {
  //   printf("%d\n",buf3[i]);
  // }
  // for (i = 0; i < 100; i++) {
  //   printf("%d\n",buf4[i]);
  // }
  memcpy(buf2,buf3,100);
  memcpy(buf2+100,buf4,100);
  for (i = 0; i < buf_size; i++) {
    printf("%d-",buf1[i]);
    printf("%d\n",buf2[i]);
    ASSERT_TRUE(buf1[i] == buf2[i])
  }

  ASSERT_PASS(fs_close(fd))

  ASSERT_PASS(freemem(buf1, sizeof(char) * buf_size))
  ASSERT_PASS(freemem(buf2, sizeof(char) * buf_size))

  ASSERT_PASS(fs_freefs(0));
  ASSERT_PASS(bs_freedev(0));
}

// =====================================================================
int fs_write_read() {
  int i;
  char *buf1, *buf2, *buf3, *buf4;
  int buf_size = 200;
  int fd;

  buf1 = getmem(sizeof(char) * buf_size);
  buf2 = getmem(sizeof(char) * buf_size);
  buf3 = getmem(sizeof(char) * 50);
  // buf4 = getmem(sizeof(char) * 200);

  for (i = 0; i < buf_size; i++) {
    buf1[i] = (char)i;
    buf2[i] = (char)0;
  }

  for (i = 100; i < 150; i++) {
    buf3[i-100] = (char)i;
  }
  // for (i = 0; i < 200; i++) {
  //   buf4[i] = (char)i;
  // }

  ASSERT_PASS(bs_mkdev(0, MDEV_BLOCK_SIZE, MDEV_NUM_BLOCKS))
  ASSERT_PASS(fs_mkfs(0, DEFAULT_NUM_INODES))

  ASSERT_PASS(fd = fs_create("file2", O_CREAT))

  printf("BEFORE WRITE\n");
  fs_print_inode(fd);

  ASSERT_TRUE(fs_write(fd, buf1, buf_size) == buf_size)
  // ASSERT_TRUE(fs_write(fd, buf4, 200) == 200)
  
  printf("AFTER WRITE\n");
  fs_print_inode(fd);
  fs_print_oft();

  ASSERT_PASS(fs_seek(fd, 100))
  printf("MID SEEK\n");
  fs_print_inode(fd);

  ASSERT_TRUE(fs_write(fd, buf3, 50) == 50)
  fs_print_inode(fd);
  
  ASSERT_PASS(fs_seek(fd, 0))
  printf("AFTER SEEK\n");
  fs_print_inode(fd);
  ASSERT_TRUE(fs_read(fd, buf2, buf_size) == buf_size)
  // ASSERT_TRUE(fs_read(fd, buf2, 512) == 512)
  // strncat(tempbuf,buf2,512);
  
  // for (i = 0; i < 512; i++) {
  //   // printf(buf1[i] == buf2[i]);
  //   ASSERT_TRUE(buf1[i] == buf2[i])
  // }
  
  // ASSERT_PASS(fs_seek(fd, 511))
  // printf("MID SEEK\n");
  // fs_print_inode(fd);
  
  // ASSERT_TRUE(fs_read(fd, buf3, 300) == 300)
  // strncat(tempbuf,buf2,512);
  printf("AFTER READ\n");
  fs_print_inode(fd);
  
  fs_print_oft();
  fs_printfreemask();

  // for (i = 0; i < 400; i++) {
  //   // printf(buf1[i] == buf2[i]);
  //   ASSERT_TRUE(buf1[i] == buf2[i])
  // }
  // for (i = 400; i < 800; i++) {
  //   // printf(buf1[i] == buf2[i]);
  //   ASSERT_TRUE(buf1[i-400] == buf2[i])
  // }
  for (i = 0; i < buf_size; i++) {
    // printf(buf1[i] == buf2[i]);
    // printf("%d-",buf1[i]);
    // printf("%d\n",buf2[i]);
  }
  for (i = 0; i < buf_size; i++) {
    ASSERT_TRUE(buf1[i] == buf2[i])
  }

  ASSERT_PASS(fs_close(fd))

  ASSERT_PASS(freemem(buf1, sizeof(char) * buf_size))
  ASSERT_PASS(freemem(buf2, sizeof(char) * buf_size))

  ASSERT_PASS(fs_freefs(0));
  ASSERT_PASS(bs_freedev(0));
}

// =====================================================================
int fstest_read_write() {
  int i;
  char *buf1, *buf2;
  int buf_size = 512;
  int fd;

  buf1 = getmem(sizeof(char) * buf_size);
  buf2 = getmem(sizeof(char) * buf_size);

  for (i = 0; i < buf_size; i++) {
    buf1[i] = (char) i;
    buf2[i] = (char) 0;
  }

  ASSERT_PASS(bs_mkdev(0, MDEV_BLOCK_SIZE, MDEV_NUM_BLOCKS))
  ASSERT_PASS(fs_mkfs(0, DEFAULT_NUM_INODES))

  ASSERT_PASS(fd = fs_create("file", O_CREAT))

  ASSERT_TRUE(fs_write(fd, buf1, buf_size) == buf_size)
  ASSERT_PASS(fs_seek(fd, 0))
  ASSERT_TRUE(fs_read(fd, buf2, buf_size) == buf_size)

  for (i = 0; i < buf_size; i++) {
    ASSERT_TRUE(buf1[i] == buf2[i])
  }

  ASSERT_PASS(fs_close(fd))

  ASSERT_PASS(freemem(buf1, sizeof(char) * buf_size))
  ASSERT_PASS(freemem(buf2, sizeof(char) * buf_size))

  ASSERT_PASS(fs_freefs(0))
  ASSERT_PASS(bs_freedev(0))
}

// =====================================================================
int fstest_write_too_much() {
  ASSERT_PASS(bs_mkdev(0, MDEV_BLOCK_SIZE, MDEV_NUM_BLOCKS))
  ASSERT_PASS(fs_mkfs(0, DEFAULT_NUM_INODES))
  int x;
  int dataSize = 10000;
  char *data = getmem(dataSize);
  int i;
 
  int fd;
  for(x=0;x<16;x++) {
    char filename[50];
    sprintf(filename, "test%d", x);
    fd = fs_create(filename, O_CREAT);
    printf("Amol0 %d",fd);
    char *readdata = getmem(dataSize);
    char characters[4] = {'a', 'b', 'c', 'd'};
    int charId = 0;
    for(i=0; i<dataSize; i++) {
      data[i]=characters[charId];
      charId = (charId + 1) % 4;
      // readdata[i] = (char) 0;
    }
    ASSERT_TRUE(fd != SYSERR)
    int bytesWritten = fs_write(fd, (void *) data, dataSize);
    printf("bytesWritten = %d\n", bytesWritten);
    fs_print_inode(fd);
    fs_print_oft();
    fs_printfreemask();
    ASSERT_TRUE(bytesWritten != -1)
    ASSERT_PASS(fs_close(fd));
    fs_print_inode(fd);
    fd = fs_open(filename, O_RDONLY);
    fs_print_inode(fd);
    printf("Amol1 %d",fd);
    ASSERT_TRUE(fd != SYSERR)
    int bytesRead = fs_read(fd, (void *) readdata, dataSize);
    printf("Amol2 %d",bytesRead);
    ASSERT_TRUE(bytesRead == 5120)
    // for(i=0; i<bytesRead; i++)
    //   printf("%c", readdata[i]);
    // for (i = 0; i < bytesRead; i++) {
    //   printf("i=%d data[i]=%c and readdata[i]=%c\n", i, data[i], readdata[i]);
    //   ASSERT_TRUE(data[i] == readdata[i])
    // }
    // ASSERT_PASS(fs_close(fd));
  }
}

// =====================================================================
int fstest_write() {
  ASSERT_PASS(bs_mkdev(0, MDEV_BLOCK_SIZE, MDEV_NUM_BLOCKS))
  ASSERT_PASS(fs_mkfs(0, DEFAULT_NUM_INODES))
  int x;
  int dataSize = 10000;
  char *data = getmem(dataSize);
  int i;
  char characters[4] = {'a', 'b', 'c', 'd'};
  int charId = 0;
  for(i=0; i<dataSize; i++) {
    data[i]=characters[charId];
    charId = (charId + 1) % 4;
    // readdata[i] = (char) 0;
  }
  int fd;
  char filename[50];
  sprintf(filename, "test", x);
  fd = fs_create(filename, O_CREAT);
  ASSERT_TRUE(fd != SYSERR)
  int bytesWritten = fs_write(fd, (void *) data, dataSize);
  printf("bytesWritten = %d\n", bytesWritten);
  ASSERT_TRUE(bytesWritten != -1)
  ASSERT_PASS(fs_close(fd));

  int j;
  fd = fs_open(filename, O_RDONLY);
  fs_print_inode(fd);
  int readSize = 7;
  char *readdata = getmem(readSize);
  int bytesRead = fs_read(fd, (void *) readdata, readSize);
  printf("bytesRead = %d\n", bytesRead);
  for(j=0;j<bytesRead;j++) printf("%c", readdata[j]);
  printf("\n");
  ASSERT_PASS(fs_close(fd));

  dataSize = 2560;
  data = getmem(dataSize);
  fd = fs_open(filename, O_RDWR);
  fs_print_inode(fd);
  fs_seek(fd, 2560);
  for(i=0; i<dataSize; i++) data[i]='b';
  printf("%d\n", fs_write(fd, (void *) data, dataSize));
  ASSERT_PASS(fs_close(fd));

  fd = fs_open("test", O_RDONLY);
  fs_print_inode(fd);
  readSize = 0;
  readdata = getmem(dataSize);
  printf("readdata pointing at %d till %d\n", readdata, readdata + dataSize);
  bytesRead = fs_read(fd, (void *) readdata, dataSize);
  printf("bytesRead = %d\n", bytesRead);
  // printf("read - %d and value: %s\n", bytesRead, readdata);

  for(j=0;j<bytesRead;j++) printf("%c", readdata[j]);
  printf("\n");
  ASSERT_PASS(fs_freefs(0))
  ASSERT_PASS(bs_freedev(0))
  return OK;
}

// =====================================================================
int fstest_link() {
  int i;
  char *buf1, *buf2;
  int buf_size = 512;
  int fd;

  buf1 = getmem(sizeof(char) * buf_size);
  buf2 = getmem(sizeof(char) * buf_size);

  for (i = 0; i < buf_size; i++) {
    buf1[i] = (char) i;
    buf2[i] = (char) 0;
  }

  ASSERT_PASS(bs_mkdev(0, MDEV_BLOCK_SIZE, MDEV_NUM_BLOCKS))
  ASSERT_PASS(fs_mkfs(0, DEFAULT_NUM_INODES))

  ASSERT_PASS(fd = fs_create("file", O_CREAT))
  fs_print_inode(fd);
  fs_print_oft();
  
  ASSERT_PASS(fs_close(fd))
  
  ASSERT_FAIL(fs_link("file", "file"))
  ASSERT_FAIL(fs_link("file222", "file111"))
  
  int k;
  for(k=1; k<=15; k++) {
    char filename[50];
    sprintf(filename, "file%d", k);
    ASSERT_PASS(fs_link("file", filename))
  }
  // ASSERT_FAIL(fs_link("file", "file16"))
  printf("AFTER LINKING\n");
  fs_print_inode(fd);
  fs_print_oft();
  
  // ASSERT_PASS(fs_unlink("file1"))
  for(k=1; k<=15; k++) {
    char filename[50];
    sprintf(filename, "file%d", k);
    ASSERT_PASS(fs_unlink(filename))
    printf("AFTER UNLINKING %d\n",k);
    fs_print_inode(fd);
    fs_print_oft();
    fs_print_dir();
  }
  printf("AFTER UNLINKING\n");
  // ASSERT_PASS(fs_unlink("file"))
  fs_print_inode(fd);
  fs_print_oft();
  fs_print_dir();

  ASSERT_PASS(fs_freefs(0))
  ASSERT_PASS(bs_freedev(0))
}

// =====================================================================
int fstest_unlink_duplicate() {
  int fd;
  ASSERT_PASS(bs_mkdev(0, MDEV_BLOCK_SIZE, MDEV_NUM_BLOCKS))
  ASSERT_PASS(fs_mkfs(0, DEFAULT_NUM_INODES))

  ASSERT_PASS(fd = fs_create("file66", O_CREAT))
  ASSERT_PASS(fs_close(fd))  

  ASSERT_PASS(fs_unlink("file66"))
  printf("AFTER UNLINKING\n");
  fs_print_inode(fd);
  fs_print_oft();
  fs_print_dir();

  ASSERT_FAIL(fs_unlink("file66"))
  printf("AFTER UNLINKING\n");
  fs_print_inode(fd);
  fs_print_oft();
  fs_print_dir();
}

// =====================================================================
int fstest_link_transitive_unlink() {
  int fd;
  ASSERT_PASS(bs_mkdev(0, MDEV_BLOCK_SIZE, MDEV_NUM_BLOCKS))
  ASSERT_PASS(fs_mkfs(0, DEFAULT_NUM_INODES))

  ASSERT_PASS(fd = fs_create("file20", O_CREAT))
  ASSERT_PASS(fs_close(fd))  

  ASSERT_PASS(fs_link("file20", "file21"))
  ASSERT_PASS(fs_link("file21", "file22"))
  ASSERT_PASS(fs_link("file22", "file23"))
  printf("AFTER LINKING\n");
  fs_print_inode(fd);
  fs_print_oft();

  ASSERT_PASS(fs_unlink("file20"))
  printf("AFTER UNLINKING\n");
  fs_print_inode(fd);
  fs_print_oft();
  fs_print_dir();

  ASSERT_PASS(fs_unlink("file21"))
  printf("AFTER UNLINKING\n");
  fs_print_inode(fd);
  fs_print_oft();
  fs_print_dir();

  ASSERT_PASS(fs_unlink("file22"))
  printf("AFTER UNLINKING\n");
  fs_print_inode(fd);
  fs_print_oft();
  fs_print_dir();

  ASSERT_PASS(fs_unlink("file23"))
  printf("AFTER UNLINKING\n");
  fs_print_inode(fd);
  fs_print_oft();
  fs_print_dir();
}

// =====================================================================
int fstest_create_unlink() {
  char *buf1, *buf2;
  int buf_size = 5120;
  int fd, fd2;
  int i;

  buf1 = getmem(sizeof(char) * buf_size);
  buf2 = getmem(sizeof(char) * buf_size);

  for (i = 0; i < buf_size; i++) {
    buf1[i] = (char) i;
    buf2[i] = (char) 0;
  }

  ASSERT_PASS(bs_mkdev(0, MDEV_BLOCK_SIZE, MDEV_NUM_BLOCKS))
  ASSERT_PASS(fs_mkfs(0, DEFAULT_NUM_INODES))

  ASSERT_PASS(fd = fs_create("file99", O_CREAT))
  fs_print_inode(fd);
  fs_print_oft();
  
  ASSERT_TRUE(fs_write(fd, buf1, buf_size) == buf_size)
  printf("AFTER WRITE\n");
  fs_print_inode(fd);
  fs_print_oft();

  ASSERT_PASS(fs_seek(fd, 0))
  fs_print_inode(fd);
  fs_print_oft();
  
  ASSERT_TRUE(fs_read(fd, buf2, buf_size) == buf_size)
  printf("AFTER READ\n");
  fs_print_inode(fd);
  fs_print_oft();

  for (i = 0; i < buf_size; i++) {
    // printf("%d-",buf1[i]);
    // printf("%d\n",buf2[i]);
    ASSERT_TRUE(buf1[i] == buf2[i])
  }

  ASSERT_PASS(fs_close(fd))

  ASSERT_PASS(fs_link("file99","file100"))
  printf("AFTER LINKING\n");
  fd2 = fs_open("file100", O_RDWR);
  fs_print_inode(fd2);
  ASSERT_PASS(fs_close(fd2))
  fs_print_oft();
  fs_print_dir();
  
  printf("amol1\n");
  ASSERT_PASS(fs_unlink("file100"))
  printf("AFTER UNLINKING\n");
  fs_print_inode(fd);
  fs_print_oft();
  fs_print_dir();

  ASSERT_PASS(freemem(buf1, sizeof(char) * buf_size))
  ASSERT_PASS(freemem(buf2, sizeof(char) * buf_size))

  ASSERT_PASS(fs_freefs(0))
  ASSERT_PASS(bs_freedev(0))
}

// =====================================================================
/*
 * Test the parse_path function
 */
int fstest_parsing_directories() {

  char filename[256];
  char *offset = NULL;

  printf("Parsing: [%s]\n", NULL);
  parse_path(NULL, &offset);
  printf("Returned: [%s] [%s]\n", NULL, offset);

  sprintf(filename, "");
  printf("\nParsing: [%s]\n", filename);
  parse_path(filename, &offset);
  printf("Returned: [%s] [%s]\n", filename, offset);

  sprintf(filename, "filename");
  printf("\nParsing: [%s]\n", filename);
  parse_path(filename, &offset);
  printf("Returned: [%s] [%s] (changed: %d)\n", filename, offset, offset - filename ? 1 : 0);

  sprintf(filename, "dirname/filename");
  printf("\nParsing: [%s]\n", filename);
  parse_path(filename, &offset);
  printf("Returned: [%s] [%s] (changed: %d)\n", filename, offset, offset - filename ? 1 : 0);

  sprintf(filename, "dirname/subdir/filename");
  printf("\nParsing: [%s]\n", filename);
  parse_path(filename, &offset);
  printf("Returned: [%s] [%s] (changed: %d)\n", filename, offset, offset - filename ? 1 : 0);

  sprintf(filename, "dirname/subdir/subsub/filename");
  printf("\nParsing: [%s]\n", filename);
  parse_path(filename, &offset);
  printf("Returned: [%s] [%s] (changed: %d)\n", filename, offset, offset - filename ? 1 : 0);

  fs_print_dirs(NULL,0);
  return OK;
}

// =====================================================================
int fstest_dir_creation_from_file() {
  ASSERT_PASS(bs_mkdev(0, MDEV_BLOCK_SIZE, MDEV_NUM_BLOCKS))
  ASSERT_PASS(fs_mkfs(0, DEFAULT_NUM_INODES))
  
  ASSERT_PASS(fs_create("dirname", O_DIR))
  ASSERT_PASS(fs_create("dirname/file", O_CREAT))
  ASSERT_FAIL(fs_create("dirname/file/subdir1", O_DIR))
  ASSERT_FAIL(fs_create("dirname/file/subdir1/subdir2", O_DIR))
 
  ASSERT_PASS(fs_freefs(0))
  ASSERT_PASS(bs_freedev(0))
}

// =====================================================================
int fstest_missing_intermediate_dir() {
  ASSERT_PASS(bs_mkdev(0, MDEV_BLOCK_SIZE, MDEV_NUM_BLOCKS))
  ASSERT_PASS(fs_mkfs(0, DEFAULT_NUM_INODES))
  
  ASSERT_PASS(fs_create("dirname", O_DIR))
  ASSERT_FAIL(fs_create("dirname/subdir/subdir1", O_DIR))
  ASSERT_FAIL(fs_create("dirname/subdir/subdir1/subdir2", O_DIR))
 
  ASSERT_PASS(fs_freefs(0))
  ASSERT_PASS(bs_freedev(0))
}

// =====================================================================
int fstest_create_dir_then_open_file() {
  int fd;
  ASSERT_PASS(bs_mkdev(0, MDEV_BLOCK_SIZE, MDEV_NUM_BLOCKS))
  ASSERT_PASS(fs_mkfs(0, DEFAULT_NUM_INODES))
  
  ASSERT_PASS(fs_create("dirname", O_DIR))
  ASSERT_PASS(fd = fs_create("dirname/file1", O_CREAT))
  fs_print_inode(fd);
  fs_print_oft();
  fs_print_dirs(NULL,0);
  
  ASSERT_PASS(fs_freefs(0))
  ASSERT_PASS(bs_freedev(0))
}

// =====================================================================
int fstest_open_dir() {
  int fd;
  ASSERT_PASS(bs_mkdev(0, MDEV_BLOCK_SIZE, MDEV_NUM_BLOCKS))
  ASSERT_PASS(fs_mkfs(0, DEFAULT_NUM_INODES))
  
  ASSERT_PASS(fs_create("dirname", O_DIR))
  ASSERT_FAIL(fd = fs_open("dirname", O_RDWR))
  fs_print_inode(fd);
  fs_print_oft();
  fs_print_dirs(NULL,0);
  
  ASSERT_PASS(fs_freefs(0))
  ASSERT_PASS(bs_freedev(0))
}

// =====================================================================
int fstest_max_dir_len() {
  ASSERT_PASS(bs_mkdev(0, MDEV_BLOCK_SIZE, MDEV_NUM_BLOCKS))
  ASSERT_PASS(fs_mkfs(0, DEFAULT_NUM_INODES))
  
  ASSERT_PASS(fs_create("dirname", O_DIR))
  ASSERT_PASS(fs_create("dirname/subdir", O_DIR))
  ASSERT_PASS(fs_create("dirname/subdir/subdir1", O_DIR))
  ASSERT_PASS(fs_create("dirname/subdir/subdir2", O_DIR))
  ASSERT_PASS(fs_create("dirname/subdir/subdir3", O_DIR))
  ASSERT_PASS(fs_create("dirname/subdir/subdir4", O_DIR))
  ASSERT_PASS(fs_create("dirname/subdir/subdir5", O_DIR))
  ASSERT_PASS(fs_create("dirname/subdir/subdir6", O_DIR))
  ASSERT_PASS(fs_create("dirname/subdir/subdir7", O_DIR))
  ASSERT_PASS(fs_create("dirname/subdir/subdir8", O_DIR))
  ASSERT_PASS(fs_create("dirname/subdir/subdir9", O_DIR))
  ASSERT_PASS(fs_create("dirname/subdir/subdir10", O_DIR))
  ASSERT_PASS(fs_create("dirname/subdir/subdir11", O_DIR))
  ASSERT_PASS(fs_create("dirname/subdir/subdir12", O_DIR))
  ASSERT_PASS(fs_create("dirname/subdir/subdir13", O_DIR))
  ASSERT_PASS(fs_create("dirname/subdir/subdir14", O_DIR))
  ASSERT_PASS(fs_create("dirname/subdir/subdir15", O_DIR))
  ASSERT_PASS(fs_create("dirname/subdir/subdir16", O_DIR))
  ASSERT_FAIL(fs_create("dirname/subdir/subdir17", O_DIR))
  
  fs_print_oft();
  fs_print_dirs(NULL,0);

  ASSERT_PASS(fs_freefs(0))
  ASSERT_PASS(bs_freedev(0))
}

// =====================================================================
int fstest_max_dir_depth() {
  ASSERT_PASS(bs_mkdev(0, MDEV_BLOCK_SIZE, MDEV_NUM_BLOCKS))
  ASSERT_PASS(fs_mkfs(0, DEFAULT_NUM_INODES))
  
  ASSERT_PASS(fs_create("dirname", O_DIR))
  ASSERT_PASS(fs_create("dirname/subdir", O_DIR))
  ASSERT_PASS(fs_create("dirname/subdir/subdir1", O_DIR))
  ASSERT_PASS(fs_create("dirname/subdir/subdir1/subdir2", O_DIR))
  ASSERT_PASS(fs_create("dirname/subdir/subdir1/subdir2/subdir3", O_DIR))
  ASSERT_PASS(fs_create("dirname/subdir/subdir1/subdir2/subdir3/subdir4", O_DIR))
  ASSERT_PASS(fs_create("dirname/subdir/subdir1/subdir2/subdir3/subdir4/subdir5", O_DIR))
  ASSERT_PASS(fs_create("dirname/subdir/subdir1/subdir2/subdir3/subdir4/subdir5/subdir6", O_DIR))
  ASSERT_PASS(fs_create("dirname/subdir/subdir1/subdir2/subdir3/subdir4/subdir5/subdir6/subdir7", O_DIR))
  ASSERT_PASS(fs_create("dirname/subdir/subdir1/subdir2/subdir3/subdir4/subdir5/subdir6/subdir7/subdir8", O_DIR))
  ASSERT_PASS(fs_create("dirname/subdir/subdir1/subdir2/subdir3/subdir4/subdir5/subdir6/subdir7/subdir8/subdir9", O_DIR))
  ASSERT_PASS(fs_create("dirname/subdir/subdir1/subdir2/subdir3/subdir4/subdir5/subdir6/subdir7/subdir8/subdir9/subdir10", O_DIR))
  ASSERT_PASS(fs_create("dirname/subdir/subdir1/subdir2/subdir3/subdir4/subdir5/subdir6/subdir7/subdir8/subdir9/subdir10/subdir11", O_DIR))
  ASSERT_PASS(fs_create("dirname/subdir/subdir1/subdir2/subdir3/subdir4/subdir5/subdir6/subdir7/subdir8/subdir9/subdir10/subdir11/subdir12", O_DIR))
  ASSERT_PASS(fs_create("dirname/subdir/subdir1/subdir2/subdir3/subdir4/subdir5/subdir6/subdir7/subdir8/subdir9/subdir10/subdir11/subdir12/subdir13", O_DIR))
  ASSERT_PASS(fs_create("dirname/subdir/subdir1/subdir2/subdir3/subdir4/subdir5/subdir6/subdir7/subdir8/subdir9/subdir10/subdir11/subdir12/subdir13/subdir14", O_DIR))
  ASSERT_FAIL(fs_create("dirname/subdir/subdir1/subdir2/subdir3/subdir4/subdir5/subdir6/subdir7/subdir8/subdir9/subdir10/subdir11/subdir12/subdir13/subdir14/subdir15", O_DIR))

  fs_print_oft();
  fs_print_dirs(NULL,0);

  ASSERT_PASS(fs_freefs(0))
  ASSERT_PASS(bs_freedev(0))
}

// =====================================================================
int fstest_dir_and_files() {
  int fd,fd2,i;
  ASSERT_PASS(bs_mkdev(0, MDEV_BLOCK_SIZE, MDEV_NUM_BLOCKS))
  ASSERT_PASS(fs_mkfs(0, DEFAULT_NUM_INODES))
  
  ASSERT_PASS(fd = fs_create("filename", O_CREAT))
  ASSERT_PASS(fs_create("twoverylongname", O_CREAT))
  ASSERT_PASS(fs_create("dirname", O_DIR))
  ASSERT_PASS(fs_create("dirname/subdir", O_DIR))
  ASSERT_PASS(fs_create("dirname/subdir/deepfile", O_CREAT))
  ASSERT_PASS(fs_create("dirname/subdir/x", O_CREAT))
  fs_print_oft();
  fs_print_dirs(NULL,0);
  
  ASSERT_FAIL(fs_link("filename", "overLimitFilenameLength"))
  ASSERT_PASS(fs_link("filename", "sameLvlFileLink"))
  ASSERT_PASS(fs_link("dirname/subdir/deepfile", "dirname/subdirFileLink"))
  ASSERT_PASS(fs_link("dirname/subdir", "fromSubdirLink"))
  
  fs_print_dirs(NULL,0);

  ASSERT_PASS(fs_unlink("sameLvlFileLink"))
  ASSERT_PASS(fs_unlink("dirname/subdirFileLink"))
  ASSERT_PASS(fs_unlink("fromSubdirLink"))
  fs_print_oft();

  fs_print_dirs(NULL,0);

  ASSERT_PASS(fs_freefs(0))
  ASSERT_PASS(bs_freedev(0)) 
}

// =====================================================================
int fstest_create_all_directories(char *path, int level) {
  int fd;
  int j,k;

  if(level == 4) {
    return;
  }
  
  for(k=1; k<=4; k++) {
    char filename[50];
    if(level == 0) {
      sprintf(filename, "%s%d", path,k);  
    }
    else {
      sprintf(filename, "%s/subdir%d", path,k);
    }
    printf("%s\n",filename);
    ASSERT_PASS(fd = fs_create(filename, O_DIR))
    ASSERT_TRUE(fd != SYSERR)

    fstest_create_all_directories(filename, level+1);
  }
}

int run_fstest_create_all_directories() {
  ASSERT_PASS(bs_mkdev(0, MDEV_BLOCK_SIZE, MDEV_NUM_BLOCKS))
  ASSERT_PASS(fs_mkfs(0, DEFAULT_NUM_INODES))
  
  char filename[50];
  sprintf(filename, "dir");
  fstest_create_all_directories(filename,0);
  fs_print_dirs(NULL,0);

  ASSERT_PASS(fs_freefs(0))
  ASSERT_PASS(bs_freedev(0))
}

// =====================================================================
int fstest(int nargs, char *args[]) {

  /* Output help, if '--help' argument was supplied */
  if (nargs == 2 && strncmp(args[1], "--help", 7) == 0) {
    printf("Usage: %s [TEST]\n\n", args[0]);
    printf("Description:\n");
    printf("\tFilesystem Test\n");
    printf("Options:\n");
    printf("\t--help\tdisplay this help and exit\n");
    return OK;
  }

  /* Check for correct number of arguments */
  if (nargs > 2) {
    fprintf(stderr, "%s: too many arguments\n", args[0]);
    fprintf(stderr, "Try '%s --help' for more information\n",
            args[0]);
    return SYSERR;
  }

#ifdef FS

  printf("\n\n\n");
  TEST(fstest_testbitmask)
  TEST(fstest_mkdev)
  TEST(fstest_create_multiple)
  TEST(fstest_close)

  TEST(fstest_write_too_much)
  TEST(fs_write_max_then_read)
  TEST(fs_write_max_plus_one)
  TEST(fs_write_chunks_then_read)
  TEST(fs_write_read)
  TEST(fs_write_then_read_chunks)
  TEST(fstest_write)
  TEST(fstest_read_write)

  TEST(fstest_link)
  TEST(fstest_unlink_duplicate)
  TEST(fstest_create_unlink)
  TEST(fstest_link_transitive_unlink)

  TEST(fstest_parsing_directories)
  TEST(fstest_max_dir_len)
  TEST(fstest_max_dir_depth)
  TEST(fstest_missing_intermediate_dir)
  TEST(fstest_create_dir_then_open_file)
  TEST(fstest_dir_creation_from_file)
  TEST(fstest_open_dir)
  TEST(run_fstest_create_all_directories)
  TEST(fstest_dir_and_files)

#else
  printf("No filesystem support\n");
#endif

  return OK;
}