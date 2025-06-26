/*
 * screw decoder
 * (C) 2007, Kunimasa Noda/PM9.com, Inc. <http://www.pm9.com,  kuni@pm9.com>
 * (C) 2020-2025, Zoltán Böszörményi <zboszor@gmail.com>
 * see file LICENSE for license details
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "php_screw.h"
#include "my_screw.h"

int main(int argc, char **argv) {
	FILE	*fp;
	struct	stat	stat_buf;
	unsigned char	*datap, *orig_datap, *newdatap;
	size_t datalen, orig_datalen, newdatalen;
	int	cryptkey_len = sizeof pm9screw_mycryptkey / 2;
	char	oldfilename[256];
	int	i, found_shebang = 0;

	if (argc != 2) {
		fprintf(stderr, "Usage: filename.\n");
		return 0;
	}
	fp = fopen(argv[1], "rb");
	if (fp == NULL) {
		fprintf(stderr, "File not found(%s)\n", argv[1]);
		return 1;
	}

	fstat(fileno(fp), &stat_buf);
	orig_datalen = datalen = stat_buf.st_size;
	orig_datap = datap = (unsigned char *)malloc(datalen + PM9SCREW_LEN);
	fread(datap, datalen, 1, fp);
	fclose(fp);

	unsigned char *prefix_start = memmem(datap, datalen, PM9SCREW, PM9SCREW_LEN);
	if (prefix_start == NULL) {
		fprintf(stderr, "Already Decrypted(%s)\n", argv[1]);
		return 0;
	}

	datap = prefix_start + PM9SCREW_LEN;
	datalen = orig_datalen - (datap - orig_datap);

	sprintf(oldfilename, "%s.unscrew", argv[1]);

	if (remove(oldfilename) != 0 && errno != ENOENT) {
		fprintf(stderr, "Can not remove backup file(%s): %s\n", oldfilename, strerror(errno));
		return 1;
	}

	if (rename(argv[1], oldfilename) != 0) {
		fprintf(stderr, "Can not create backup file(%s): %s\n", oldfilename, strerror(errno));
		return 1;
	}

	for (i = 0; i < datalen; i++)
		datap[i] = (char)pm9screw_mycryptkey[(datalen - i) % cryptkey_len] ^ (~(datap[i]));

	newdatap = zdecode(datap, datalen, &newdatalen);

	fp = fopen(argv[1], "wb");
	if (fp == NULL) {
		fprintf(stderr, "Can not create crypt file(%s)\n", oldfilename);
		return 1;
	}

	found_shebang = prefix_start - orig_datap;
	if (found_shebang)
		fwrite(orig_datap, found_shebang, 1, fp);
	fwrite(newdatap, newdatalen, 1, fp);
	fclose(fp);
	fprintf(stderr, "Success Decrypting(%s)\n", argv[1]);
	free(newdatap);
	free(orig_datap);

	return 0;
}
