/*
 * A/UX 3's stdio: its FILE, which the shared library works on directly, and the
 * functions it and libaux have. getc and putc are functions here, not A/UX's macros.
 */
#ifndef _STDIO_H
#define _STDIO_H

#include <stdarg.h>
#include <stddef.h>

typedef struct {
	int _cnt;
	unsigned char *_ptr;
	unsigned char *_base;
	char _flag;
	char _file;
} FILE;
typedef long fpos_t;

#define BUFSIZ		1024
#define _NFILE		128
#define _SBFSIZ		8
#define EOF		(-1)
#define FOPEN_MAX	_NFILE
#define FILENAME_MAX	1024
#define L_tmpnam	25
#define TMP_MAX		17576

#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2

#define _IOFBF		0000
#define _IOREAD		0001
#define _IOWRT		0002
#define _IONBF		0004
#define _IOMYBUF	0010
#define _IOEOF		0020
#define _IOERR		0040
#define _IOLBF		0100
#define _IORW		0200

extern FILE _iob[_NFILE];
extern unsigned char *_bufendtab[];

#define stdin		(&_iob[0])
#define stdout		(&_iob[1])
#define stderr		(&_iob[2])

#define feof(p)		(((p)->_flag & _IOEOF) != 0)
#define ferror(p)	(((p)->_flag & _IOERR) != 0)
#define clearerr(p)	((void)((p)->_flag &= ~(_IOERR | _IOEOF)))
#define fileno(p)	((p)->_file)

FILE *fopen(const char *path, const char *mode);
FILE *freopen(const char *path, const char *mode, FILE *f);
int fclose(FILE *f);
int fflush(FILE *f);
void setbuf(FILE *f, char *buf);

int fgetc(FILE *f);
char *fgets(char *s, int n, FILE *f);
char *gets(char *s);
int getchar(void);
int ungetc(int c, FILE *f);
size_t fread(void *p, size_t size, size_t n, FILE *f);
#define getc(f)		fgetc(f)

int fputc(int c, FILE *f);
int fputs(const char *s, FILE *f);
int puts(const char *s);
int putchar(int c);
size_t fwrite(const void *p, size_t size, size_t n, FILE *f);
#define putc(c, f)	fputc(c, f)

int fseek(FILE *f, long offset, int whence);
long ftell(FILE *f);

/* libaux's, with C99's conversions, rather than the shared library's */
int printf(const char *fmt, ...);
int fprintf(FILE *f, const char *fmt, ...);
int sprintf(char *s, const char *fmt, ...);

/* libaux */
int asprintf(char **sp, const char *fmt, ...);
int dprintf(int fd, const char *fmt, ...);
FILE *fdopen(int fd, const char *mode);
void perror(const char *s);
int remove(const char *path);
int rename(const char *path, const char *newpath);
int snprintf(char *s, size_t size, const char *fmt, ...);
int vasprintf(char **sp, const char *fmt, va_list ap);
int vdprintf(int fd, const char *fmt, va_list ap);
int vfprintf(FILE *f, const char *fmt, va_list ap);
int vprintf(const char *fmt, va_list ap);
int vsnprintf(char *s, size_t size, const char *fmt, va_list ap);
int vsprintf(char *s, const char *fmt, va_list ap);

#endif
