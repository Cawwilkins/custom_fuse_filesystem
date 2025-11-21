#include "proj5structs.h"

// TODO: fill out each of these!
static struct fuse_operations simfs_operations = {
  .init       = NULL,
  .getattr    = NULL,
  .readdir    = NULL,
  .read       = NULL,
  .open       = NULL,
  .create     = NULL,
  .write      = NULL,
  .unlink     = NULL
};

/**
 *  SimpleFS Main Loop. Objectives:
 *    1. load the file via the first argument passed to this program.
 *    2. store it somewhere in your program.
 *    3. fuse_main() will tell FUSE to start and readies your directory.
 *    4. this should return 1 if the filesystem image does not exist.
 *
 *  Syntax:
 *    simplefs_main <your_fs.img> <fuse_flags> <mount_point>
 *
 *  Where:
 *    - <your_fs.img> is the generated filesystem image you made.
 *    - <fuse_flags> are flags that FUSE recognizes. some may be -f for "foreground", and -d for "debug"
 *    - <mount_point> is a directory on your machine.
 * */
int main(int argc, char* argv[]) {

  // FUSE doesnt care about the filesystem image.
  // hence, knowing that the first argument isnt valid we skip over it.
  return fuse_main(argc--, argv++, &simfs_operations, NULL);
}
