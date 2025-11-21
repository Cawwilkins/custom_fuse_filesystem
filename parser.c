#include "proj5structs.h"

typedef struct img {
  simfs_superblock_t msb;
  simfs_file_t files[256];
  simfs_superblock_t bsb;
} img_t;

int main() {
  printf("->> Project: SimpleFS Image Parser\n");
  printf("->> Author: AJ Angarita (aangar) :3\n");
  printf("->> Complaints/Issues?\n");
  printf("--->> uhhhhh ... monster break. yep. at the store. brb.\n\n");

  int sb = sizeof(simfs_superblock_t) * 2;
  int file = sizeof(simfs_file_t) * 256;
  int totals = sb + file;

  int fd = open("simplefs.img", O_RDONLY); 
  if (fd < 0) {
    printf("ERROR: failed to open simplefs.img\n");
    return 1;
  }

  uint8_t bytes[totals];
  ssize_t r = read(fd, &bytes, totals);
  if (r != totals) {
    printf("ERROR: parser failed to read all bytes.\n");
    printf("INFO: is your file 271144 bytes in size?\n");
    return 1;
  }

  close(fd);

  img_t* img = (img_t*) &bytes;

  printf("****** simplefs.img information ******\n");
  printf("|-> Main Superblock\n");
  printf("|-|-> .signature => %s\n", img->msb.signature);
  printf("|-|-> .version => %d\n", ntohs(img->msb.version));
  printf("|-|-> .num_files => %d\n", ntohs(img->msb.num_files));
  printf("|-|-> .magic => %d\n\n", ntohs(img->msb.magic));

  printf("|-> Backup Superblock\n");
  printf("|-|-> .signature => %s\n", img->bsb.signature);
  printf("|-|-> .version => %d\n", ntohs(img->bsb.version));
  printf("|-|-> .num_files => %d\n", ntohs(img->bsb.num_files));
  printf("|-|-> .magic => %d\n\n", ntohs(img->bsb.magic));


  printf("|-> File Overview\n");
  simfs_file_t* simfile;
  for (int i = 0; i < 256; i++) {
    simfile = &img->files[i];
    if (simfile->inuse == 1) {
      printf("|-|-> Found file %d inuse.\n", i);
      printf("|-|-|-> .data_bytes: %d\n", ntohs(simfile->data_bytes));

      /**
       *  NOTE: ntohs() is "network-to-host", aka converting from network order
       *        to the hosts order. this will "undo" htons.
       * */
      uint16_t year = ntohs(simfile->date_created[0]);
      uint16_t month = ntohs(simfile->date_created[1]);
      uint16_t day = ntohs(simfile->date_created[2]);
      printf("|-|-|-> .date_created: %d-%d-%d\n", year, month, day);
      printf("|-|-|-> .uid: %d\n", ntohs(simfile->uid));
      printf("|-|-|-> .gid: %d\n\n", ntohs(simfile->gid));
    }
  }

  return 0;
}
