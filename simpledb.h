#include <limits.h>
#include <stdio.h>

#ifdef DEBUG
	#define dbg(X, ...)	do {					\
		fprintf(stderr, ("LOG> " X "\n"), ## __VA_ARGS__);	\
	} while(0)
#else
	#define dbg(X, ...)
#endif

struct item_birth {	/* for storing the birth date. See usage below */
	unsigned int year:12;
	unsigned int month:4;
	unsigned int day:5;
};

struct dbitem {
	union {		/* excplicitly mark field as key -- used as filename */
		char 		last_name	[20 * MB_LEN_MAX];
		char 		key		[20 * MB_LEN_MAX];
	};
	char			first_name	[20 * MB_LEN_MAX];
	char			email		[30 * MB_LEN_MAX];
	struct item_birth	birth_date;
};

enum err {
	E_OK = 0,
	E_FNAME,	// Wrong file name
	E_OSERR,	// miscallenous os errors
	E_FOPEN,	// File open
	E_FREAD,	// File read
	E_FWRITE,	// File write
	E_WRONGCRC,		// CRC mismatch
	E_last_	// stores index of the last element
};

struct strvec {
	unsigned int n;	/* number of items			   */
	char **vec;	/* vector of dynamically allocated strings */
};

enum err item_read_f(FILE *, struct dbitem *);

enum err item_write_f(FILE *, struct dbitem *);

enum err item_read(const char *, struct dbitem *);

enum err item_write(const char *, struct dbitem *);

enum err item_remove_bykey(const char *, const char *);

enum err listdir(const char *, struct strvec *);

/* Warning: The resulting string is malloced.
 *          And memory *should* 2 be freed afterwards. 
 */
char *m_strjoin(const char *, const char *, const char *);

enum err item_remove_bykey(const char *, const char *);

FILE *xopen(const char *, const char *);