#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#ifndef SCREW_TOOL
#include <php.h>
#endif

#define OUTBUFSIZ  100000

unsigned char *zcodecom(int mode, unsigned char *inbuf, size_t inbuf_len, size_t *resultbuf_len) {
	z_stream z;
	int count, status;
	unsigned char *outbuf, *resultbuf;
	int total_count = 0;

	memset(&z, 0, sizeof(z));

	z.zalloc = Z_NULL;
	z.zfree = Z_NULL;
	z.opaque = Z_NULL;

	z.next_in = Z_NULL;
	z.avail_in = 0;
	if (mode == 0)
		deflateInit(&z, 1);
	else
		inflateInit(&z);

#ifdef SCREW_TOOL
	outbuf = malloc(OUTBUFSIZ);
	resultbuf = malloc(OUTBUFSIZ);
#else
	outbuf = emalloc(OUTBUFSIZ);
	resultbuf = emalloc(OUTBUFSIZ);
#endif

	z.next_out = outbuf;
	z.avail_out = OUTBUFSIZ;
	z.next_in = inbuf;
	z.avail_in = inbuf_len;

	while (1) {
		if (mode == 0)
			status = deflate(&z, Z_FINISH);
		else
			status = inflate(&z, Z_NO_FLUSH);
		if (status == Z_STREAM_END) break;
		if (status != Z_OK) {
			if (mode == 0)
				deflateEnd(&z);
			else
				inflateEnd(&z);
			*resultbuf_len = 0;
#ifdef SCREW_TOOL
			free(outbuf);
#else
			efree(outbuf);
#endif
			return resultbuf;
		}
		if (z.avail_out == 0) {
#ifdef SCREW_TOOL
			resultbuf = realloc(resultbuf, total_count + OUTBUFSIZ);
#else
			resultbuf = erealloc(resultbuf, total_count + OUTBUFSIZ);
#endif
			memcpy(resultbuf + total_count, outbuf, OUTBUFSIZ);
			total_count += OUTBUFSIZ;
			z.next_out = outbuf;
			z.avail_out = OUTBUFSIZ;
		}
	}
	if ((count = OUTBUFSIZ - z.avail_out) != 0) {
#ifdef SCREW_TOOL
		resultbuf = realloc(resultbuf, total_count + OUTBUFSIZ);
#else
		resultbuf = erealloc(resultbuf, total_count + OUTBUFSIZ);
#endif
		memcpy(resultbuf + total_count, outbuf, count);
		total_count += count;
	}
	if (mode == 0)
		deflateEnd(&z);
	else
		inflateEnd(&z);
	*resultbuf_len = total_count;
#ifdef SCREW_TOOL
	free(outbuf);
#else
	efree(outbuf);
#endif
	return resultbuf;
}

unsigned char *zencode(unsigned char *inbuf, size_t inbuf_len, size_t *resultbuf_len) {
	return zcodecom(0, inbuf, inbuf_len, resultbuf_len);
}

unsigned char *zdecode(unsigned char *inbuf, size_t inbuf_len, size_t *resultbuf_len) {
	return zcodecom(1, inbuf, inbuf_len, resultbuf_len);
}
