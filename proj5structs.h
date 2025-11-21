#ifndef UMBC_CMSC_421_PROJ5
#define UMBC_CMSC_421_PROJ5

#define FUSE_USE_VERSION 31

#include <fuse3/fuse.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>

// SimpleFS Superblock Structure.
// DO NOT MODIFY!
typedef struct __attribute__((packed)) simfs_superblock {
  uint8_t     signature[16];
  uint16_t    version;
  uint16_t    num_files;
  uint16_t    magic;
} simfs_superblock_t ;


// SimpleFS File Structure
// DO NOT MODIFY!
typedef struct __attribute__((packed)) simfs_file {
  char      name[24];
  uint8_t   inuse;
  uint16_t  uid;
  uint16_t  gid;
  uint16_t  date_created[3];
  uint16_t  data_bytes;
  char      data[1024];
} simfs_file_t;


#endif
