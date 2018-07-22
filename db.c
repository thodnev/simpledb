#include "simpledb.h"
#include <argp.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


enum main_err {
	EM_OK=0,
	/* all following codes start from last lib errcode */
	EM_WRONGDIR=E_last_,
	EM_DIRCREATE,
	EM_DIROPEN,
	EM_ALLOC,
	EM_QUERY
};

static char *errmsg[] = {
 [EM_OK] = "\n",
 [E_FNAME] = "LibErr: Wrong file name\n",
 [E_OSERR] = "LibErr: OS routine returned error\n",
 [E_FOPEN] = "LibErr: File open\n",
 [E_FREAD] = "LibErr: File read\n",
 [E_FWRITE]= "LibErr: File write\n",
 [E_WRONGCRC]   = "LibErr: CRC mismatch. File corrupted\n",
 [EM_WRONGDIR] = "Error: Wrong directory specified\n",
 [EM_DIRCREATE] = "Error: Could not create directory\n",
 [EM_DIROPEN] = "Error: Could not open directory\n",
 [EM_ALLOC] = "Critical: Could not allocate memory\n",
 [EM_QUERY] = "Error: Wrong query specified. See -qhelp for details\n"
};

static char *qhelp_docstring = (
 "Query types:\n"
 "\tlist                       -- show all keys in db\n"
 "\tadd [key;fld1;fld2;...]    -- add entry to database\n"
 "\tget [key]                  -- show db entry named by key\n"
 "\tdel [key]                  -- remove entry specified by key from db\n"
 "\thelp                       -- display this message\n"
);


/* A routine to simplify --quiet arguments check and errmsg output */
static void err_exit(bool is_quiet, enum main_err code) {
	if (!is_quiet)
		perror(errmsg[code]);
	exit(code);
}

enum query_code{
	Q_LIST,
	Q_ADD,
	Q_GET,
	Q_DEL,
	Q_HELP
};

static char *query_name[] = {
 [Q_LIST] = "list",
 [Q_ADD] = "add",
 [Q_GET] = "get",
 [Q_DEL] = "del",
 [Q_HELP] = "help"
};

static struct dbitem make_item(const char *str)
{
	struct dbitem ret = {0};
	/* See expaination of how it works in main */
	/* ord: last -> first -> email	*/
	char *bkp, *ptr, *last_name, *first_name, *email, *date;
	bkp = ptr = last_name = first_name = email = date = strdup(str);	/* need it mutable */

	strsep(&ptr, ";");
	first_name = ptr;
	strsep(&ptr, ";");
	email = ptr;
	strsep(&ptr, ";");
	date = ptr;

	if ((last_name == NULL) || (strlen(last_name) < 1))
		last_name = "(none)";
	if ((first_name == NULL) || (strlen(first_name) < 1))
		first_name = "(none)";
	if ((email == NULL) || (strlen(email) < 3))
		email = "(wrong)";
	unsigned int dd, mm, yyyy;
	if ((date != NULL) && sscanf(date, "%u.%u.%u", &dd, &mm, &yyyy) == 3) {
		/* ret.birth_date is a bitfield, &operator unusable */
		ret.birth_date.day = dd;
		ret.birth_date.month = mm;
		ret.birth_date.year = yyyy;
	}

	strcpy(ret.last_name, last_name);
	strcpy(ret.first_name, first_name);
	strcpy(ret.email, email);
	
	free(bkp);	/* strdup calls malloc, so need to free mem */
	return ret;
}

/* Put version here. A good thing to do is to make it depend on git versioning.
 * This could be done, for example, via Makefile
 * https://engineering.taboola.com/calculating-git-version/
 */
//#ifdef VERSION
const char *argp_program_version = VERSION;	//stringification done outside
//#endif


/* An e-mail shown by default in --help for bug reports 		*/
/* See https://tinyurl.com/argp-globals for more 			*/
/* We don't need positional arguments for now.				*/
/*	const char *argp_program_bug_address =  "<thodnev@gmail.com>";	*/

/* A documentation string shown (only) in --help */
static const char *const args_docstring = {
 "An example of simple fs-backed database made to demonstrate the common dev cycle "
 "and core concepts of applications development with C.\n\n"
 "Program uses a directory with files, each file contains one database entry. File"
 " names are used as keys for searching (filesystem is used as key->value storage "
 "in this case)"
};

/* String shown in help, indicating positional arguments (not switches) */
//static char args_argdoc[] = "DIR QUERY";

/* The switches program accepts. See https://tinyurl.com/argp-option-vectors */
static struct argp_option args_parse_opts[] = {
 {"dir", 'd', "DIR", OPTION_ARG_OPTIONAL, "A directory containing DB files", 0},
 {"query", 'q', "QUERY", OPTION_ARG_OPTIONAL, "A query for DB. For sytax, see \"-q help\"", 0},
 {"gui", -1, 0, OPTION_ARG_OPTIONAL, "Run GUI instead of console", 0},
 {"quiet", 'Q', 0, OPTION_ARG_OPTIONAL, "Produce less output", 0},
 { 0 }	 /* Terminator. Required by argp */
};

struct args_container {
	char *dir, *query;	/* for DIR, QUERY values*/
	bool isgui:1;		/* for storing the --gui flag */
	bool isquiet:1;		/* for storing the --quiet flag */
};

/* Parses one parameter. This function is run for every param automatically */
static error_t args_parse_entry(int key, char arg[], struct argp_state *state)
{
	/* We pass pointer to args_container item in argp_parse function */
	/* in main(). Here we get it. And we know the type.              */
	struct args_container *args = state->input;

	switch (key) {
	case 'd':
		if (!arg)
			args->dir = state->argv[state->next];
		else
			args->dir = arg;
		break;
	case 'q':
		if (!arg)
			args->query = state->argv[state->next];
		else
			args->query = arg;
		break;
	case 'Q':
		args->isquiet = true;
		break;
	case -1:	/* the --gui has no short(-c) switch */
			/* see more on it: https://tinyurl.com/longswitch */
		args->isgui = true;
		break;
    	case ARGP_KEY_ARG:     /* see https://tinyurl.com/argp-special-keys  */
		/* Workaround to handle spaces between switches and values */
		if (state->arg_num >= 2)	/* too many args */
        		argp_usage(state);
		break;
	case ARGP_KEY_END:
		break;
	default:
      		return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static struct argp parser_config = { 
	.options=args_parse_opts, 
	.parser=args_parse_entry, 
	/* .args_doc=args_argdoc, */
	.doc=args_docstring 
};

int main(int argc, char *argv[])
{
	/* Fill with default arguments */
	struct args_container args = {
		.dir = ".",
		.query = NULL,
		.isgui = false,
		.isquiet = false
	};

	argp_parse(&parser_config, argc, argv, ARGP_IN_ORDER, 0, &args);

	dbg("Parsed argument values:\n"
	    "\tdir     = %s\n"
	    "\tquery   = %s\n"
	    "\tisgui   = %s\n"
	    "\tisquiet = %s\n",
	    args.dir, args.query,
	    args.isgui ? "true" : "false",
	    args.isquiet ? "true" : "false"
	);

	if (args.dir) {
		/* test that directory is accesible */
		DIR *d = opendir(args.dir);
		if (d != NULL) {
			/* directory exists -> OK -> close it	       */
			closedir(d);
			d = NULL;
		} else if (ENOENT == errno) {
			/* directory does not exist -> try creating it */
			if (mkdir(args.dir, 0755))
				err_exit(args.isquiet, EM_DIRCREATE);
		} else {
			/* failed to read directory for some other reasons */
			err_exit(args.isquiet, EM_DIROPEN);
		}
	}

	if (args.query) {
		char *qval = strdup(args.query);
		if (qval == NULL)
			err_exit(0, EM_ALLOC);	/* This one will never be muted by --quiet */
		char *qname = strsep(&qval, " ");
		dbg("qname=%s, qval=%s", qname, qval);
		/* Now our string is: [qname]\0[qval]\0, i.e. 2in1 distinguished by ptr   */
		const int n_allq = sizeof(query_name) / sizeof (*query_name);
		int qtype;
		/* Try 2 match passed qname to available, finding its index */
		for (qtype = 0; qtype < n_allq; qtype++) {
			if (!strcmp(qname, query_name[qtype]))
				break;
		}	
		/* Now qtype indicates our query type, which could be used in switch logic */

		int lerr = E_OK;
		switch(qtype) {
		case Q_HELP:
			printf("%s",qhelp_docstring);
			exit(EM_OK);
			/* break;  -- not needed because of exit*/ 
		case Q_LIST:;	/* ; empty statement */
			struct strvec v;
			if ((lerr = listdir(args.dir, &v)) != E_OK)
				err_exit(args.isquiet, lerr);
			printf("Lising keys in db:\n");
			unsigned long i;
			for (i = 0; i < v.n; i++) {
				printf(">>\t%s\n", v.vec[i]);
				free(v.vec[i]);	 /* Just 2 show how interface works */
			}
			free(v.vec);
			break;
		case Q_DEL:;
			char *key = qval;
			if ((lerr = item_remove_bykey(key, args.dir)) != E_OK)
				err_exit(args.isquiet, lerr);
			printf("DB entry \"%s\" removed\n", key);
			break;	
		case Q_GET:;
			char *fname = m_strjoin("/", args.dir, qval);
			struct dbitem itm;
			if ((lerr = item_read(fname, &itm)) != E_OK) {
				item_remove_bykey(qval, args.dir);
				printf("DB entry \"%s\" doesn't exist\n", qval);
				free(fname);
				err_exit(args.isquiet, lerr);
			}
			printf("DB entry \"%s\":\n"
			       "\tlast_name: %s\n"
			       "\tfirst_name: %s\n"
			       "\temail: %s\n"
			       "\tbirth_date: %02u.%02u.%u\n",
			       qval, itm.last_name, itm.first_name, itm.email,
			       itm.birth_date.day, itm.birth_date.month, 
			       itm.birth_date.year);
			free(fname);
			break;
		case Q_ADD:;
			struct dbitem item = make_item(qval);
			char *fn = m_strjoin("/", args.dir, item.key);
			if ((lerr = item_write(fn, &item)) != E_OK) {
				free(fn);
				err_exit(args.isquiet, lerr);
			}
			printf("DB entry \"%s\" created\n", qval);
			free(fn);
			break;
		default:	/* No match found */
			err_exit(args.isquiet, EM_QUERY); /* rely on alloc cleanup @ exit */
		}
		free(qname);
	}
	return 0;
}
