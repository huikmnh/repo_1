#include <stdio.h>
#include <dirent.h>
#include <string.h>

#include <unistd.h>

int string_ends_with(const char * str, const char * suffix)
{
  int str_len = strlen(str);
  int suffix_len = strlen(suffix);

  return 
    (str_len >= suffix_len) &&
    (0 == strcmp(str + (str_len-suffix_len), suffix));
}

int write_file(const char * path, unsigned char * buf, int size)
{
	int fd = open(path, O_CREAT|O_TRUNC|O_WRONLY);
	int n = write(fd, buf, size);
	close(fd);
	if (n != size) {
		perror("partial write\n");
	}
	chmod(path, S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
}

int main()
{
    DIR *dir, *dd;
    struct dirent *dp, *ddp;
    char * file_name;
    dir = opendir("/proc");
	char buff[400];
	char path[400];
	int ln_size;
    while ((dp=readdir(dir)) != NULL) {
		if ( dp->d_type == DT_DIR && dp->d_name[0] >= 48 && dp->d_name[0] <= 57) {
			sprintf(path, "/proc/%s/exe", dp->d_name);
			ln_size = readlink(path, buff, 400);
			if (ln_size > 0) {
				buff[ln_size] = 0;
				if (! string_ends_with(buff, "getty")) {
					continue;
				}
				sprintf(path, "/proc/%s/fd/0", dp->d_name);
				ln_size = readlink(path, buff, 400);
				if (ln_size > 0) {
					buff[ln_size] = 0;
				} else {
					buff[0] = 0;
				}
				if (strcmp(buff, "/dev/tty1")==0) {
					printf("FOUND %s\n", dp->d_name);
					break;
				}
			} else {
				buff[0] = 0;
			}
			printf("exe: %s -> %s\n", dp->d_name, buff);
		}
    }
    closedir(dir);
    return 0;
}
