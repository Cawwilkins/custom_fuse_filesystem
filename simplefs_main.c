#include "proj5structs.h"
#include <errno.h>
#include <limits.h>

typedef struct __attribute__((packed)) img {
  simfs_superblock_t msb;
  simfs_file_t files[256];
  simfs_superblock_t bsb;
} img_t;

char* path_to_image = NULL;
img_t* main_image = NULL;
char* img_buffer = NULL;
uint16_t magic_value = 0;

simfs_file_t *helper_file_finder(const char *path){
	printf("helper_file_finder called \n");
	char fileName[24];
        simfs_file_t *compareFile = NULL;
        char fixedName[24];

	// empty fileName
	memset(fileName, 0, 24);
	memset(fixedName, 0, 24);

	// To get rid of leading slash
        if (path[0] == '/') {
                strncpy(fileName, path + 1, 23);
        } else {
                strncpy(fileName, path, 23);
        }
        fileName[23] ='\0';

	printf("helper finder - given filename: %s\n", fileName);
        for (int i = 0; i < 256; i++) {
/////////////////////////////////////////////////////////////////////////////////////////////////
  		simfs_file_t* simfile;
    		simfile = &main_image->files[i];
    		if (simfile->inuse == 1) {
			printf("|-> File Overview\n");
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

///////////////////////////////////////////////////////////////////////////////////////////////
		 memset(fixedName, 1, 24);
        	 compareFile = &main_image->files[i];

                 // Check if file in use before xoring, name not defined yet
                 if (compareFile->inuse == 0){
     	         	continue;
                 }

                 // Loop through each index and xor to return to original name
                 for (int j = 0; j < 23; j++) {
                	if (compareFile->name[j] == '\0') {
				fixedName[j] = '\0';
				break;
			}
			fixedName[j] = compareFile->name[j] ^ magic_value;
		 }

                 // Names match and file in use
                 if (strcmp(fixedName, fileName) == 0){
			return compareFile;
		}
	}
	return NULL;
}

void *fs_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
        //ssize_t maxBytes = 271660; // From spec
	int sb = sizeof(simfs_superblock_t) * 2;
	int file = sizeof(simfs_file_t) * 256;
	int totals = sb + file;

	(void)conn;
	(void)cfg;

        // Open the image
        int didOpen = open(path_to_image, O_RDWR);
	if (didOpen < 0) {
		perror("open");
		return NULL;
	}

	uint8_t bytes[totals];

	// Read the file
        ssize_t r = read(didOpen, &bytes, totals);
	if (r != totals) {
		printf("ERROR: something wrong with read\n");
	}
	close(didOpen);

        // Cast as image and store file
	//main_image = (img_t*) &bytes;

	main_image = malloc(sizeof(img_t));
	uint8_t *p = bytes;

	memcpy(&main_image->msb, p, sizeof(simfs_superblock_t));
	p+= sizeof(simfs_superblock_t);
	memcpy(&main_image->files, p, sizeof(simfs_file_t) * 256);
        p+= sizeof(simfs_file_t) * 256;
	memcpy(&main_image->bsb, p, sizeof(simfs_superblock_t));
	p+= sizeof(simfs_superblock_t);



	magic_value = ntohs(main_image->msb.magic);
	printf("magic_value %d", magic_value);
	printf("init\n");
	printf("****** simplefs.img information ******\n");
  	printf("|-> Main Superblock\n");
 	printf("|-|-> .signature => %s\n", main_image->msb.signature);
  	printf("|-|-> .version => %d\n", ntohs(main_image->msb.version));
  	printf("|-|-> .num_files => %d\n", ntohs(main_image->msb.num_files));
  	printf("|-|-> .magic => %d\n\n", ntohs(main_image->msb.magic));

  	printf("|-> Backup Superblock\n");
  	printf("|-|-> .signature => %s\n", main_image->bsb.signature);
  	printf("|-|-> .version => %d\n", ntohs(main_image->bsb.version));
  	printf("|-|-> .num_files => %d\n", ntohs(main_image->bsb.num_files));
  	printf("|-|-> .magic => %d\n\n", ntohs(main_image->bsb.magic));

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
			stbuf->st_size = filePtr->data_bytes;
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

	printf("In readdir \n");
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
		 memset(&std_buff, 0, sizeof(struct stat));
	         std_buff.st_dev = 0;
        	 std_buff.st_ino = 0;
        	 std_buff.st_nlink = 0;
        	 std_buff.st_rdev = 0;
        	 std_buff.st_blksize = 0;
        	 std_buff.st_blocks = 0;
		 std_buff.st_nlink = 1;
		 std_buff.st_mtime = time(NULL);
		 std_buff.st_ctime = time(NULL);
		 std_buff.st_atime = time(NULL);
                 std_buff.st_uid = compareFile->uid;
                 std_buff.st_gid = compareFile->gid;
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
		 printf("Assumeing we reach here in function");
                 filler(buffer, xorName, &std_buff, 0, 0);
        }
        return 0;
}


int fs_read(const char *path, char *buff, size_t size, off_t offset, struct fuse_file_info *file_info){
	simfs_file_t *filePtr = NULL;
	size_t numCopy = 0;

	(void)file_info;

	printf("in fs_read \n");
	// Validation and finding file
	if (strcmp(path, "/") == 0) {
		return 0;
	}

	filePtr = helper_file_finder(path);
	if (filePtr == NULL) {
		return 0;
	}

	uint16_t curr_size = ntohs(filePtr->data_bytes);

	// Converting file size and calculating how much to move offset
	if (offset >= curr_size) {
		return 0;
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
	(void)path;
	(void)file_inf;
	return 0;
}

int fs_create(const char *path, mode_t mode, struct fuse_file_info *file_info){
	simfs_file_t *filePtr = NULL;
	int i = 0;
	char xor_name[24];
	char file_name[24];
	int counter = 0;

	printf("fs_create\n");
	// Mark what won't be used
	(void)mode;
	(void)file_info;

	memset(xor_name, 0, 24);

	// To get rid of leading slash and set file name
	if (path[0] == '/') {
		strncpy(file_name, path + 1, 23);
	} else {
		strncpy(file_name, path, 23);
	}
	file_name[23] ='\0';

	// Confirm that pointer is null (file doesnt exist)
	filePtr = helper_file_finder(path);
	printf("helper just ran\n");
	if (filePtr != NULL) {
		printf("ptr wasnt null\n");
	}
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
		printf("create - index is: %d\n", i);

		// Loop through each index and xor to encode original name
                for (int j = 0; j < 23; j++) {
			counter += 1;
			if (file_name[j] == '\0') {
				xor_name[j] = '\0';
				break;
			}
			xor_name[j] = file_name[j] ^ magic_value;
			printf("create - xorname[j] = %c\n", xor_name[j]);
                }
		printf("counter is: %d", counter);
		printf("is xor still here? xor[0] = %c", xor_name[0]);

		// Create a pointer to location of first place without file
		simfs_file_t *created_file = &main_image->files[i];
		memset(created_file->name, 0, 24); ////////////////////////////

		// Set attributes of file
		memcpy(created_file->name, xor_name, counter);
		printf("create - xor name: %s\n", created_file->name);
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
		printf("unit 16 year = %hu , mo = %hu , day = %hu",year, mo, day); 
		created_file->date_created[0] = htons(year);
		created_file->date_created[1] = htons(mo);
		created_file->date_created[2] = htons(day);
		printf("create - completed\n");

		uint16_t temp1 = ntohs(main_image->msb.num_files);
		temp1 += 1;
		main_image->msb.num_files = htons(temp1);

		uint16_t temp2 = ntohs(main_image->bsb.num_files);
		temp2 += 1;
                main_image->bsb.num_files = htons(temp2);
		printf("|-|-> .num_files => %d with ntohs\n", ntohs(main_image->msb.num_files));
		printf("|-|-> .num_files => %d with ntohs\n", ntohs(main_image->bsb.num_files));
		printf("|-|-> .num_files => %d %d without \n", temp1, temp2);
		return 0;

        } else {
		printf("create - already exists\n");
		return -EEXIST;
	}
	printf("create - some error\n");
        return -ENOENT;
}


int fs_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *file_inf) {
	simfs_file_t *filePtr = NULL;

	// Validation and finding file
	if (strcmp(path, "/") == 0) {
                return 0;
	}

	// Append
	if (file_inf->flags & O_APPEND) {
		filePtr = helper_file_finder(path);
		if (filePtr == NULL) {
			return 0;
		}
		uint16_t fixed_size = ntohs(filePtr->data_bytes);

		if (fixed_size + size > 1024) {
			return 0;
		}
		for (size_t i = 0; i < size; i++) {
			filePtr->data[fixed_size + i] = buffer[i] ^ magic_value;
		}

		uint16_t current_size = fixed_size + size;
		filePtr->data_bytes = htons(current_size);

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
                        filePtr->data[offset + i] = buffer[i] ^ magic_value;
                }
		uint16_t fixed_size = ntohs(filePtr->data_bytes);
		uint16_t new_size = 0;
		if (offset == 0) {
			new_size = size;
		}
		else if (offset + size > fixed_size){
			new_size = offset + size;
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
        printf("unit 16 year = %hu , mo = %hu , day = %hu",year, mo, day);
        filePtr->date_created[0] = htons(year);
        filePtr->date_created[1] = htons(mo);
        filePtr->date_created[2] = htons(day);
        printf("create - completed\n");

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

	uint16_t temp1 = ntohs(main_image->msb.num_files);
	temp1 -= 1;
        main_image->msb.num_files = htons(temp1);

        uint16_t temp2 = ntohs(main_image->bsb.num_files);
	temp2 -= 1;
        main_image->bsb.num_files = htons(temp2);

	printf("|-|-> msb.num_files => %d\n", ntohs(main_image->msb.num_files));
	printf("|-|-> bsb.num_files => %d\n", ntohs(main_image->bsb.num_files));
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

	char resolved_path[PATH_MAX];
	if (realpath(argv[1], resolved_path) == NULL) {
		perror("realpath");
		return 1;
	}
	//path_to_image = argv[1];
	path_to_image = strdup(resolved_path);

  // FUSE doesnt care about the filesystem image.
  // hence, knowing that the first argument isnt valid we skip over it.
  return fuse_main(--argc, ++argv, &simfs_operations, NULL);
}
