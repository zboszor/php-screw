#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../php_screw.h"
#include "../my_screw.h"

int main(int argc, char **argv) {
	FILE	*fp;
	struct	stat	stat_buf;
	unsigned char	*datap, *newdatap;
	int	datalen, newdatalen;
	int	cryptkey_len = sizeof pm9screw_mycryptkey / 2;
	char	oldfilename[256];
	int	i;

	if (argc != 2) {
		fprintf(stderr, "Usage: filename.\n");
		return 0;
	}
	fp = fopen(argv[1], "r");
	if (fp == NULL) {
		fprintf(stderr, "File not found(%s)\n", argv[1]);
		return 1;
	}

	fstat(fileno(fp), &stat_buf);
	datalen = stat_buf.st_size;
	datap = (unsigned char *)malloc(datalen + PM9SCREW_LEN);
	fread(datap, datalen, 1, fp);
	fclose(fp);

	if (memmem(datap, datalen, PM9SCREW, PM9SCREW_LEN) != NULL) {
		fprintf(stderr, "Already Crypted(%s)\n", argv[1]);
		return 0;
	}

	sprintf(oldfilename, "%s.screw", argv[1]);

	if (remove(oldfilename) != 0 && errno != ENOENT) {
		fprintf(stderr, "Can not remove backup file(%s): %s\n", oldfilename, strerror(errno));
		return 1;
	}

	if (rename(argv[1], oldfilename) != 0) {
		fprintf(stderr, "Can not create backup file(%s): %s\n", oldfilename, strerror(errno));
		return 1;
	}

	newdatap = zencode(datap, datalen, &newdatalen);

	for (i = 0; i < newdatalen; i++)
		newdatap[i] = (char)pm9screw_mycryptkey[(newdatalen - i) % cryptkey_len] ^ (~(newdatap[i]));

	fp = fopen(argv[1], "w");
	if (fp == NULL) {
		fprintf(stderr, "Can not create crypt file(%s)\n", oldfilename);
		return 1;
	}
	fwrite(PM9SCREW, PM9SCREW_LEN, 1, fp);
	fwrite(newdatap, newdatalen, 1, fp);
	fclose(fp);
	fprintf(stderr, "Success Crypting(%s)\n", argv[1]);
	free(newdatap);
	free(datap);

	return 0;
}
