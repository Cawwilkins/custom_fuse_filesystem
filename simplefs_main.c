#include "proj5structs.h"
#include <errno.h>
#include <limits.h>

// Packed because thats how the other structs were declared
typedef struct __attribute__((packed)) img {
  simfs_superblock_t msb;
  simfs_file_t files[256];
  simfs_superblock_t bsb;
} img_t;


// Globals
char* path_to_image = NULL;
img_t* main_image = NULL;
char* img_buffer = NULL;
uint16_t magic_value = 0;

simfs_file_t *helper_file_finder(const char *path){

	// Set variables
	char fileName[24];
        simfs_file_t *compareFile = NULL;
        char fixedName[24];

	// Empty fileName and the corrected name so no trash values
	memset(fileName, 0, 24);
	memset(fixedName, 0, 24);

	// To get rid of leading slash and null terminate
        if (path[0] == '/') {
                strncpy(fileName, path + 1, 23);
        } else {
                strncpy(fileName, path, 23);
        }
        fileName[23] ='\0';

	// Loop through each index and compare its fixed name to given name to see if it is the desired file
        for (int i = 0; i < 256; i++) {
		// Reset FixedName each loop
		memset(fixedName, 1, 24);
		// Set compareFile to current index of files
        	compareFile = &main_image->files[i];

                // Check if file in use before xoring and if not skip this loop
                if (compareFile->inuse == 0) {
     	        	continue;
                }

                // Loop through each index and xor to return to original name
                for (int j = 0; j < 23; j++) {
			// If the compareFile is at a null terminator, copy it, and break loop
                	if (compareFile->name[j] == '\0') {
				fixedName[j] = '\0';
				break;
			}
			fixedName[j] = compareFile->name[j] ^ magic_value;
		}

                // Names match and file in use
                if (strcmp(fixedName, fileName) == 0) {
			return compareFile;
		}
	}
	return NULL;
}


void *fs_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {

	// Initialize vars (like how AJ did in parser)
	int sb = sizeof(simfs_superblock_t) * 2;
	int file = sizeof(simfs_file_t) * 256;
	int totals = sb + file;

	// Void unneeded vars
	(void)conn;
	(void)cfg;

        // Open the image
        int didOpen = open(path_to_image, O_RDWR);
	if (didOpen < 0) {
		// Some issue with opening
		return NULL;
	}

	// Declare bytes as an array w the total bytes as number of entries
	uint8_t bytes[totals];

	// Read and close file
        ssize_t r = read(didOpen, &bytes, totals);
	if (r != totals) {
		// Something wrong with read
		return NULL;
	}
	close(didOpen);

        // Dynamically allocate main_image
	main_image = malloc(sizeof(img_t));
	uint8_t *offset = bytes;

	// Copy the size of the superblock into msb
	memcpy(&main_image->msb, offset, sizeof(simfs_superblock_t));
	// Increment offset to be at correct position for next block of data
	offset+= sizeof(simfs_superblock_t);

	// Copy size of files into files[]
	memcpy(&main_image->files, offset, sizeof(simfs_file_t) * 256);
        // Increment offset to be at correct pos for next block of data
	offset+= sizeof(simfs_file_t) * 256;

	// Copy size of superblock into bsb
	memcpy(&main_image->bsb, offset, sizeof(simfs_superblock_t));
	// Dont need to finish offset ...i dont think... but I just added it for completeness sake
	offset+= sizeof(simfs_superblock_t);

	// Store the magic value for xor-ing later
	magic_value = ntohs(main_image->msb.magic);

	// Validate superblocks
	if (strcmp((char *)main_image->msb.signature, "single-level-fs") != 0) {
		memcpy(main_image->msb.signature, main_image->bsb.signature, 16);
	}
	if (main_image->msb.num_files != main_image->bsb.num_files) {
		main_image->msb.num_files = main_image->bsb.num_files;
	}
	if (main_image->msb.magic != main_image->bsb.magic) {
                main_image->msb.magic = main_image->bsb.magic;
        }
	if (main_image->msb.version != main_image->bsb.version) {
                main_image->msb.version = main_image->bsb.version;
        }

	// Write any changes to msb back to disk
	off_t off_offset = 0;
        didOpen = open(path_to_image, O_RDWR);
        lseek(didOpen, off_offset, SEEK_SET);
        write(didOpen, &main_image->msb, sizeof(simfs_superblock_t));
	close(didOpen);

	return NULL;
}


int fs_utimens(const char *path, const struct timespec tv[2], struct fuse_file_info *file_info) {

	// Void unneeded vars
	(void)path;
	(void)tv;
	(void)file_info;

	// Return 0 like AJ said
	return 0;
}


int fs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *file_info) {

	// Set vars
	simfs_file_t *filePtr = NULL;

	// Void unneeded vars
	(void)file_info;

	// Set stat structure
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
	time_t now = time(NULL);
	stbuf->st_atime = now;
	stbuf->st_mtime = now;
	stbuf->st_ctime = now;

	// If it is a directory path given
	if (strcmp(path,"/") == 0) {
		// Set mode to have normal directory permissions
		stbuf->st_mode = S_IFDIR | 0755;
		// Num hard links, parent and self
		stbuf->st_nlink = 2;
		return 0;

	// If it is a filepath given
	} else {
		filePtr = helper_file_finder(path);

		// If duplicate exists and it is in use
		if (filePtr != NULL && filePtr->inuse == 1) {
			// Set mode to have normal file permissions
			stbuf->st_mode = S_IFREG | 0644;
			// Num hard links, just parent
			stbuf->st_nlink = 1;
			// Set params
			stbuf->st_size = ntohs(filePtr->data_bytes);
			stbuf->st_uid = ntohs(filePtr->uid);
			stbuf->st_gid = ntohs(filePtr->gid);
			return 0;
		}
	}
	// File not found
	return -ENOENT;
}


int fs_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *file_inf, enum fuse_readdir_flags flags) {

	// Declare variables and void uneeded ones
	char xorName[25];
	(void)file_inf;
	(void)flags;
	(void)offset;

	// Make sure it is just a path command
	if (strcmp(path, "/") != 0) {
                return 0;
        }


	// Admittedly I don't exactly know what these two lines do but they were in the example
	// and the code doesnt work without them
	filler(buffer, ".",  NULL, 0, 0);
	filler(buffer, "..", NULL, 0, 0);

        // Loop through each file
        for (int i = 0; i < 256; i++) {
                 simfs_file_t *compareFile = &main_image->files[i];

                 // Check if file in use before xoring - if not, just do next loop
                 if (compareFile->inuse == 0){
                        continue;
                 }

		 // Set attributes of file assuming it is in use
                 struct stat std_buff;

		 // Set everything to 0
		 memset(&std_buff, 0, sizeof(struct stat));

		 // Set Stat struct params
	         std_buff.st_dev = 0;
        	 std_buff.st_ino = 0;
        	 std_buff.st_rdev = 0;
        	 std_buff.st_blksize = 0;
        	 std_buff.st_blocks = 0;
		 std_buff.st_nlink = 1;
		 std_buff.st_mtime = time(NULL);
		 std_buff.st_ctime = time(NULL);
		 std_buff.st_atime = time(NULL);
                 std_buff.st_uid = ntohs(compareFile->uid);
                 std_buff.st_gid = ntohs(compareFile->gid);
                 std_buff.st_size = ntohs(compareFile->data_bytes);
                 std_buff.st_mode = S_IFREG | 0644;


                 // Loop through each index and xor to return to original name
                 for (int j = 0; j < 24; j++) {
                        xorName[j] = compareFile->name[j] ^ magic_value;
                	if (compareFile->name[j] == '\0') {
				xorName[j] = '\0';
				break;
			}
		 }
                 filler(buffer, xorName, &std_buff, 0, 0);
        }
        return 0;
}


int fs_read(const char *path, char *buff, size_t size, off_t offset, struct fuse_file_info *file_info){

	// Setting variables and voiding unneeded ones
	simfs_file_t *filePtr = NULL;
	size_t numCopy = 0;
	(void)file_info;

	// If path is just / then return
	if (strcmp(path, "/") == 0) {
		return 0;
	}

	// Get a pointer to the appropriate file
	filePtr = helper_file_finder(path);

	// If filePtr is Null, the file wasnt found so return
	if (filePtr == NULL) {
		return 0;
	}

	// Converting size and calculate offset
	uint16_t curr_size = ntohs(filePtr->data_bytes);

	// If the offset is bigger than the size, there has been some error so return
	if (offset >= curr_size) {
		return 0;

	// Otherwise continue
	} else {
		if (offset + size > curr_size) {
			numCopy = curr_size - offset;
		} else {
			numCopy = size;
		}

		// Copy each index and translate it
		for (size_t i = 0; i < numCopy; i++) {
			buff[i] = filePtr->data[offset + i] ^ magic_value;
		}
	}
	return numCopy;
}


int fs_open(const char *path, struct fuse_file_info *file_inf){

	// Void unneeded variables
	(void)path;
	(void)file_inf;

	// Per the spec, just return 0
	return 0;
}


int fs_create(const char *path, mode_t mode, struct fuse_file_info *file_info){

	// Set Variables
	simfs_file_t *filePtr = NULL;
	int i = 0;
	char xor_name[24];
	char file_name[24];
	int counter = 0;

	// Void unneeded params
	(void)mode;
	(void)file_info;

	// Zero xor_name - I was getting some strange errors from not doing this
	memset(xor_name, 0, 24);

	// To get rid of leading slash and set file name
	if (path[0] == '/') {
		// If name too long return
		if (strlen(path) > 24) {
			return -ENOENT;
		}
		strncpy(file_name, path + 1, 23);

	// No leading slash
	} else {
		// If name too long return
		if (strlen(path) > 24) {
			return -ENOENT;
		}
		strncpy(file_name, path, 23);
	}
	// Null terminate for rest of program
	file_name[23] ='\0';

	// Confirm that pointer is null (file doesnt exist)
	filePtr = helper_file_finder(path);

	// No file yet so make one
	if (filePtr == NULL) {

		// Find first index where we can store files
		while (i < 256 && main_image->files[i].inuse == 1) {
			i++;
		}

		// If we get through all of our files, return
		if (i == 256) {
			return -ENOENT;
		}

		// Loop through each index and xor to encode original name
                for (int j = 0; j < 23; j++) {

			// Counter keeps track of how many indexes we have written
			counter += 1;

			// Stop when original file name is null terminated
			if (file_name[j] == '\0') {
				xor_name[j] = '\0';
				break;
			}

			xor_name[j] = file_name[j] ^ magic_value;
                }

		// Create a pointer to location of first place without file
		simfs_file_t *created_file = &main_image->files[i];

		// Clear name so no bad vals
		memset(created_file->name, 0, 24);

		// Set attributes of file
		memcpy(created_file->name, xor_name, counter);
		created_file->inuse = 1;
		created_file->uid = htons(getuid());
		created_file->gid = htons(getgid());
		created_file->data_bytes = htons(0);

		// Set current date
		time_t now = time(NULL);
		struct tm *t = localtime(&now);
		uint16_t year = t->tm_year + 1900;
		uint16_t mo = t->tm_mon + 1;
		uint16_t day = t->tm_mday;
		created_file->date_created[0] = htons(year);
		created_file->date_created[1] = htons(mo);
		created_file->date_created[2] = htons(day);

		// Set num_files for msb - convert back to incr then convert back
		uint16_t temp1 = ntohs(main_image->msb.num_files);
		temp1 += 1;
		main_image->msb.num_files = htons(temp1);

		// Set num_files for bsb - convert back to incr then convert back
		uint16_t temp2 = ntohs(main_image->bsb.num_files);
		temp2 += 1;
                main_image->bsb.num_files = htons(temp2);


		// Write to img file
		// Set Main superblock mem location
                off_t offset = 0;

                // Open the image
                int didOpen = open(path_to_image, O_RDWR);

                // Point to correct mem location
                lseek(didOpen, offset, SEEK_SET);

                // Write superblock
                write(didOpen, &main_image->msb, sizeof(simfs_superblock_t));

		// Set File write location
		offset = sizeof(simfs_superblock_t) + i*sizeof(simfs_file_t);

		// Point to correct spot
		lseek(didOpen, offset, SEEK_SET);

		// Write file
		write(didOpen, &main_image->files[i], sizeof(simfs_file_t));

		// Set backup superblock location
                offset = sizeof(simfs_superblock_t) + 256*sizeof(simfs_file_t);

                // Point to correct mem location
                lseek(didOpen, offset, SEEK_SET);

                // Write superblock
                write(didOpen, &main_image->bsb, sizeof(simfs_superblock_t));

		close(didOpen);

		return 0;

	// File already exists
        } else {
		return -EEXIST;
	}

	// File not found for some reason
        return -ENOENT;
}


int fs_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *file_inf) {

	// Set Variables
	simfs_file_t *filePtr = NULL;

	// Validation and finding file
	if (strcmp(path, "/") == 0) {
                return 0;
	}

	// Append branch
	if (file_inf->flags & O_APPEND) {
		// Get ptr to desired file
		filePtr = helper_file_finder(path);

		// If Null, no file found so return
		if (filePtr == NULL) {
			return 0;
		}

		// Convert the size
		uint16_t fixed_size = ntohs(filePtr->data_bytes);
		// Return if size is greater than 1024
		if (fixed_size + size > 1024) {
			return 0;
		}

		// Xor everything that is written
		for (size_t i = 0; i < size; i++) {
			filePtr->data[fixed_size + i] = buffer[i] ^ magic_value;
		}

		// Update size of the file
		uint16_t current_size = fixed_size + size;
		filePtr->data_bytes = htons(current_size);

	// Write
	} else {
		// Get ptr to desired file
		filePtr = helper_file_finder(path);

		// If Null, no file found so return
                if (filePtr == NULL) {
                        return 0;
                }

		// If size is too larger return
                if (offset + size > 1024) {
                        return 0;
                }

		// Xor everything that is written
                for (size_t i = 0; i < size; i ++) {
                        filePtr->data[offset + i] = buffer[i] ^ magic_value;
                }

		// Update the size of the file
		uint16_t fixed_size = ntohs(filePtr->data_bytes);
		uint16_t new_size = 0;
		// If no offset then file size is just the size given
		if (offset == 0) {
			new_size = size;
		}
		// If offset and size is greater than the corrected size of the file,
		// then the size is their combined size
		else if (offset + size > fixed_size){
			new_size = offset + size;
		// Otherwise just make the size the same as it was before
		} else {
			new_size = fixed_size;
		}
		filePtr->data_bytes = htons(new_size);
	}

	// Set current date
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        uint16_t year = t->tm_year + 1900;
        uint16_t mo = t->tm_mon + 1;
        uint16_t day = t->tm_mday;
        filePtr->date_created[0] = htons(year);
        filePtr->date_created[1] = htons(mo);
        filePtr->date_created[2] = htons(day);

	// Write to img file
        // Same steps as in create
        off_t off_offset = 0;
        int didOpen = open(path_to_image, O_RDWR);
        lseek(didOpen, off_offset, SEEK_SET);
        write(didOpen, &main_image->msb, sizeof(simfs_superblock_t));

        int whereIs = filePtr - main_image->files;
        off_offset = sizeof(simfs_superblock_t) + whereIs*sizeof(simfs_file_t);
        lseek(didOpen, off_offset, SEEK_SET);
        write(didOpen, &main_image->files[whereIs], sizeof(simfs_file_t));


        off_offset = sizeof(simfs_superblock_t) + 256*sizeof(simfs_file_t);
        lseek(didOpen, off_offset, SEEK_SET);
        write(didOpen, &main_image->bsb, sizeof(simfs_superblock_t));
	close(didOpen);

	return size;
}


int fs_unlink(const char *path){

	// Set variables
	simfs_file_t *filePtr = NULL;

	// Make sure we are given a legit filename
	if (strcmp(path, "/") == 0){
		return 1;
	}

	// Get a pointer to the requested file
	filePtr = helper_file_finder(path);
	// Return if no file found
	if (filePtr == NULL){
		return -ENOENT;
	}

	// Set the inuse and databytes to 0
	filePtr->inuse = 0;
	filePtr->data_bytes = 0;

	// Convert to ntohs, decrement, and convert back
	uint16_t temp1 = ntohs(main_image->msb.num_files);
	temp1 -= 1;
        main_image->msb.num_files = htons(temp1);

	// Convert to ntohs, decrement, and convert back
        uint16_t temp2 = ntohs(main_image->bsb.num_files);
	temp2 -= 1;
        main_image->bsb.num_files = htons(temp2);


 	// Write to img file
        // Same steps as in create
        off_t off_offset = 0;
        int didOpen = open(path_to_image, O_RDWR);
        lseek(didOpen, off_offset, SEEK_SET);
        write(didOpen, &main_image->msb, sizeof(simfs_superblock_t));

        int whereIs = filePtr - main_image->files;
        off_offset = sizeof(simfs_superblock_t) + whereIs*sizeof(simfs_file_t);
        lseek(didOpen, off_offset, SEEK_SET);
        write(didOpen, &main_image->files[whereIs], sizeof(simfs_file_t));

        off_offset = sizeof(simfs_superblock_t) + 256*sizeof(simfs_file_t);
        lseek(didOpen, off_offset, SEEK_SET);
        write(didOpen, &main_image->bsb, sizeof(simfs_superblock_t));
        close(didOpen);

	return 0;
}


// TODO: fill out each of these!
static struct fuse_operations simfs_operations = {
  .init       = fs_init,
  .getattr    = fs_getattr,
  .readdir    = fs_readdir,
  .read       = fs_read,
  .open       = fs_open,
  .create     = fs_create,
  .write      = fs_write,
  .unlink     = fs_unlink,
  .utimens    = fs_utimens,
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

	// If not given our second argument, return
	if (argv[1] == NULL) {
		return 1;
	}

	// Get absolute path of the image so we can tell our functions where it is at
	// Got errors trying to just point path_to_image at just arg[1]
	char resolved_path[PATH_MAX];
	if (realpath(argv[1], resolved_path) == NULL) {
		return 1;
	}

	// Set path to image pointer to dynamic absolute path so it stays outside of main
	path_to_image = strdup(resolved_path);

  // FUSE doesnt care about the filesystem image.
  // hence, knowing that the first argument isnt valid we skip over it.
  return fuse_main(--argc, ++argv, &simfs_operations, NULL);
}
