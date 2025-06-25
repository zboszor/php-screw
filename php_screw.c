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

struct php_screw_data {
	char *buf;
	size_t len;
};

ZEND_API zend_op_array *(*org_compile_file)(zend_file_handle *file_handle, int type TSRMLS_DC);

ZEND_API zend_op_array *php_screw_compile_file(zend_file_handle *file_handle, int type TSRMLS_DC)
{
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

	struct php_screw_data screw_data = { NULL, 0L };

	if (zend_stream_fixup(file_handle, &screw_data.buf, &screw_data.len TSRMLS_CC) == FAILURE)
		return NULL;

	char *prefix = memmem(screw_data.buf, screw_data.len, PM9SCREW, PM9SCREW_LEN);

	if (prefix != NULL) {
		struct php_screw_data new_screw_data = { NULL, 0L };
		unsigned char *encrypted_start = (unsigned char *)prefix + PM9SCREW_LEN;
		size_t encrypted_len = screw_data.len - ((char *)encrypted_start - screw_data.buf);
		int cryptkey_len = sizeof pm9screw_mycryptkey / 2;
		int i;

		for (i = 0; i < encrypted_len; i++)
			encrypted_start[i] = (char)pm9screw_mycryptkey[(encrypted_len - i) % cryptkey_len] ^ (~(encrypted_start[i]));

		new_screw_data.buf = (char *)zdecode(encrypted_start, encrypted_len, &new_screw_data.len);

#if PHP_VERSION_ID >= 70400
		efree(file_handle->buf);
		file_handle->buf = new_screw_data.buf;
		file_handle->len = new_screw_data.len;
#else
		efree(file_handle->handle.stream.mmap.buf);
		file_handle->handle.stream.mmap.buf = new_screw_data.buf;
		file_handle->handle.stream.mmap.len = new_screw_data.len;
#endif

		switch (file_handle->type) {
		case ZEND_HANDLE_FP:
			if (file_handle->handle.fp) {
				fclose(file_handle->handle.fp);
				file_handle->handle.fp = NULL;
			}
			break;
		case ZEND_HANDLE_STREAM:
			if (file_handle->handle.stream.closer && file_handle->handle.stream.handle) {
				file_handle->handle.stream.closer(file_handle->handle.stream.handle);
			}
			file_handle->handle.stream.handle = NULL;
			break;
		case ZEND_HANDLE_FILENAME:
		default:
			break;
		}

		file_handle->handle.fp = fmemopen(new_screw_data.buf, new_screw_data.len, "r");
		file_handle->type = ZEND_HANDLE_FP;

		if (zend_stream_fixup(file_handle, &screw_data.buf, &screw_data.len TSRMLS_CC) == FAILURE)
			return NULL;
	}

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
