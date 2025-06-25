/*
 * php_screw
 * (C) 2007, Kunimasa Noda/PM9.com, Inc. <http://www.pm9.com,  kuni@pm9.com>
 * (C) 2020-2025, Zoltán Böszörményi <zboszor@gmail.com>
 * see file LICENSE for license details
 */
#ifndef _PHP_SCREW_H_
#define _PHP_SCREW_H_

#define PHPSCREW_NAME    "php_screw"
#define PHPSCREW_VERSION "2.0.0"
#define PHPSCREW_AUTHOR  "Kunimasa Noda and Zoltán Böszörményi"
#define PHPSCREW_URL_FAQ "https://github.com/zboszor/php-screw"
#define PHPSCREW_COPYRIGHT_SHORT "(C) 2020-2025 Zoltán Böszörményi <zboszor@gmail.com>"

#if PHP_VERSION_ID < 70400
#define ZEND_COMPILE_EXTENDED_STMT ZEND_COMPILE_EXTENDED_INFO
#endif

#define PM9SCREW        "\tPM9SCREW\t"
#define PM9SCREW_LEN     10

unsigned char *zdecode(unsigned char *inbuf, int inbuf_len, int *resultbuf_len);
unsigned char *zencode(unsigned char *inbuf, int inbuf_len, int *resultbuf_len);

#endif
