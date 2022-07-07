#include <xinu.h>
#include <kernel.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#ifdef FS
#include <fs.h>

fsystem_t fsd;
int dev0_numblocks;
int dev0_blocksize;
char *dev0_blocks;

extern int dev0;

char block_cache[512];

#define SB_BLK 0 // Superblock
#define BM_BLK 1 // Bitmapblock

#define NUM_FD 16

filetable_t oft[NUM_FD]; // open file table
#define isbadfd(fd) (fd < 0 || fd >= NUM_FD || oft[fd].in.id == EMPTY)

#define INODES_PER_BLOCK (fsd.blocksz / sizeof(inode_t))
#define NUM_INODE_BLOCKS (( (fsd.ninodes % INODES_PER_BLOCK) == 0) ? fsd.ninodes / INODES_PER_BLOCK : (fsd.ninodes / INODES_PER_BLOCK) + 1)
#define FIRST_INODE_BLOCK 2

/**
 * Helper functions
 */
int _fs_fileblock_to_diskblock(int dev, int fd, int fileblock) {
  int diskblock;

  if (fileblock >= INODEDIRECTBLOCKS) {
    errormsg("No indirect block support! (%d >= %d)\n", fileblock, INODEBLOCKS - 2);
    return SYSERR;
  }

  // Get the logical block address
  diskblock = oft[fd].in.blocks[fileblock];

  return diskblock;
}

/**
 * Filesystem functions
 */
int _fs_get_inode_by_num(int dev, int inode_number, inode_t *out) {
  int bl, inn;
  int inode_off;

  if (dev != dev0) {
    errormsg("Unsupported device: %d\n", dev);
    return SYSERR;
  }
  if (inode_number > fsd.ninodes) {
    errormsg("inode %d out of range (> %s)\n", inode_number, fsd.ninodes);
    return SYSERR;
  }

  bl  = inode_number / INODES_PER_BLOCK;
  inn = inode_number % INODES_PER_BLOCK;
  bl += FIRST_INODE_BLOCK;

  inode_off = inn * sizeof(inode_t);

  bs_bread(dev0, bl, 0, &block_cache[0], fsd.blocksz);
  memcpy(out, &block_cache[inode_off], sizeof(inode_t));

  return OK;

}

int _fs_put_inode_by_num(int dev, int inode_number, inode_t *in) {
  int bl, inn;

  if (dev != dev0) {
    errormsg("Unsupported device: %d\n", dev);
    return SYSERR;
  }
  if (inode_number > fsd.ninodes) {
    errormsg("inode %d out of range (> %d)\n", inode_number, fsd.ninodes);
    return SYSERR;
  }

  bl = inode_number / INODES_PER_BLOCK;
  inn = inode_number % INODES_PER_BLOCK;
  bl += FIRST_INODE_BLOCK;

  bs_bread(dev0, bl, 0, block_cache, fsd.blocksz);
  memcpy(&block_cache[(inn*sizeof(inode_t))], in, sizeof(inode_t));
  bs_bwrite(dev0, bl, 0, block_cache, fsd.blocksz);

  return OK;
}

int fs_mkfs(int dev, int num_inodes) {
  int i;

  if (dev == dev0) {
    fsd.nblocks = dev0_numblocks;
    fsd.blocksz = dev0_blocksize;
  } else {
    errormsg("Unsupported device: %d\n", dev);
    return SYSERR;
  }

  if (num_inodes < 1) {
    fsd.ninodes = DEFAULT_NUM_INODES;
  } else {
    fsd.ninodes = num_inodes;
  }

  i = fsd.nblocks;
  while ( (i % 8) != 0) { i++; }
  fsd.freemaskbytes = i / 8;

  if ((fsd.freemask = getmem(fsd.freemaskbytes)) == (void *) SYSERR) {
    errormsg("fs_mkfs memget failed\n");
    return SYSERR;
  }

  /* zero the free mask */
  for(i = 0; i < fsd.freemaskbytes; i++) {
    fsd.freemask[i] = '\0';
  }

  fsd.inodes_used = 0;

  /* write the fsystem block to SB_BLK, mark block used */
  fs_setmaskbit(SB_BLK);
  bs_bwrite(dev0, SB_BLK, 0, &fsd, sizeof(fsystem_t));

  /* write the free block bitmask in BM_BLK, mark block used */
  fs_setmaskbit(BM_BLK);
  bs_bwrite(dev0, BM_BLK, 0, fsd.freemask, fsd.freemaskbytes);

  // Initialize all inode IDs to EMPTY
  inode_t tmp_in;
  for (i = 0; i < fsd.ninodes; i++) {
    _fs_get_inode_by_num(dev0, i, &tmp_in);
    tmp_in.id = EMPTY;
    _fs_put_inode_by_num(dev0, i, &tmp_in);
  }
  fsd.root_dir.numentries = 0;
  for (i = 0; i < DIRECTORY_SIZE; i++) {
    fsd.root_dir.entry[i].inode_num = EMPTY;
    memset(fsd.root_dir.entry[i].name, 0, FILENAMELEN);
  }

  for (i = 0; i < NUM_FD; i++) {
    oft[i].state     = 0;
    oft[i].fileptr   = 0;
    oft[i].de        = NULL;
    oft[i].in.id     = EMPTY;
    oft[i].in.type   = 0;
    oft[i].in.nlink  = 0;
    oft[i].in.device = 0;
    oft[i].in.size   = 0;
    memset(oft[i].in.blocks, 0, sizeof(oft[i].in.blocks));
    oft[i].flag      = 0;
  }

  return OK;
}

int fs_freefs(int dev) {
  if (freemem(fsd.freemask, fsd.freemaskbytes) == SYSERR) {
    return SYSERR;
  }

  return OK;
}

/**
 * Debugging functions
 */
void fs_print_oft(void) {
  int i;

  printf ("\n\033[35moft[]\033[39m\n");
  printf ("%3s  %5s  %7s  %8s  %6s  %5s  %4s  %s\n", "Num", "state", "fileptr", "de", "de.num", "in.id", "flag", "de.name");
  for (i = 0; i < NUM_FD; i++) {
    if (oft[i].de != NULL) printf ("%3d  %5d  %7d  %8d  %6d  %5d  %4d  %s\n", i, oft[i].state, oft[i].fileptr, oft[i].de, oft[i].de->inode_num, oft[i].in.id, oft[i].flag, oft[i].de->name);
  }

  printf ("\n\033[35mfsd.root_dir.entry[] (numentries: %d)\033[39m\n", fsd.root_dir.numentries);
  printf ("%3s  %3s  %s\n", "ID", "id", "filename");
  for (i = 0; i < DIRECTORY_SIZE; i++) {
    if (fsd.root_dir.entry[i].inode_num != EMPTY) printf("%3d  %3d  %s\n", i, fsd.root_dir.entry[i].inode_num, fsd.root_dir.entry[i].name);
  }
  printf("\n");
}

void fs_print_inode(int fd) {
  int i;

  printf("\n\033[35mInode FS=%d\033[39m\n", fd);
  printf("Name:    %s\n", oft[fd].de->name);
  printf("State:   %d\n", oft[fd].state);
  printf("Flag:    %d\n", oft[fd].flag);
  printf("Fileptr: %d\n", oft[fd].fileptr);
  printf("Type:    %d\n", oft[fd].in.type);
  printf("nlink:   %d\n", oft[fd].in.nlink);
  printf("device:  %d\n", oft[fd].in.device);
  printf("size:    %d\n", oft[fd].in.size);
  printf("blocks: ");
  for (i = 0; i < INODEBLOCKS; i++) {
    printf(" %d", oft[fd].in.blocks[i]);
  }
  printf("\n");
  return;
}

void fs_print_fsd(void) {
  int i;

  printf("\033[35mfsystem_t fsd\033[39m\n");
  printf("fsd.nblocks:       %d\n", fsd.nblocks);
  printf("fsd.blocksz:       %d\n", fsd.blocksz);
  printf("fsd.ninodes:       %d\n", fsd.ninodes);
  printf("fsd.inodes_used:   %d\n", fsd.inodes_used);
  printf("fsd.freemaskbytes  %d\n", fsd.freemaskbytes);
  printf("sizeof(inode_t):   %d\n", sizeof(inode_t));
  printf("INODES_PER_BLOCK:  %d\n", INODES_PER_BLOCK);
  printf("NUM_INODE_BLOCKS:  %d\n", NUM_INODE_BLOCKS);

  inode_t tmp_in;
  printf ("\n\033[35mBlocks\033[39m\n");
  printf ("%3s  %3s  %4s  %4s  %3s  %4s\n", "Num", "id", "type", "nlnk", "dev", "size");
  for (i = 0; i < NUM_FD; i++) {
    _fs_get_inode_by_num(dev0, i, &tmp_in);
    if (tmp_in.id != EMPTY) printf("%3d  %3d  %4d  %4d  %3d  %4d\n", i, tmp_in.id, tmp_in.type, tmp_in.nlink, tmp_in.device, tmp_in.size);
  }
  for (i = NUM_FD; i < fsd.ninodes; i++) {
    _fs_get_inode_by_num(dev0, i, &tmp_in);
    if (tmp_in.id != EMPTY) {
      printf("%3d:", i);
      int j;
      for (j = 0; j < 64; j++) {
        printf(" %3d", *(((char *) &tmp_in) + j));
      }
      printf("\n");
    }
  }
  printf("\n");
}

void fs_print_dir(void) {
  int i;
  printf("%22s  %9s  %s\n", "DirectoryEntry", "inode_num", "name");
  for (i = 0; i < DIRECTORY_SIZE; i++) {
    printf("fsd.root_dir.entry[%2d]  %9d  %s\n", i, fsd.root_dir.entry[i].inode_num, fsd.root_dir.entry[i].name);
  }
}

void fs_print_dirs(directory_t *working_dir, int level) {
  int i, j;
  if (working_dir == NULL) {
    working_dir = &(fsd.root_dir);
    level = 0;
    printf("\nroot\n");
  }

  inode_t _in;
  directory_t cache;
  for (i = 0; i < DIRECTORY_SIZE; i++) {
    if (working_dir->entry[i].inode_num != EMPTY) {

      _fs_get_inode_by_num(dev0, working_dir->entry[i].inode_num, &_in);

      for (j = 0; j <= level; j++) printf("\033[38;5;238m:\033[39m ");
      if (_in.type == INODE_TYPE_FILE) {
        printf("f %s\n", working_dir->entry[i].name);
      } else {
        printf("d %s\n", working_dir->entry[i].name);
        bs_bread(dev0, _in.blocks[0], 0, &cache, sizeof(directory_t));
        if (cache.numentries > 0) fs_print_dirs(&cache, level + 1);
      }
    }
  }
}

int fs_setmaskbit(int b) {
  int mbyte, mbit;
  mbyte = b / 8;
  mbit = b % 8;

  fsd.freemask[mbyte] |= (0x80 >> mbit);
  return OK;
}

int fs_getmaskbit(int b) {
  int mbyte, mbit;
  mbyte = b / 8;
  mbit = b % 8;

  return( ( (fsd.freemask[mbyte] << mbit) & 0x80 ) >> 7);
}

int fs_clearmaskbit(int b) {
  int mbyte, mbit, invb;
  mbyte = b / 8;
  mbit = b % 8;

  invb = ~(0x80 >> mbit);
  invb &= 0xFF;

  fsd.freemask[mbyte] &= invb;
  return OK;
}

/**
 * This is maybe a little overcomplicated since the lowest-numbered
 * block is indicated in the high-order bit.  Shift the byte by j
 * positions to make the match in bit7 (the 8th bit) and then shift
 * that value 7 times to the low-order bit to print.  Yes, it could be
 * the other way...
 */
void fs_printfreemask(void) { // print block bitmask
  int i, j;

  for (i = 0; i < fsd.freemaskbytes; i++) {
    for (j = 0; j < 8; j++) {
      printf("%d", ((fsd.freemask[i] << j) & 0x80) >> 7);
    }
    printf(" ");
    if ( (i % 8) == 7) {
      printf("\n");
    }
  }
  printf("\n");
}

/*
 * parse_path
 * Parse the directory path given by @input
 * Output every intermediate directory as it parses
 * Writes the start of the last level to @offset
 *
 * @param    input    input path string
 * @param    offset   pointer into the input string (last level)
 */
void parse_path(char *input, char **offset) {
  /*  xxxxx/xxxxx/xxxx0
   *  ^
   *  a,b
   *
   *  #####/xxxxx/xxxx0
   *  ^    ^
   *  a    b
   *
   *  xxxxx/#####/xxxx0
   *        ^    ^
   *        a    b
   *
   *  xxxxx/xxxxx/####0
   *              ^   ^
   *              a   b
   */

  char *a, *b;
  char tmp[FILENAMELEN];

  if (input == NULL) return;
  int depth = 1;
  *offset = input;

  for (a = b = input; *b != '\0'; b++) {
    if (*b == '/') {
      memset(tmp, 0, sizeof(tmp));
      memcpy(tmp, a, b-a);
      depth++;
      // printf("> %s\n", tmp);
      a = b + 1;
    }
  }

  if (a < b) *offset = a;
}

// =====================================================================
// Input: Filepath
// Returns: Last level directory inode num or SYSERR
int fs_check_intermediate_directories(char *filepath) {
  int i;
  int depth = 0;
  int DIRECTORY_DEPTH = 16;
  int final_level_inode_num = -9999;

  // Checks if intermediate directories are present
  char *a, *b;
  char tmp[FILENAMELEN];
  for (a = b = filepath; *b != '\0'; b++) {
    if (*b == '/') {
      memset(tmp, 0, sizeof(tmp));
      memcpy(tmp, a, b-a);
      if(strlen(tmp) > FILENAMELEN || strlen(tmp) == 0) {
        return SYSERR;
      }

      // Check if directory is present
      // If not, return SYSERR
      bool flag = TRUE;
      a = b + 1;
      if(depth == 0) {
        for (i = 0; i < DIRECTORY_SIZE; i++) {
          if(strcmp(tmp,fsd.root_dir.entry[i].name) == 0) {
            final_level_inode_num = fsd.root_dir.entry[i].inode_num;
            flag = FALSE;
            depth++;
            break;
          }
        }
      }
      else {
        inode_t dir_inode;
        directory_t tmp_dir;
        _fs_get_inode_by_num(dev0, final_level_inode_num, &dir_inode);
        bs_bread(dev0, dir_inode.blocks[0], 0, &tmp_dir, sizeof(directory_t));
        for (i = 0; i < DIRECTORY_SIZE; i++) {
          if(strcmp(tmp,tmp_dir.entry[i].name) == 0) {
            final_level_inode_num = tmp_dir.entry[i].inode_num;
            flag = FALSE;
            depth++;
            break;
          }
        }

        // Creating directory from file
        inode_t typecheck_inode;
        _fs_get_inode_by_num(dev0, final_level_inode_num, &typecheck_inode);
        if(typecheck_inode.type != INODE_TYPE_DIR) {
          return SYSERR;
        }
      }
      // If intermediate directory is not present
      if(flag) {
        return SYSERR;
      }
    }
  }
  if(depth >= 16) {
    return SYSERR;
  }
  return final_level_inode_num;
}

// =====================================================================
int fs_open(char *filepath, int flags) {
  // Check for NULL filename
  if(filepath == NULL) {
    return SYSERR;
  }
  // Invalid flags provided
  if(flags != O_RDONLY && flags != O_WRONLY && flags != O_RDWR && flags != O_CREAT && flags != O_DIR) {
    return SYSERR;
  }
  if(flags == O_CREAT) {
    return fs_create(filepath,flags);
  }

  int final_level_inode_num = fs_check_intermediate_directories(filepath);
  if(final_level_inode_num == SYSERR) {
    return SYSERR;
  }

  int i,j,k;
  char *filename = NULL;
  parse_path(filepath,&filename);

  if(final_level_inode_num != -9999) {
    inode_t inode;
    directory_t tmp_dir;
    if(_fs_get_inode_by_num(dev0, final_level_inode_num, &inode)) {
      bs_bread(dev0, inode.blocks[0], 0, &tmp_dir, sizeof(directory_t));
      for (i=0; i<DIRECTORY_SIZE; i++) {
        if (strcmp(filename, tmp_dir.entry[i].name) == 0) {
          inode_t file_inode;
          bool is_opened = FALSE;
          if(_fs_get_inode_by_num(dev0, tmp_dir.entry[i].inode_num, &file_inode)) {
            // Check if the file is already open in oft
            for (j=0; j<NUM_FD; j++) {
              if(oft[j].in.id == file_inode.id && oft[j].state == FSTATE_OPEN) {
                return SYSERR;
              }
            }
            // Cannot open directories
            if(file_inode.type == INODE_TYPE_DIR) {
              return SYSERR;
            }
            // Search for an empty oft slot 
            for (j=0; j<NUM_FD; j++) {
              if(oft[j].de == NULL) {
                is_opened = TRUE;
                // Opening a file
                oft[j].state = FSTATE_OPEN;
                oft[j].fileptr = 0;
                oft[j].de = &tmp_dir.entry[i];
                oft[j].in = file_inode;
                oft[j].flag = flags;
                break;
              }
            }
            // Checks OFT Limit and successful file opening
            if(!is_opened) {
              return SYSERR;
            }
            if(_fs_put_inode_by_num(dev0,tmp_dir.entry[i].inode_num,&file_inode)) {
              return j;
            }
          }
        }
      }
    }
  }
  else {
    inode_t inode;
    for (i=0; i<DIRECTORY_SIZE; i++) {
      if (strcmp(filename, fsd.root_dir.entry[i].name) == 0) {        
        bool is_opened = FALSE;
        if(_fs_get_inode_by_num(0, fsd.root_dir.entry[i].inode_num, &inode)) {
          // Check if the file is already open in oft
          for (j=0; j<NUM_FD; j++) {
            if(oft[j].in.id == inode.id && oft[j].state == FSTATE_OPEN) {
              return SYSERR;
            }
          }
          // Cannot open directories
          if(inode.type == INODE_TYPE_DIR) {
            return SYSERR;
          }
          // Search for an empty oft slot 
          for (j=0; j<NUM_FD; j++) {
            if(oft[j].de == NULL) {
              is_opened = TRUE;
              // Opening a file
              oft[j].state = FSTATE_OPEN;
              oft[j].fileptr = 0;
              oft[j].de = &fsd.root_dir.entry[i];
              oft[j].in = inode;
              oft[j].flag = flags;
              break;
            }
          }
          // Checks OFT Limit and successful file opening
          if(!is_opened) {
            return SYSERR;
          }
          if(_fs_put_inode_by_num(0,fsd.root_dir.entry[i].inode_num,&oft[j].in)) {
            return j;
          }
        }
      }
    }
  }
  return SYSERR;
}

// =====================================================================
int fs_close(int fd) {
  // File is already closed
  if (oft[fd].state == FSTATE_CLOSED) {
      return SYSERR;
  }
  // Invalid file descriptor
  else if (isbadfd(fd)) {
      return SYSERR;
  }
  oft[fd].state = FSTATE_CLOSED;
  oft[fd].fileptr = 0;
  oft[fd].de = NULL;
  return OK;
}

// =====================================================================
int fs_create(char *filepath, int mode) {
  // Check if filepath is NULL
  if (filepath == NULL) return SYSERR;
  // Check if last char is '/'
  char lastChar = filepath[strlen(filepath)-1];
  if(lastChar == '/') {
    return SYSERR;
  }
  // If mode is incorrect
  if(mode != O_CREAT && mode != O_DIR && mode != O_RDWR) {
    return SYSERR;
  }
  int final_level_inode_num = fs_check_intermediate_directories(filepath);
  if(final_level_inode_num == SYSERR) {
    return SYSERR;
  }
  
  int i,j,k;
  char *filename = NULL;
  parse_path(filepath,&filename);
  if(strlen(filename)+1 > FILENAMELEN || strlen(filename) == 0) {
    return SYSERR;
  }

  // Check if filename is NULL
  if (filename == NULL) return SYSERR;
  if(fsd.inodes_used >= fsd.ninodes) {
    return SYSERR;
  }
  if(mode == O_CREAT) {
    inode_t inode;
    if(_fs_get_inode_by_num(dev0, fsd.inodes_used, &inode)) {
      inode.id = fsd.inodes_used++;
      inode.type =  INODE_TYPE_FILE;
      inode.nlink = 1;
      inode.device = dev0;
      inode.size = 0;
      
      if(_fs_put_inode_by_num(dev0, inode.id, &inode)) {
        if(final_level_inode_num != -9999) {
          inode_t dir_inode;
          directory_t tmp_dir;
          if(_fs_get_inode_by_num(dev0, final_level_inode_num, &dir_inode)) {
            bs_bread(dev0, dir_inode.blocks[0], 0, &tmp_dir, sizeof(directory_t));
            // If current directory is full
            if(tmp_dir.numentries >= DIRECTORY_SIZE) {
              return SYSERR;
            }
            // If file is already present in final directory
            for (i=0; i<DIRECTORY_SIZE; i++) {
              if (strcmp(filename, tmp_dir.entry[i].name) == 0) {
                  return SYSERR;
              }
            }

            for(j=0; j<DIRECTORY_SIZE; j++) {
              if(tmp_dir.entry[j].inode_num == EMPTY) {
                tmp_dir.entry[j].inode_num = inode.id;
                strcpy(tmp_dir.entry[j].name,filename);
                tmp_dir.numentries = tmp_dir.numentries + 1;
                _fs_put_inode_by_num(dev0, final_level_inode_num, &dir_inode);
                bs_bwrite(dev0, dir_inode.blocks[0], 0, &tmp_dir, sizeof(directory_t));
                break;
              }
            }
          }
        }
        else {
          // If current directory is full
          if(fsd.root_dir.numentries >= DIRECTORY_SIZE) {
            return SYSERR;
          }
          // If file is already present
          for (i=0; i<DIRECTORY_SIZE; i++) {
            if (strcmp(filename, fsd.root_dir.entry[i].name) == 0) {
                return SYSERR;
            }
          }

          for(j=0; j<DIRECTORY_SIZE; j++) {
            if(fsd.root_dir.entry[j].inode_num == EMPTY) {
              fsd.root_dir.entry[j].inode_num = inode.id;
              strcpy(fsd.root_dir.entry[j].name,filename);
              fsd.root_dir.numentries++;
              break;
            }
          }
        }
        return fs_open(filepath, O_RDWR);
      }
    }
  }
  else if(mode == O_DIR) {
    int free_inode_blk;
    inode_t inode;
    directory_t new_dir;
    new_dir.numentries = 0;
    for (i = 0; i < DIRECTORY_SIZE; i++) {
      new_dir.entry[i].inode_num = EMPTY;
      memset(new_dir.entry[i].name, 0, FILENAMELEN);
    }

    // search for free inode block
    if(_fs_get_inode_by_num(dev0, fsd.inodes_used, &inode)) {
      for(k=18; k<fsd.nblocks; k++) {
        if(fs_getmaskbit(k) == 0) {
          free_inode_blk = k;
          break;
        }
      }
      if(k >= fsd.nblocks) {
        return SYSERR;
      }
      inode.id = fsd.inodes_used++;
      inode.type =  INODE_TYPE_DIR;
      inode.nlink = 1;
      inode.device = dev0;
      inode.size = 0;
      inode.blocks[0] = free_inode_blk;
      fs_setmaskbit(free_inode_blk);
      bs_bwrite(dev0, free_inode_blk, 0, &new_dir, sizeof(directory_t));

      if(_fs_put_inode_by_num(dev0, inode.id, &inode)) {
        if(final_level_inode_num != -9999) {
          inode_t dir_inode;
          directory_t tmp_dir;
          if(_fs_get_inode_by_num(dev0, final_level_inode_num, &dir_inode)) {
            bs_bread(dev0, dir_inode.blocks[0], 0, &tmp_dir, sizeof(directory_t));
            // If current directory is full
            if(tmp_dir.numentries >= DIRECTORY_SIZE) {
              return SYSERR;
            }
            // If file is already present in final directory
            for (i=0; i<DIRECTORY_SIZE; i++) {
              if (strcmp(filename, tmp_dir.entry[i].name) == 0) {
                  return SYSERR;
              }
            }

            for(j=0; j<DIRECTORY_SIZE; j++) {
              if(tmp_dir.entry[j].inode_num == EMPTY) {
                tmp_dir.entry[j].inode_num = inode.id;
                strcpy(tmp_dir.entry[j].name,filename);
                tmp_dir.numentries = tmp_dir.numentries + 1;
                _fs_put_inode_by_num(dev0, final_level_inode_num, &dir_inode);
                bs_bwrite(dev0, dir_inode.blocks[0], 0, &tmp_dir, sizeof(directory_t));
                break;
              }
            }
          }
        }
        else {
          // If current directory is full
          if(fsd.root_dir.numentries >= DIRECTORY_SIZE) {
            return SYSERR;
          }
          // If file is already present in final directory
          for (i=0; i<DIRECTORY_SIZE; i++) {
            if (strcmp(filename, fsd.root_dir.entry[i].name) == 0) {
                return SYSERR;
            }
          }

          for(j=0; j<DIRECTORY_SIZE; j++) {
            if(fsd.root_dir.entry[j].inode_num == EMPTY) {
              fsd.root_dir.entry[j].inode_num = inode.id;
              strcpy(fsd.root_dir.entry[j].name,filename);
              fsd.root_dir.numentries++;
              break;
            }
          }
        }
        return OK;
      }
    }
  }
  return SYSERR;
}

// =====================================================================
int fs_seek(int fd, int offset) {
  if(isbadfd(fd)) {
    return SYSERR;
  }
  if(oft[fd].state == FSTATE_CLOSED){
    return SYSERR;
  }
  if(offset < 0 || offset > oft[fd].in.size) {
    return SYSERR;
  }
  
  oft[fd].fileptr = offset;
  return OK;
}

// =====================================================================
int fs_read(int fd, void *buf, int nbytes) { 
  if(isbadfd(fd)) {
    return SYSERR;
  }
  else if(oft[fd].state == FSTATE_CLOSED) {
    return SYSERR;
  }
  else if(nbytes < 0) {
    return SYSERR;
  }
  else if(oft[fd].flag == O_WRONLY) {
    return SYSERR;
  }
  
  int i;
  int bytes_read = 0;
  int block_no = (oft[fd].fileptr / fsd.blocksz);
  int block_offset = (oft[fd].fileptr % fsd.blocksz);
  int data_block_read_size = fsd.blocksz - block_offset;

  // Read size goes beyond limit
  if(nbytes > fsd.blocksz * INODEDIRECTBLOCKS) {
    nbytes = fsd.blocksz * INODEDIRECTBLOCKS;
  }

  int bytes_to_read = nbytes;
  for(; bytes_to_read>0; bytes_to_read=bytes_to_read-data_block_read_size) {
    // Get data block
    int data_block = _fs_fileblock_to_diskblock(dev0,fd,block_no++);
    // Final block left to read
    if(bytes_to_read <= data_block_read_size) {
      bs_bread(dev0, data_block, block_offset, buf, bytes_to_read);
      buf = buf + bytes_to_read;
      // Set filepointer to last read location
      oft[fd].fileptr = oft[fd].fileptr + nbytes;
      // Set bytes_read to remaining bytes only in final block
      bytes_read += bytes_to_read;
      return bytes_read;
    }
    else {
      bs_bread(dev0, data_block, block_offset, buf, data_block_read_size);
      buf = buf + data_block_read_size;
      bytes_read += data_block_read_size;
      block_offset = 0;
    }
  }
  bytes_read = nbytes - bytes_to_read;
  return bytes_read;
}

// =====================================================================
int fs_write(int fd, void *buf, int nbytes) {
  if(isbadfd(fd)) {
    return SYSERR;
  }
  else if(oft[fd].state == FSTATE_CLOSED) {
    return SYSERR;
  }
  else if(nbytes < 0) {
    return SYSERR;
  }
  else if(oft[fd].flag == O_RDONLY) {
    return SYSERR;
  }

  int i;
  int bytes_written = 0;
  int block_no = (oft[fd].fileptr / fsd.blocksz);
  int block_offset = (oft[fd].fileptr % fsd.blocksz);
  int data_block_write_size = fsd.blocksz - block_offset;

  // Inode blocks full and can't write anymore
  // Or writing past block limit
  if(oft[fd].fileptr >= fsd.blocksz * INODEDIRECTBLOCKS) {
    return 0;
  }
  // Write size goes beyond limit
  if(nbytes > fsd.blocksz * INODEDIRECTBLOCKS) {
    nbytes = fsd.blocksz * INODEDIRECTBLOCKS;
  }

  int bytes_to_write = nbytes;
  for(; bytes_to_write>0; bytes_to_write=bytes_to_write-data_block_write_size) {
    // Get data block
    int data_block_no = _fs_fileblock_to_diskblock(dev0,fd,block_no);
    int in_blk_no;
    // data is present in inode block (overwriting)
    if(data_block_no > 0) {
      in_blk_no = data_block_no;
    }
    // data is to be stored in inode block
    // hence, allocate an inode block (intialize)
    else if(data_block_no == 0) {
      int j;
      for(j = 18; j < fsd.nblocks; j++) {
        if(fs_getmaskbit(j) == 0) {
            in_blk_no = j;
            break;
        }
      }
      if(j >= fsd.nblocks) {
        return 0;
      }
      oft[fd].in.blocks[block_no] = in_blk_no;
      _fs_put_inode_by_num(dev0, oft[fd].in.id, &oft[fd].in);
      fs_setmaskbit(in_blk_no);
    }
    block_no++;

    // Final block left to write
    if(bytes_to_write <= data_block_write_size) {
      // write inode block to disk
      bs_bwrite(dev0, in_blk_no, block_offset, buf, bytes_to_write);
      buf = buf + bytes_to_write;
      // Set filepointer to last write location
      oft[fd].fileptr = oft[fd].fileptr + nbytes;
      // Change size only if it exceeds the file size i.e. not overwrites intermediate data
      if(oft[fd].fileptr > oft[fd].in.size) {
        oft[fd].in.size = oft[fd].fileptr;
      }
      // write inode structure to disk
      _fs_put_inode_by_num(dev0, oft[fd].in.id, &oft[fd].in);
      // Set bytes_written to remaining bytes only in final block
      bytes_written += bytes_to_write;
      return bytes_written;
    }
    else {
      bs_bwrite(dev0, in_blk_no, block_offset, buf, data_block_write_size);
      buf = buf + data_block_write_size;
      bytes_written += data_block_write_size;
      block_offset = 0;
    }
  }
  bytes_written = nbytes - bytes_to_write;
  return bytes_written;
}

// =====================================================================
int fs_link(char *src_filepath, char* dst_filepath) {
  // Check if file is NULL
  if(src_filepath == NULL || dst_filepath == NULL) {
    return SYSERR;
  }
  // Check if last char is '/'
  char lastChar = dst_filepath[strlen(dst_filepath)-1];
  if(lastChar == '/') {
    return SYSERR;
  }
  int src_final_level_inode_num = fs_check_intermediate_directories(src_filepath);
  if(src_final_level_inode_num == SYSERR) {
    return SYSERR;
  }

  int dst_final_level_inode_num = fs_check_intermediate_directories(dst_filepath);
  if(dst_final_level_inode_num == SYSERR) {
    return SYSERR;
  }
  
  int i,j,k;
  char *src_filename = NULL;
  char *dst_filename = NULL;
  parse_path(src_filepath,&src_filename);
  parse_path(dst_filepath,&dst_filename);
  if(strlen(dst_filename)+1 > FILENAMELEN || strlen(dst_filename) == 0) {
    return SYSERR;
  }
  // Check if filename is NULL
  if (dst_filename == NULL) return SYSERR;

  inode_t src_dir_inode;
  directory_t src_dir;
  inode_t dst_dir_inode;
  directory_t dst_dir;

  if(src_final_level_inode_num != -9999 && dst_final_level_inode_num == -9999) {
    // SRC DIRECTORY CHECKS
    if(_fs_get_inode_by_num(dev0, src_final_level_inode_num, &src_dir_inode)) {
      bs_bread(dev0, src_dir_inode.blocks[0], 0, &src_dir, sizeof(directory_t));
      for (i = 0; i < DIRECTORY_SIZE; i++) {
        // If src file is present in src directory
        if (strcmp(src_filename, src_dir.entry[i].name) == 0) {       
          // If dst directory is full
          if(fsd.root_dir.numentries >= DIRECTORY_SIZE) {
            return SYSERR;
          }
          // If dst file is already present in dst directory
          for (k=0; k<DIRECTORY_SIZE; k++) {
            if (strcmp(dst_filename, fsd.root_dir.entry[k].name) == 0) {
                return SYSERR;
            }
          }

          // Get src file inode num and increase nlink
          inode_t srcfile_inode;
          if(_fs_get_inode_by_num(dev0, src_dir.entry[i].inode_num, &srcfile_inode)) {
            // Add link file to dst directory
            for(j=0; j<DIRECTORY_SIZE; j++) {
              if(fsd.root_dir.entry[j].inode_num == EMPTY) {
                fsd.root_dir.entry[j].inode_num = srcfile_inode.id;
                strcpy(fsd.root_dir.entry[j].name,dst_filename);
                fsd.root_dir.numentries = fsd.root_dir.numentries + 1;

                // Increse src file nlink
                srcfile_inode.nlink += 1;
                _fs_put_inode_by_num(dev0, src_dir.entry[i].inode_num, &srcfile_inode);

                _fs_put_inode_by_num(dev0, src_final_level_inode_num, &src_dir_inode);
                bs_bwrite(dev0, src_dir_inode.blocks[0], 0, &src_dir, sizeof(directory_t));
                return OK;
              }
            }
          }
        }
      }
    }
    return SYSERR;
  }
  else if(src_final_level_inode_num == -9999 && dst_final_level_inode_num != -9999) {
    // SRC DIRECTORY CHECKS
    for (i = 0; i < DIRECTORY_SIZE; i++) {
      // If src file is present in src directory
      if (strcmp(src_filename, fsd.root_dir.entry[i].name) == 0) {
        // DST DIRECTORY CHECKS
        if(_fs_get_inode_by_num(dev0, dst_final_level_inode_num, &dst_dir_inode)) {
          bs_bread(dev0, dst_dir_inode.blocks[0], 0, &dst_dir, sizeof(directory_t));
          // If dst directory is full
          if(dst_dir.numentries >= DIRECTORY_SIZE) {
            return SYSERR;
          }
          // If dst file is already present in dst directory
          for (k=0; k<DIRECTORY_SIZE; k++) {
            if (strcmp(dst_filename, dst_dir.entry[k].name) == 0) {
                return SYSERR;
            }
          }

          // Get src file inode num and increase nlink
          inode_t srcfile_inode;
          if(_fs_get_inode_by_num(dev0, fsd.root_dir.entry[i].inode_num, &srcfile_inode)) {
            // Add link file to dst directory
            for(j=0; j<DIRECTORY_SIZE; j++) {
              if(dst_dir.entry[j].inode_num == EMPTY) {
                dst_dir.entry[j].inode_num = srcfile_inode.id;
                strcpy(dst_dir.entry[j].name,dst_filename);
                dst_dir.numentries = dst_dir.numentries + 1;

                _fs_put_inode_by_num(dev0, dst_final_level_inode_num, &dst_dir_inode);
                bs_bwrite(dev0, dst_dir_inode.blocks[0], 0, &dst_dir, sizeof(directory_t));

                // Increse src file nlink
                srcfile_inode.nlink += 1;
                _fs_put_inode_by_num(dev0, fsd.root_dir.entry[i].inode_num, &srcfile_inode);
                return OK;
              }
            }
          }
        }
      }
    }
    return SYSERR;
  }
  else if(src_final_level_inode_num != -9999 && dst_final_level_inode_num != -9999) {
    // 1. Check for file in src directory
    // 2. If src file present in src, check for dst file in dst diirectory
    // 3. Grab src file inode num
    // 4. If file not present in dst directory, add link from src file

    // SRC DIRECTORY CHECKS
    if(_fs_get_inode_by_num(dev0, src_final_level_inode_num, &src_dir_inode)) {
      bs_bread(dev0, src_dir_inode.blocks[0], 0, &src_dir, sizeof(directory_t));
      for (i = 0; i < DIRECTORY_SIZE; i++) {
        // If src file is present in src directory
        if (strcmp(src_filename, src_dir.entry[i].name) == 0) {
          // DST DIRECTORY CHECKS
          if(_fs_get_inode_by_num(dev0, dst_final_level_inode_num, &dst_dir_inode)) {
            bs_bread(dev0, dst_dir_inode.blocks[0], 0, &dst_dir, sizeof(directory_t));
            // If dst directory is full
            if(dst_dir.numentries >= DIRECTORY_SIZE) {
              return SYSERR;
            }
            // If dst file is already present in dst directory
            for (k=0; k<DIRECTORY_SIZE; k++) {
              if (strcmp(dst_filename, dst_dir.entry[k].name) == 0) {
                  return SYSERR;
              }
            }

            // Get src file inode num and increase nlink
            inode_t srcfile_inode;
            if(_fs_get_inode_by_num(dev0, src_dir.entry[i].inode_num, &srcfile_inode)) {
              // Add link file to dst directory
              for(j=0; j<DIRECTORY_SIZE; j++) {
                if(dst_dir.entry[j].inode_num == EMPTY) {
                  dst_dir.entry[j].inode_num = srcfile_inode.id;
                  strcpy(dst_dir.entry[j].name,dst_filename);
                  dst_dir.numentries = dst_dir.numentries + 1;
                  _fs_put_inode_by_num(dev0, dst_final_level_inode_num, &dst_dir_inode);
                  bs_bwrite(dev0, dst_dir_inode.blocks[0], 0, &dst_dir, sizeof(directory_t));
                  
                  // Increse src file nlink
                  srcfile_inode.nlink += 1;
                  _fs_put_inode_by_num(dev0, src_dir.entry[i].inode_num, &srcfile_inode);

                  _fs_put_inode_by_num(dev0, src_final_level_inode_num, &src_dir_inode);
                  bs_bwrite(dev0, src_dir_inode.blocks[0], 0, &src_dir, sizeof(directory_t));
                  return OK;
                }
              }
            }
          }
        }
      }
    }
    return SYSERR;
  }
  else if(src_final_level_inode_num == -9999 && dst_final_level_inode_num == -9999) {
    // If directory is full
    if(fsd.root_dir.numentries >= DIRECTORY_SIZE) {
      return SYSERR;
    }
    // Check for duplicate destination filename
    for (k = 0; k < DIRECTORY_SIZE; k++) {
      if(strcmp(fsd.root_dir.entry[k].name,dst_filename) == 0) {
        return SYSERR;
      }
    }

    int i,j;
    inode_t node;
    for (i = 0; i < DIRECTORY_SIZE; i++) {
      if (strcmp(src_filename, fsd.root_dir.entry[i].name) == 0) {
        _fs_get_inode_by_num(dev0, fsd.root_dir.entry[i].inode_num, &node);
        node.nlink = node.nlink + 1;
        _fs_put_inode_by_num(dev0, fsd.root_dir.entry[i].inode_num, &node);
        
        for(j=0; j<DIRECTORY_SIZE; j++) {
          if(fsd.root_dir.entry[j].inode_num == EMPTY) {
            fsd.root_dir.entry[j].inode_num = node.id;
            strcpy(fsd.root_dir.entry[j].name,dst_filename);
            fsd.root_dir.numentries++;
            return OK;
          }
        }
      }
    }
    return SYSERR;
  }
}

// =====================================================================
int fs_unlink(char *filepath) {
  if(filepath == NULL) {
    return SYSERR;
  }
  int final_level_inode_num = fs_check_intermediate_directories(filepath);
  if(final_level_inode_num == SYSERR) {
    return SYSERR;
  }
  
  int i,j,k;
  char *filename = NULL;
  inode_t dir_inode;
  directory_t tmp_dir;
  parse_path(filepath,&filename);
  
  if(final_level_inode_num != -9999) {
    if(_fs_get_inode_by_num(dev0, final_level_inode_num, &dir_inode)) {
      bs_bread(dev0, dir_inode.blocks[0], 0, &tmp_dir, sizeof(directory_t));
      for (i = 0; i < DIRECTORY_SIZE; i++) {
        // If file is present in directory
        if (strcmp(filename, tmp_dir.entry[i].name) == 0) {
          // Get src file inode num and decrease nlink
          inode_t tmp_fileinode;
          if(_fs_get_inode_by_num(dev0, tmp_dir.entry[i].inode_num, &tmp_fileinode)) {
            // Multiple links present
            if (tmp_fileinode.nlink > 1) {
              tmp_fileinode.nlink -= 1;
              _fs_put_inode_by_num(dev0, tmp_dir.entry[i].inode_num, &tmp_fileinode);      

              // remove entry from directory
              tmp_dir.entry[i].inode_num = EMPTY;
              memset(tmp_dir.entry[i].name, 0, FILENAMELEN);
              tmp_dir.numentries = tmp_dir.numentries - 1;

              _fs_put_inode_by_num(dev0, final_level_inode_num, &dir_inode);
              bs_bwrite(dev0, dir_inode.blocks[0], 0, &tmp_dir, sizeof(directory_t));
              return OK;
            }
            // Last link present - delete file/dir
            else if(tmp_fileinode.nlink == 1) {
              if(tmp_fileinode.type == INODE_TYPE_DIR) {
                directory_t dir_to_delete;
                bs_bread(dev0, tmp_fileinode.blocks[0], 0, &dir_to_delete, sizeof(directory_t));
                if(dir_to_delete.numentries > 0) {
                  return SYSERR;
                }
              }
              for (j=0; j<INODEDIRECTBLOCKS; j++) {
                int datablk=_fs_fileblock_to_diskblock(dev0,tmp_fileinode.id,j);
                fs_clearmaskbit(datablk);
              }
              
              // empty inode for reuse
              tmp_fileinode.id     = EMPTY;
              tmp_fileinode.type   = 0;
              tmp_fileinode.nlink  = 0;
              tmp_fileinode.device = 0;
              tmp_fileinode.size   = 0;
              memset(tmp_fileinode.blocks, 0, sizeof(tmp_fileinode.blocks));

              _fs_put_inode_by_num(dev0, tmp_dir.entry[i].inode_num, &tmp_fileinode);
              // fsd.inodes_used--;

              // remove entry from root directory
              tmp_dir.entry[i].inode_num = EMPTY;
              memset(tmp_dir.entry[i].name, 0, FILENAMELEN);
              tmp_dir.numentries = tmp_dir.numentries - 1;

              _fs_put_inode_by_num(dev0, final_level_inode_num, &dir_inode);
              bs_bwrite(dev0, dir_inode.blocks[0], 0, &tmp_dir, sizeof(directory_t));
              return OK;
            }
          }
        }
      }
    }
  }
  else {
    inode_t node;
    for (i = 0; i < DIRECTORY_SIZE; i++) {
      if(strcmp(filename,fsd.root_dir.entry[i].name) == 0) {
        _fs_get_inode_by_num(dev0, fsd.root_dir.entry[i].inode_num, &node);
        // Multiple links present
        if (node.nlink > 1) {
          node.nlink -= 1;
          _fs_put_inode_by_num(dev0, fsd.root_dir.entry[i].inode_num, &node);      

          // remove entry from root directory
          fsd.root_dir.entry[i].inode_num = EMPTY;
          memset(fsd.root_dir.entry[i].name, 0, FILENAMELEN);
          fsd.root_dir.numentries--;
          return OK;
        }
        // Single link present
        else if(node.nlink == 1) {
          if(node.type == INODE_TYPE_DIR) {
            directory_t dir_to_delete;
            bs_bread(dev0, node.blocks[0], 0, &dir_to_delete, sizeof(directory_t));
            if(dir_to_delete.numentries > 0) {
              return SYSERR;
            }
          }
          for (j=0; j<INODEDIRECTBLOCKS; j++) {
            int datablk=_fs_fileblock_to_diskblock(dev0,node.id,j);
            fs_clearmaskbit(datablk);
          }
          
          // empty inode for reuse
          node.id     = EMPTY;
          node.type   = 0;
          node.nlink  = 0;
          node.device = 0;
          node.size   = 0;
          memset(node.blocks, 0, sizeof(node.blocks));

          _fs_put_inode_by_num(dev0, fsd.root_dir.entry[i].inode_num, &node);

          // remove entry from root directory
          fsd.root_dir.entry[i].inode_num = EMPTY;
          memset(fsd.root_dir.entry[i].name, 0, FILENAMELEN);
          fsd.root_dir.numentries--;
          return OK;
        }
      }
    }
  }
  return SYSERR;
}

#endif /* FS */