#include "proj5structs.h"

// simple flag for easy changing on generation
#define USE_TESTING_VALUES 0

int main() {
  srand(time(NULL));
  uint8_t max_val = 0xff;
  int magic = rand() % (max_val - 2) + 1;
  printf("Generated Magic: %d\n", magic);

  // refer lower to what htons() does. parser.c shows you how to undo it.
  simfs_superblock_t sb = {
    .signature      = "single-level-fs\0",
    .version        = htons(1),
    .num_files      = htons(0),
    .magic          = htons(magic) 
  };

  simfs_file_t sample_file = {
    .inuse      = 0,  // IMPORTANT: you do NOT need to call htons on this field.
    .uid        = 0,  // you will call it for uid, gid, data_bytes,
    .gid        = 0,  // but zeroes will be unchanged between the two :D
    .data_bytes = 0,
  };
  memset(sample_file.name, 0, 24);
  memset(sample_file.data, 0, 1024);

  if (USE_TESTING_VALUES == 1) {
    // these warnings can eat rocks.
    uint16_t* yyyy = &sample_file.date_created[0];
    uint16_t* mm = &sample_file.date_created[1];
    uint16_t* dd = &sample_file.date_created[2];

    /**
     *  NOTE: htons() is short for "host-to-network" and converts endianness.
     *        numbers (int, uint*_t) should be stored like so such that they can be converted back.
     *        otherwise, you will NOT get the values you expect.
     * */
    *yyyy                   = htons(2025);
    *mm                     = htons(6);
    *dd                     = htons(10);
    sample_file.gid         = htons(0xef);
    sample_file.uid         = htons(0xaa);
    sample_file.data_bytes  = htons(69); //heh, nice.
    sample_file.inuse = 1;    // set inuse so that the parser will show this file.
    strcpy(sample_file.name, "testing-fname");
  }


  FILE* fd = fopen("simplefs.img", "wb");
  if (fd == NULL) {
    printf("ERROR: builder was unable to open the stream. Do you have permissions for this directory?\n");
    fclose(fd);
    return 1;
  }

  fclose(fd);

  int file = open("simplefs.img", O_RDWR);

  if (lseek(file, 0, SEEK_SET) < 0) {
    printf("ERROR: builder failed to set seek to zero.\n");
    close(file);
    return 1;
  }

  if (write(file, &sb, sizeof(simfs_superblock_t)) < 0) {
    printf("ERROR: builder failed to write main superblock.\n");
    close(file);
    return 1;
  }

  for (int i = 0; i < 256; i++) {
    if (write(file, &sample_file, sizeof(simfs_file_t)) < 0) {
      printf("ERROR: builder failed to write file %d, exiting.\n", i + 1);
      close(file);
      i = 0xffff;
      return 1;
    }
  }

  if (write(file, &sb, sizeof(simfs_superblock_t)) < 0) {
    printf("ERROR: builder failed to write main superblock.\n");
    close(file);
    return 1;
  }


  return 0;
}
