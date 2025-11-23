#include "proj5structs.h"

typedef struct img {
  simfs_superblock_t msb;
  simfs_file_t files[256];
  simfs_superblock_t bsb;
} img_t;


// TODO: fill out each of these!
static struct fuse_operations simfs_operations = {
  .init       = fs_init, // Check 1
  .getattr    = fs_getattr, // Check 1
  .readdir    = NULL, 	// Check 1
  .read       = NULL,	 // Check 1
  .open       = fs_open, // Check 1
  .create     = NULL, // Check 2
  .write      = NULL, // Final
  .unlink     = NULL  // Final
  // Saving image on unmount  // Final
};

char* path_to_image = NULL;
img_t* main_image = NULL;
char* img_buffer = NULL;

void *fs_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
	ssize_t maxBytes = 271660; // From spec

	int didOpen = open(path_to_image, 0_RDWR);
	img_buffer = malloc(maxBytes);

	read(didOpen, img_buffer, max_bytes);
	close(didOpen);

	main_image = (img_t*) img_buffer;
}

int *fs_getattr(const char *path, struct stat *stbuf, strcut fuse_file_info *fi){

	char *fileName = NULL;
	simfs_file_t *compareFile = NULL;
	char xorName[25];

	// Set Stat
	stbuf->st_dev = 0;
	stbuf->st_ino = 0;
	stbuf->st_mode = 0;
	stbuf->st_nlink = 0;
	stbuf->st_uid = 0;
	stbuf->st_gid = 0;
	stbuf->st_rdev = 0;
	stbuf->st_size = 0;
	stbuf->st_blksize = 0;
	stbuf->st_blocks = 0;

	// Directory
	if (strcmp(path,"/") == 0) {		// does this need to be more robust like cd /asdf/asdf/adfs (multi slash)
		stbuf->st_mode = S_IFDIR;
		stbuf->st_nlink = 2;
		return 0;
	// File
	} else {
		// Get rid of the slash
		fileName = path + 1;
		for (int i = 0; i < 256; i++) {
			compareFile = &main_image->files[i];

			// Check if file in use before xoring, name not defined yet
			if (compareFile->inuse == 0){
				continue;
			}

			// Loop through each index and xor to return to original name
			for (int j = 0; j < 24; j++) {
				xorName[j] = compareFile->name[j] ^ magic_value;
			}
			xorName[24] = '\0';

			// Names match and file not in use
			if (stcmp(xorName, fileName) == 0) {
				stbuf->st_mode = S_IFREG;
				stbuf->st_nlink = 1;
				stbuf->st_size = ntohs(compareFile->data_bytes);
				return 0;
			}
		}
		return 1;
	}
	return 1;
}

int *fs_open(const char *, struct fuse_file_info *){
	return 0;
}


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
	if (argv[1] == NULL) {
		return 1;
	}
	path_to_image = argv[1];

  // FUSE doesnt care about the filesystem image.
  // hence, knowing that the first argument isnt valid we skip over it.
  return fuse_main(--argc, ++argv, &simfs_operations, NULL);
}
