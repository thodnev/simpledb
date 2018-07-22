#include "simpledb.h"

#include <zlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static const size_t HASH_SIZE = sizeof(uLong);


enum err item_read_f(FILE *fp, struct dbitem *item)
{
	uint8_t buffer[sizeof(*item) + HASH_SIZE];
	if (sizeof(buffer) != fread(buffer, 1, sizeof(buffer), fp))
		return E_FREAD;
	uLong crc_file = *(uLong *)buffer;
	uLong crc_calc = crc32(0, buffer + HASH_SIZE, sizeof(*item));
	dbg("CRC calc=0x%lX file=0x%lX", crc_calc, crc_file);
	if (crc_file != crc_calc)
		return E_WRONGCRC;
	memcpy(item, buffer + HASH_SIZE, sizeof(*item));	/* ! buffer[] won't exist after exit */
	return E_OK;
}

enum err item_write_f(FILE *fp, struct dbitem *item)
{
	uint8_t buffer[sizeof(*item)];
	memcpy(buffer, item, sizeof(*item));
	uLong crc_calc = crc32(0, buffer, sizeof(*item));
	if (sizeof crc_calc != fwrite(&crc_calc, 1, sizeof crc_calc, fp))
			return E_FWRITE;
	if (sizeof *item != fwrite(item, 1, sizeof *item, fp))
			return E_FWRITE;
	return E_OK;
}

/* Here function pointer is utilized for code reuse */ 
static inline enum err _item_do(enum err (*func)(FILE *, struct dbitem *), 
		                const char *pathname, struct dbitem *item)
{
	enum err retcode = E_OK;
	FILE *fp = xopen(pathname, "r+");
	if (fp == NULL)
		return E_FOPEN;
	retcode = (*func)(fp, item);
	fclose(fp);
	return retcode;
}

/* No need for copy-paste. Code is reused */
enum err item_read(const char *pathname, struct dbitem *item)
{
	return _item_do(&item_read_f, pathname, item);
}

/* Read kernel CodingStyle, ch. 12 for inline caveats */
enum err item_write(const char *pathname, struct dbitem *item)
{
	return _item_do(&item_write_f, pathname, item);
}

char *m_strjoin(const char *delimeter, const char *str1, const char *str2)
{
	unsigned long len = strlen(delimeter)+strlen(str1)+strlen(str2)+1;
	char *res = malloc(MB_CUR_MAX * len * sizeof(*res));
	if (NULL != res) {
		strcpy(res, str1);
		strcat(res, delimeter);
		strcat(res, str2);
	}
	dbg("Generated path %s", res);
	return res;
}

enum err item_remove_bykey(const char *key, const char *dir)
{
	enum err errlevel = E_OK;
	char *path = m_strjoin("/", dir, key);
	if (NULL == path) {
		errlevel = E_OSERR;
		goto cleanup_item_remove;
	};
	if (remove(path) < 0)
		errlevel = (EPERM == errno || ENOENT == errno) ? E_FNAME : E_OSERR;

	/* We need to free memory allocated. That's why it's so nasty.	*/
	/* In the end, that is the way of working with strings in C	*/
	cleanup_item_remove:
	free(path);
	return errlevel;
}

/* A filter function to leave only the files in resulting directory list
 * used in item_listdir below						*/
inline static int _onlyfiles_filter(const struct dirent * dir)
{
	return DT_REG == (dir->d_type);
}

/* see man man 3 scandir, man dirent.h, man 3 readdir */
enum err listdir(const char *dirname, struct strvec *vector)
{
	enum err errlevel = E_OK;
	struct dirent **namelist;
	int n = scandir(dirname, &namelist, &_onlyfiles_filter, alphasort);
	if (n < 0) {
		errlevel = E_OSERR;
		goto cleanup_item_listdir;
	}

	vector->vec = malloc(n * sizeof *vector->vec);	
	if(NULL == vector->vec) {
		errlevel = E_OSERR;
		goto namelist_dealloc;
	}
	/* Copy contents to vec. If error occurs, stop copy and stick with what we have */
	int i;
	for (i = 0; i < n; i++) {
		vector->vec[i] = strdup(namelist[i]->d_name);
		if (NULL == vector->vec[i]) break;
	}
	vector->n = i;
	if (i != n)
		errlevel = E_OSERR;

	cleanup_item_listdir:
	while (n--)
		free(namelist[n]);
	namelist_dealloc:
	free(namelist);
	return errlevel;
}


FILE *xopen(const char *pathname, const char *mode)
{
	int fd = open(pathname, O_RDWR | O_CREAT, 0666);  /* "a+" + creation */
	if (fd < 0)
		return NULL;
	return fdopen(fd, mode);
}