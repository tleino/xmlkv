#include <unistd.h>
#include <err.h>
#include <stdio.h>
#include <expat.h>
#include <string.h>
#include <sys/types.h>
#include <regex.h>

enum {
	MAX_PATTERN = 256,
	MAX_DEPTH = 256,
	MAX_BUF = 8192,
	MAX_TAGPATH = 256
};

static char	buf[MAX_BUF];
static char	tagPath[MAX_TAGPATH];
static regex_t	pattern[MAX_PATTERN];
static int	npattern;
static int	match[MAX_DEPTH];
static int	single;
static int	quiet;
static int	depth;
static int	pathonly;
static int	rootmode;
static int	rootmatch;
static char*	root;

static void XMLCALL
cdata(void *data, const XML_Char *s, int len)
{
	int i;

	if (match[depth] == 0)
		return;
	if (pathonly == 1)
		return;
	for (i = 0; i < len; i++) {
		if (single == 0 && s[i] == '\n') printf("\n\t");
		else if (single == 1 && s[i] == '\n') continue;
		else printf("%c", s[i]);
	}
	fflush(stdout);
}

static void
printpath(const char *path)
{
	if (quiet == 0) {
		printf("%s", path);
		if (pathonly == 0)
			putchar('=');
	}
	if (single == 0)
		printf("\n\t");
}

static void XMLCALL
start(void *data, const char *el, const char **attr)
{
	int i;

	if (strlen(tagPath) + strlen(el) + 1 >= MAX_TAGPATH-1)
		errx(1, "max tag path reached");
	if (depth == MAX_DEPTH)
		errx(1, "max depth reached");
	strcat(tagPath, "/");
	strcat(tagPath, el);
	depth++;
	if (rootmode == 1 && strcmp(root, tagPath) == 0) {
		printf("[%s]\n", tagPath);
		rootmatch = 1;
		return;
	}
	match[depth] = 0;
	for (i = 0; i < npattern; i++) {
		regmatch_t rm[1];
		if (regexec(&pattern[i], tagPath, 1, rm, 0) == 0) {
			match[depth] = 1;
			break;
		}
	}
	if (match[depth] == 0 && rootmatch == 0)
		return;
	if (depth > 0 && match[depth-1] == 1)
		printf("\n");
	printpath(tagPath);
}

static void XMLCALL
end(void *data, const char *el)
{
	if (match[depth]) {
		printf("\n");
		if (single == 0)
			printf("\n");
	}
	if (rootmatch == 1 && strcmp(tagPath, root) == 0) {
		printf("\n");
	}
	tagPath[strlen(tagPath)-strlen(el)-1] = '\0';
#if 0
	if (match[depth] == 1 && depth > 1 && match[depth-1] == 1) {
		printpath(tagPath);
	}
#endif
	match[depth] = 0;
	depth--;
}

static void parse(FILE *fp)
{
	XML_Parser p;
	int done;

	p = XML_ParserCreate(NULL);
	if (p == NULL)
		errx(1, "parser creation failed");
	XML_SetElementHandler(p, start, end);
	XML_SetCharacterDataHandler(p, cdata);	
	do {
		int len;

		len = (int)fread(buf, 1, MAX_BUF, fp);
		if (ferror(fp))
			err(1, "fread");
		done = feof(fp);
		if (XML_Parse(p, buf, len, done) == XML_STATUS_ERROR) {
			errx(1, "parse error line %lu:\n%s\n",
				XML_GetCurrentLineNumber(p),
				XML_ErrorString(XML_GetErrorCode(p)));
		}
	} while (!done);
	XML_ParserFree(p);
}

int
main(int argc, char *argv[])
{
	int i, ch, cflags;

#ifdef __OpenBSD__
	if (pledge("stdio", NULL) < 0)
		err(1, "pledge");
#endif
	cflags = REG_NOSUB | REG_ICASE;
	while ((ch = getopt(argc, argv, "psqr:h")) != -1) {
		switch (ch) {
		case 's':
			single = 1;
			break;
		case 'q':
			quiet = 1;
			break;
		case 'p':
			pathonly = 1;
			break;	
		case 'r':
			rootmode = 1;
			root = strdup(optarg);
			if (regcomp(&pattern[0], root, cflags) != 0)
				err(1, "regcomp");
			npattern++;
			break;
		case 'h':
		default:
			fprintf(stderr, "Usage: %s [-psq] [-r root] re ..\n",
			    argv[0]);
			fprintf(stderr,
			    "\t-s\t\tsingle\n"
			    "\t-q\t\tquiet\n"
			    "\t-p\t\tpath only\n"
			    "\t-r <root>\tbegin from root\n");
			return 1;
		}
	}
	argc -= optind;
	argv += optind;
	for (i = 0; i < argc; i++) {
		if (regcomp(&pattern[npattern], argv[i], cflags) != 0)
			err(1, "regcomp");
		npattern++;
	}
	if (npattern == 0) {
		if (regcomp(&pattern[0], ".", cflags) != 0)
			err(1, "regcomp");
		npattern++;
	}
	parse(stdin);

	for (i = 0; i < npattern; i++)
		regfree(&pattern[i]);

	if (root != NULL)
		free(root);

	return 0;
}
