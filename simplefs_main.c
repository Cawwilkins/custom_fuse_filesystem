#include "proj5structs.h"
#include <errno.h>

typedef struct img {
  simfs_superblock_t msb;
  simfs_file_t files[256];
  simfs_superblock_t bsb;
} img_t;

char* path_to_image = NULL;
img_t* main_image = NULL;
char* img_buffer = NULL;
uint16_t magic_value;

simfs_file_t *helper_file_finder(const char *path){
	printf("helper_file_finder called \n");
	char fileName[24];
        simfs_file_t *compareFile = NULL;
        char xorName[24];
	int j = 0;

	memset(fileName, 0, 24);

	// To get rid of leading slash
        if (path[0] == '/') {
                strncpy(fileName, path + 1, 23);
        } else {
                strncpy(fileName, path, 23);
        }
        fileName[23] ='\0';

	printf("given filename: %s\n", fileName);
        for (int i = 0; i < 256; i++) {
        	 compareFile = &main_image->files[i];

                 // Check if file in use before xoring, name not defined yet
                 if (compareFile->inuse == 0){
     	         	continue;
                 }

		 printf("comparfile name is: %s \n", compareFile->name);
                 // Loop through each index and xor to return to original name
                 for (int j = 0; j < 23 && compareFile->name[j] != '\0'; j++) {
                 	xorName[j] = compareFile->name[j] ^ main_image->msb.magic;
                 }
                 xorName[j] = '\0';

                 // Names match and file not in use
		 printf("xor name %s, fileName %s\n", xorName, fileName);
                 if (strcmp(xorName, fileName) == 0){
			printf("helper finder - names match and not in use\n");
			return compareFile;
		}
	}
	printf("helper finder - nothing matched, return null\n");
	return NULL;
}

void *fs_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
        //ssize_t maxBytes = 271660; // From spec
	int sb = sizeof(simfs_superblock_t) * 2;
	int file = sizeof(simfs_file_t) * 256;
	int totals = sb + file;

	(void)conn;
	(void)cfg;

	printf("Debug - path to image is: %s\n", path_to_image);
        // Open the image
        int didOpen = open(path_to_image, O_RDWR);
	if (didOpen < 0) {
		perror("open");
		return NULL;
	}

	uint8_t bytes[totals];
        //img_buffer = malloc(maxBytes);

        // Read the file
        //read(didOpen, img_buffer, maxBytes);
        ssize_t r = read(didOpen, &bytes, totals);
	if (r != totals) {
		printf("ERROR: something wrong with read\n");
	}
	close(didOpen);

        // Cast as image and store file
        //main_image = (img_t*) img_buffer;
	main_image = (img_t*) &bytes;

	magic_value = ntohs(main_image->msb.magic);
	printf("magic_value %d", magic_value);
	printf("init\n");

	return NULL;
}

int fs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
	simfs_file_t *filePtr = NULL;

	(void)fi;
	printf("getattr\n");
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

	time_t now = time(NULL);
	stbuf->st_atime = now;
	stbuf->st_mtime = now;
	stbuf->st_ctime = now;

	// Directory
	if (strcmp(path,"/") == 0) {
		printf("getattr in directory path\n");
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
		return 0;
	// File
	} else {
		filePtr = helper_file_finder(path);

		// If duplicate exists and it is in use
		if (filePtr != NULL && filePtr->inuse == 1) {
			printf("In getattr, file branch\n");
			stbuf->st_mode = S_IFREG | 0644;
			stbuf->st_nlink = 1;
			stbuf->st_size = ntohs(filePtr->data_bytes);
			stbuf->st_uid = filePtr->uid;
			stbuf->st_gid = filePtr->gid;
			return 0;
		}
	}
	printf("Getattr returning enoent\n");
	return -ENOENT;
}

int fs_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *file_inf, enum fuse_readdir_flags flags) {
        char xorName[25];

	(void)file_inf;
	(void)flags;
	(void)offset;

	printf("readdir \n");
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
                 struct stat std_buff;
	         std_buff.st_dev = 0;
        	 std_buff.st_ino = 0;
        	 std_buff.st_nlink = 0;
        	 std_buff.st_rdev = 0;
        	 std_buff.st_blksize = 0;
        	 std_buff.st_blocks = 0;
                 std_buff.st_uid = compareFile->uid;
                 std_buff.st_gid = compareFile->gid;
                 std_buff.st_size = ntohs(compareFile->data_bytes);
                 std_buff.st_mode = S_IFREG | 0644;


                 // Loop through each index and xor to return to original name
                 for (int j = 0; j < 24; j++) {
                        xorName[j] = compareFile->name[j] ^ main_image->msb.magic;
                 }
                 xorName[24] = '\0';

                 filler(buffer, xorName, &std_buff, 0, 0);
        }
        return 0;
}


int fs_read(const char *path, char *buff, size_t size, off_t offset, struct fuse_file_info *file_info){
	simfs_file_t *filePtr = NULL;
	int fileSize = 0;
	int numCopy = 0;
	int currentIndex = 0;
	size_t size_off = (size_t)offset;
	(void)file_info;
	printf("in fs_read \n");
	// Validation and finding file
	if (strcmp(path, "/") == 0) {
		return 0;
	} else {
		filePtr = helper_file_finder(path);
		if (filePtr == NULL) {
			return 0;
		}
	}

	// Converting file size and calculating how much to move offset
	fileSize = ntohs(filePtr->data_bytes);
	if (fileSize <= offset) {
		return 0;
	} else {
		if (size < (fileSize - size_off)) {
			numCopy = size;
		} else {
			numCopy = fileSize - offset;
		}
		// Copy each index and translate it
		for (int i = 0; i < numCopy; i++) {
			currentIndex = offset + i;
			buff[i] = filePtr->data[currentIndex] ^ main_image->msb.magic;
		}
	}
	return numCopy;
}

int fs_open(const char *path, struct fuse_file_info *file_inf){
	(void)path;
	(void)file_inf;
	return 0;
}

int fs_create(const char *path, mode_t mode, struct fuse_file_info *file_info){
	simfs_file_t *filePtr = NULL;
	int i = 0;
	char xor_name[24];
	char file_name[24];

	printf("fs_create\n");
	// Mark what won't be used
	(void)mode;
	(void)file_info;

	// To get rid of leading slash and set file name
	if (path[0] == '/') {
		strncpy(file_name, path + 1, 24);
	} else {
		strncpy(file_name, path, 24);
	}
	file_name[23] ='\0';

	// Confirm that pointer is null (file doesnt exist)
	filePtr = helper_file_finder(path);

	// No file yet so make one
	if (filePtr == NULL) {
		printf("create - no file exists so making one\n");
		// Find first index where we can store files
		while (i < 256 && main_image->files[i].inuse == 1) {
			i++;
		}
		// If we get through all of our files, return
		if (i == 256) {
			return -ENOENT;
		}

		// Loop through each index and xor to encode original name
                for (int j = 0; j < 24; j++) {
                       xor_name[j] = file_name[j] ^ main_image->msb.magic;
                }
                xor_name[24] = '\0';
		printf("create file - magic value is %d\n", magic_value);
		printf("create file - xor name %s filename %s \n", xor_name, file_name);

		// Create a pointer to location of first place without file
		simfs_file_t *created_file = &main_image->files[i];

		// Set attributes of file
		memcpy(created_file->name, xor_name, 24);
		printf("xor name: ");
		for (int c = 0; c < 24; c++){
			printf("%c", xor_name[c]);
		}
		created_file->inuse = 1;
		created_file->uid = getuid();
		created_file->gid = getgid();
		created_file->data_bytes = 0;

		// Set current date
		time_t now = time(NULL);
		struct tm *t = localtime(&now);
		created_file->date_created[0] = t->tm_year + 1900;
		created_file->date_created[1] = t->tm_mon + 1;
		created_file->date_created[2] = t->tm_mday;
		printf("create - completed\n");
		return 0;

        } else {
		printf("create - already exists\n");
		return -EEXIST;
	}
	printf("create - some error");
        return -ENOENT;
}


int fs_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *file_inf) {
	simfs_file_t *filePtr = NULL;

	// Validation and finding file
	if (strcmp(path, "/") == 0) {
                return 0;
	}

	// Append
	if (file_inf->flags && O_APPEND) {
		filePtr = helper_file_finder(path);
		if (filePtr == NULL) {
			return 0;
		}
		if (filePtr->data_bytes + size > 1024) {
			return 0;
		}
		for (size_t i = 0; i < size; i ++) {
			filePtr->data[filePtr->data_bytes + i] = buffer[i] ^ main_image->msb.magic;
		}
		filePtr->data_bytes += size;

	// Write
	} else {
		filePtr = helper_file_finder(path);
                if (filePtr == NULL) {
                        return 0;
                }
                if (offset + size > 1024) {
                        return 0;
                }
                for (size_t i = 0; i < size; i ++) {
                        filePtr->data[offset + i] = buffer[i] ^ main_image->msb.magic;
                }
		filePtr->data_bytes = offset + size;
	}

	time_t now = time(NULL);
        struct tm *t = localtime(&now);
        filePtr->date_created[0] = t->tm_year + 1900;
        filePtr->date_created[1] = t->tm_mon + 1;
        filePtr->date_created[2] = t->tm_mday;

	return size;
}


int fs_unlink(const char *path){
	simfs_file_t *filePtr = NULL;

	if (strcmp(path, "/") == 0){
		return 1;
	}

	filePtr = helper_file_finder(path);
	if (filePtr == NULL){
		return -ENOENT;
	}

	filePtr->inuse = 0;
	filePtr->data_bytes = 0;
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
	if (argv[1] == NULL) {
		return 1;
	}
	path_to_image = argv[1];

  // FUSE doesnt care about the filesystem image.
  // hence, knowing that the first argument isnt valid we skip over it.
  return fuse_main(--argc, ++argv, &simfs_operations, NULL);
}
