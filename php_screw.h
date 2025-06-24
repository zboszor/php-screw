#define PHPSCREW_NAME		"php_screw"
#define PHPSCREW_VERSION	"1.6.0"
#define PHPSCREW_AUTHOR		"Kunimasa Noda"
#define PHPSCREW_URL_FAQ	"http://www.pm9.com"
#define PHPSCREW_COPYRIGHT_SHORT	"(C) 2007 Kunimasa Noda/PM9.com, Inc."

#if PHP_VERSION_ID < 70400
#define ZEND_COMPILE_EXTENDED_STMT ZEND_COMPILE_EXTENDED_INFO
#endif

#define PM9SCREW        "\tPM9SCREW\t"
#define PM9SCREW_LEN     10

unsigned char *zdecode(unsigned char *inbuf, int inbuf_len, int *resultbuf_len);
unsigned char *zencode(unsigned char *inbuf, int inbuf_len, int *resultbuf_len);
