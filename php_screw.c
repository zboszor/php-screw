/*
 * php_screw
 * (C) 2007, Kunimasa Noda/PM9.com, Inc. <http://www.pm9.com,  kuni@pm9.com>
 * (C) 2020-2025, Zoltán Böszörményi <zboszor@gmail.com>
 * see file LICENSE for license details
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/file.h"
#include "ext/standard/info.h"
#include "Zend/zend_extensions.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "php_screw.h"
#include "my_screw.h"

#if PHP_MAJOR_VERSION >= 8
#define TSRMLS_C
#define TSRMLS_CC
#define TSRMLS_DC
#endif

#if PHP_VERSION_ID >= 70300
static int (*php_screw_orig_post_startup_cb)(void);
static int php_screw_post_startup(void);
#endif

PHP_MINIT_FUNCTION(php_screw);
PHP_MSHUTDOWN_FUNCTION(php_screw);
PHP_RINIT_FUNCTION(php_screw);
PHP_MINFO_FUNCTION(php_screw);

FILE *php_screw_ext_fopen(FILE *fp)
{
	struct	stat	stat_buf;
	unsigned char *datap, *newdatap;
	size_t	datalen, newdatalen;
	int	cryptkey_len = sizeof pm9screw_mycryptkey / 2;
	int	i;

	fstat(fileno(fp), &stat_buf);
	datalen = stat_buf.st_size - ftell(fp);
	datap = emalloc(datalen);
	memset(datap, 0, datalen);
	fread(datap, datalen, 1, fp);
	fclose(fp);

	for (i = 0; i < datalen; i++)
		datap[i] = (char)pm9screw_mycryptkey[(datalen - i) % cryptkey_len] ^ (~(datap[i]));

	newdatap = zdecode(datap, datalen, &newdatalen);

	fp = tmpfile();
	fwrite(newdatap, newdatalen, 1, fp);

	efree(datap);
	efree(newdatap);

	rewind(fp);
	return fp;
}

ZEND_API zend_op_array *(*org_compile_file)(zend_file_handle *file_handle, int type TSRMLS_DC);

ZEND_API zend_op_array *php_screw_compile_file(zend_file_handle *file_handle, int type TSRMLS_DC)
{
	FILE	*fp;
	char	buf[PM9SCREW_LEN + 1];
#if PHP_VERSION_ID >= 70000
	char	*opened_path;
#endif

	if (!file_handle || !file_handle->filename)
		return org_compile_file(file_handle, type TSRMLS_CC);

	if (zend_is_executing(TSRMLS_C)) {
		const char *fname = get_active_function_name(TSRMLS_C);

		if (fname && fname[0]) {
			if ( strcasecmp(fname, "show_source") == 0
			  || strcasecmp(fname, "highlight_file") == 0) {
				return NULL;
			}
		}
	}

#if PHP_VERSION_ID >= 80100
	fp = fopen(ZSTR_VAL(file_handle->filename), "rb");
#else
	fp = fopen(file_handle->filename, "rb");
#endif
	if (!fp) {
		return org_compile_file(file_handle, type TSRMLS_CC);
	}

	fread(buf, PM9SCREW_LEN, 1, fp);

	if (memcmp(buf, "#!", 2) == 0) {
		/* Shebang line found, ignore it. */
		char *lineptr = NULL;
		size_t bufsize = 0;
		ssize_t line_len;

		rewind(fp);

		line_len = getline(&lineptr, &bufsize, fp);
		if (line_len > 0 && bufsize > 0 && lineptr)
			free(lineptr);

		fread(buf, PM9SCREW_LEN, 1, fp);
	}
	if (memcmp(buf, PM9SCREW, PM9SCREW_LEN) != 0) {
		fclose(fp);
		return org_compile_file(file_handle, type TSRMLS_CC);
	}

	if (file_handle->type == ZEND_HANDLE_FP) {
		fclose(file_handle->handle.fp);
		file_handle->handle.fp = NULL;
	}
#ifdef ZEND_HANDLE_STREAM
	if (file_handle->type == ZEND_HANDLE_STREAM) {
		if (file_handle->handle.stream.closer && file_handle->handle.stream.handle)
			file_handle->handle.stream.closer(file_handle->handle.stream.handle);
		file_handle->handle.stream.handle = NULL;
	}
#endif
#ifdef ZEND_HANDLE_FD
	if (file_handle->type == ZEND_HANDLE_FD) {
		close(file_handle->handle.fd);
		file_handle->handle.fd = -1;
	}
#endif

	file_handle->handle.fp = php_screw_ext_fopen(fp);
	file_handle->type = ZEND_HANDLE_FP;

#if PHP_VERSION_ID >= 80100
	opened_path = expand_filepath(ZSTR_VAL(file_handle->filename), NULL TSRMLS_CC);
	file_handle->opened_path = zend_string_init(opened_path, strlen(opened_path), 0);
#elif PHP_VERSION_ID >= 70000
	opened_path = expand_filepath(file_handle->filename, NULL TSRMLS_CC);
	file_handle->opened_path = zend_string_init(opened_path, strlen(opened_path), 0);
#else
	file_handle->opened_path = expand_filepath(file_handle->filename, NULL TSRMLS_CC);
#endif

	return org_compile_file(file_handle, type TSRMLS_CC);
}

PHP_RINIT_FUNCTION(php_screw)
{
#if defined(ZTS) && defined(COMPILE_DL_PHP_SCREW) && defined(ZEND_TSRMLS_CACHE_UPDATE)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif

#if PHP_VERSION_ID >= 70000
	CG(compiler_options) = CG(compiler_options) | ZEND_COMPILE_EXTENDED_STMT;
#endif

	return SUCCESS;
}

zend_module_entry php_screw_module_entry = {
	STANDARD_MODULE_HEADER,
	PHPSCREW_NAME,
	NULL,
	PHP_MINIT(php_screw),
	PHP_MSHUTDOWN(php_screw),
	PHP_RINIT(php_screw),
	NULL,
	PHP_MINFO(php_screw),
	PHPSCREW_VERSION,
	STANDARD_MODULE_PROPERTIES
};

#if COMPILE_DL_PHP_SCREW
ZEND_GET_MODULE(php_screw);
#if defined(ZTS) && defined(ZEND_TSRMLS_CACHE_DEFINE)
ZEND_TSRMLS_CACHE_DEFINE();
#endif
#endif

PHP_MINFO_FUNCTION(php_screw)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "php_screw support", "enabled");
	php_info_print_table_end();
}

void php_screw_base_post_startup(void)
{
	org_compile_file = zend_compile_file;
	zend_compile_file = php_screw_compile_file;
}

PHP_MINIT_FUNCTION(php_screw)
{
#if PHP_VERSION_ID < 70000
	CG(compiler_options) = CG(compiler_options) | ZEND_COMPILE_EXTENDED_STMT;
#endif

	/*
	 * Redirect compile function to our own. For PHP 7.3 and
	 * later, we hook it in php_screw_post_startup instead.
	 */
#if PHP_VERSION_ID < 70300
	php_screw_base_post_startup();
#endif
	return SUCCESS;
}

void php_screw_base_shutdown(void)
{
	zend_compile_file = org_compile_file;
}

PHP_MSHUTDOWN_FUNCTION(php_screw)
{
#if PHP_VERSION_ID < 70300
	php_screw_base_shutdown();
#endif
	return SUCCESS;
}

ZEND_DLEXPORT int php_screw_zend_startup(zend_extension *extension)
{
#if PHP_VERSION_ID >= 70300
	php_screw_orig_post_startup_cb = zend_post_startup_cb;
	zend_post_startup_cb = php_screw_post_startup;
#endif
	return zend_startup_module(&php_screw_module_entry);
}

ZEND_DLEXPORT void php_screw_zend_shutdown(zend_extension *extension)
{
#if PHP_VERSION_ID >= 70300
	php_screw_base_shutdown();
#endif
}

#if PHP_VERSION_ID >= 70300
static int php_screw_post_startup(void)
{
	if (php_screw_orig_post_startup_cb) {
		int (*cb)(void) = php_screw_orig_post_startup_cb;

		php_screw_orig_post_startup_cb = NULL;
		if (cb() != SUCCESS) {
			return FAILURE;
		}
	}

	php_screw_base_post_startup();
	return SUCCESS;
}
#endif

#ifndef ZEND_EXT_API
#define ZEND_EXT_API	ZEND_DLEXPORT
#endif

ZEND_EXT_API zend_extension_version_info extension_version_info = { ZEND_EXTENSION_API_NO, (char*) ZEND_EXTENSION_BUILD_ID };

ZEND_DLEXPORT zend_extension zend_extension_entry = {
	(char*) PHPSCREW_NAME,
	(char*) PHPSCREW_VERSION,
	(char*) PHPSCREW_AUTHOR,
	(char*) PHPSCREW_URL_FAQ,
	(char*) PHPSCREW_COPYRIGHT_SHORT,
	php_screw_zend_startup,
	php_screw_zend_shutdown,
	NULL,			/* activate_func_t */
	NULL,			/* deactivate_func_t */
	NULL,			/* message_handler_func_t */
	NULL,			/* op_array_handler_func_t */
	NULL,			/* statement_handler_func_t */
	NULL,			/* fcall_begin_handler_func_t */
	NULL,			/* fcall_end_handler_func_t */
	NULL,			/* op_array_ctor_func_t */
	NULL,			/* op_array_dtor_func_t */
	STANDARD_ZEND_EXTENSION_PROPERTIES
};
