#include "proj5structs.h"

typedef struct img {
  simfs_superblock_t msb;
  simfs_file_t files[256];
  simfs_superblock_t bsb;
} img_t;


// TODO: fill out each of these!
static struct fuse_operations simfs_operations = {
  .init       = fs_init,
  .getattr    = fs_getattr,
  .readdir    = fs_readdir,
  .read       = fs_read,
  .open       = fs_open,
  .create     = NULL, 		// Checkpoint 2
  .write      = NULL,  		// Final
  .unlink     = NULL  		// Final
  // Saving image on unmount    // Final
};

char* path_to_image = NULL;
img_t* main_image = NULL;
char* img_buffer = NULL;


simfs_file_t helper_file_finder(const char *path){


void *fs_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
	ssize_t maxBytes = 271660; // From spec

	// Open the image
	int didOpen = open(path_to_image, 0_RDWR);
	img_buffer = malloc(maxBytes);

	// Read the file
	read(didOpen, img_buffer, max_bytes);
	close(didOpen);

	// Cast as image and store file
	main_image = (img_t*) img_buffer;
}

simfs_file_t helper_file_finder(const char *path){
	char *fileName = NULL;
        simfs_file_t *compareFile = NULL;
        char xorName[25];

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
                 if (strcmp(xorName, fileName) == 0){
			return compareFile;
		}
	}
	return NULL;
}


int *fs_getattr(const char *path, struct stat *stbuf, strcut fuse_file_info *fi) {
	simfs_file_t *filePtr = NULL;

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
	if (strcmp(path,"/") == 0) {
		stbuf->st_mode = S_IFDIR;
		stbuf->st_nlink = 2;
		return 0;
	// File
	} else {
		// Names match and file not in use
		filePtr = helper_file_finder(path);
		if (filePtr != NULL) {
			stbuf->st_mode = S_IFREG;
			stbuf->st_nlink = 1;
			stbuf->st_size = ntohs(compareFile->data_bytes);
			return 0;
		}
		return 1;
	}
	return 1;
}

int *fs_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *, enum fuse_readdir_flags) {
        char xorName[25];

	// Make sure it is just a path command
	if (strcmp(path, "/") != 0) {
                return 0;
        }

	filler(buffer, ".",  NULL, 0, 0);
	filler(buffer, "..", NULL, 0, 0);

        // Loop through each file
        for (int i = 0; i < 256; i++) {
                 simfs_file_t *compareFile = &main_image->files[i];

                 // Check if file in use before xoring, name not defined yet
                 if (compareFile->inuse == 0){
                        continue;
                 }

		 // Set attributes of file
                 struct stat* std_buff;
	         std_buff->st_dev = 0;
        	 std_buff->st_ino = 0;
        	 std_buff->st_nlink = 0;
        	 std_buff->st_rdev = 0;
        	 std_buff->st_blksize = 0;
        	 std_buff->st_blocks = 0;
                 std_buff->st_uid = compareFile->uid;
                 std_buff->st_gid = compareFile->gid;
                 std_buff->st_size = ntohs(compareFile->data_bytes);
                 std_buff->st_mode = S_IFREG;


                 // Loop through each index and xor to return to original name
                 for (int j = 0; j < 24; j++) {
                        xorName[j] = compareFile->name[j] ^ magic_value;
                 }
                 xorName[24] = '\0';

                 filler(buffer, xorName, &std_buff, 0, 0);
        }
        return 0;
}


int *fs_read(const char *path, char *buff, size_t size, off_t offset, struct fuse_file_info *file_info){
	simfs_file_t *filePtr = NULL;
	int fileSize = 0;
	int numCopy = 0;
	int currentIndex = 0;

	// Validation and finding file
	if (strcmp(path, "/") == 0) {
		return 0;
	} else {
		fileptr = helper_file_finder(path);
		if (filePtr == NULL) {
			return 0;
		}
	}

	// Converting file size and calculating how much to move offset
	file_size = ntohs(filePtr->data_bytes);
	if (fileSize <= offset) {
		return 0;
	} else {
		if (size < (fileSize - offset)) {
			numCopy = size;
		} else {
			numCopy = fileSize - offset;
		}
		// Copy each index and translate it
		for (int i = 0; i < num_copy; i++) {
			currentIndex = offset + i;
			buff[i] = filePtr->data[currentIndex] ^ magic_value;
		}
	}
	return numCopy;
}

int *fs_open(const char *, struct fuse_file_info *){
	return 0;
}

int* fs_create(const char *path, mode_t mode, struct fuse_file_info *file_info){
	simfs_file_t *filePtr = NULL;
	int i;
	char xor_name[25];
	char file_name[25]

	// To get rid of leading slash
	if (path[0] = '/') {
		file_name = path + 1;
	} else {
		file_name = path;
	}

	filePtr = helper_file_finder(path);

	// No file with that name yet
	if (filePtr == NULL) {
		// Find first index where we can store files
		while (&main_image->files[i] != NULL) {
			i++;
		}

		// Loop through each index and xor to return to original name
                for (int j = 0; j < 24; j++) {
		       // J + 1 because of leading /
                       xorName[j] = path[j+1] ^ magic_value;
                }
                xorName[24] = '\0';

		created_file = &main_image->files[i];
		
                return 0;
        }
        return 1;
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
