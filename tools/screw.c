/*
 * screw encoder
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
	int	datalen, orig_datalen, newdatalen;
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

	if (datalen >= 2 && memcmp(datap, "#!", 2) == 0) {
		found_shebang = 1;
		datap += 2;
		datalen -= 2;

		while (datalen > 0 && *datap != '\r' && *datap != '\n') {
			datap++;
			datalen--;
		}

		if (datalen >= 2 && *datap == '\r' && *(datap + 1) == '\n') {
			datap += 2;
			datalen -= 2;
		} else if (datalen >= 1) {
			datap++;
			datalen--;
		} else {
			fprintf(stderr, "Only shebang line found(%s)\n", argv[1]);
			return 0;
		}
	}

	newdatap = zencode(datap, datalen, &newdatalen);

	for (i = 0; i < newdatalen; i++)
		newdatap[i] = (char)pm9screw_mycryptkey[(newdatalen - i) % cryptkey_len] ^ (~(newdatap[i]));

	fp = fopen(argv[1], "wb");
	if (fp == NULL) {
		fprintf(stderr, "Can not create crypt file(%s)\n", oldfilename);
		return 1;
	}
	if (found_shebang)
		fwrite(orig_datap, orig_datalen - datalen, 1, fp);
	fwrite(PM9SCREW, PM9SCREW_LEN, 1, fp);
	fwrite(newdatap, newdatalen, 1, fp);
	fclose(fp);
	fprintf(stderr, "Success Crypting(%s)\n", argv[1]);
	free(newdatap);
	free(orig_datap);

	return 0;
}
