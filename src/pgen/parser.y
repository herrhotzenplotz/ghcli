%{
#include <stdio.h>

#include <gcli/pgen.h>

FILE	*outfile	 = NULL;
char	*outfilename = NULL;
int		 dumptype	 = 0;

%}

%token PARSER IS OBJECT WITH AS USE FATARROW
%token OPAREN CPAREN SEMICOLON ARRAY OF COMMA

%union {
	struct strlit		strlit;
	struct ident		ident;
	struct objentry		objentry;
	struct objentry     *objentries;
	struct objparser    objparser;
}

%token	<strlit>		STRLIT
%token	<ident>			IDENT

%type	<objentry>		obj_entry
%type	<objentries>	obj_entries
%type	<objparser>		parser

%%
input:			parser
				{
					printf("object parser: name = %s, type = %s\n",
						   $1.name,
						   $1.returntype);
					for (struct objentry *it = $1.entries; it != NULL; it = it->next)
						printf("  entry: kind = %s, name = %s, type = %s, parser = %s\n",
							   it->kind == OBJENTRY_SIMPLE ? "simple" : "array",
							   it->name, it->type, it->parser);
				}
		;
parser:			PARSER IDENT IS OBJECT OF IDENT WITH OPAREN obj_entries CPAREN SEMICOLON
				{
					$$.name		  = $2.text;
					$$.returntype = $6.text;
					$$.entries	  = $9;
				}
		;

obj_entries:	obj_entries COMMA obj_entry
				{
					$$ = malloc(sizeof(*($$)));
					*($$) = $3;
					$$->next = $1;
				}
		|		obj_entry
				{
					$$ = malloc(sizeof(*($$)));
					*($$) = $1;
					$$->next = NULL;
				}
		;

obj_entry:		STRLIT FATARROW IDENT AS IDENT
				{
					$$.kind	  = OBJENTRY_SIMPLE;
					$$.name	  = $3.text;
					$$.type	  = $5.text;
					$$.parser = NULL;
				}
		|		STRLIT FATARROW IDENT AS IDENT USE IDENT
				{
					$$.kind	  = OBJENTRY_SIMPLE;
					$$.name	  = $3.text;
					$$.type	  = $5.text;
					$$.parser = $7.text;
				}
		|		STRLIT FATARROW IDENT AS ARRAY OF IDENT USE IDENT
				{
					$$.kind	  = OBJENTRY_ARRAY;
					$$.name	  = $3.text;
					$$.type	  = $7.text;
					$$.parser = $9.text;
				}
		;
%%

#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern FILE *yyin;
extern char *yyfile;

static void
usage(void)
{
	fprintf(stderr, "usage: pgen [-v] [-o outputfile] [-t c|plain] [...]\n");
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "   -v       Print version and exit\n");
	fprintf(stderr, "   -o file  Dump output into the given file\n");
	fprintf(stderr, "   -t type  Type of the output. Can be either C or plain.\n");
}

int
main(int argc, char *argv[])
{
	int ch;

	while ((ch = getopt(argc, argv, "hvo:t:")) != -1) {
		switch (ch) {
		case 'o': {
			if (outfile)
				errx(1, "cannot specify -o more than once");
			outfile = fopen(optarg, "w");
			outfilename = optarg;
		} break;
		case 'v': {
			fprintf(stderr, "pgen version 0.1\n");
			exit(0);
		} break;
		case 't': {
			if (strcmp(optarg, "plain") == 0)
				dumptype = DUMP_PLAIN;
			else if (strcmp(optarg, "c") == 0)
				dumptype = DUMP_C;
			else
				errx(1, "invalid dump type %s", optarg);
		} break;
		case '?':
		case '-':
		default:
			usage();
			exit(1);
		}
	}

	argc -= optind;
	argv += optind;

	if (argc) {
		for (int i = 0; i < argc; ++i) {
			yyfile = argv[i];
			yyin = fopen(argv[i], "r");
			yyparse();
			fclose(yyin);
		}
	} else  {
		yyfile = "<stdin>";
		yyin = stdin;
		yyparse();
	}

	return 0;
}
