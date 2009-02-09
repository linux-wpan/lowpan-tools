%{
	#include <stdio.h>
	#include <unistd.h>
	#include <string.h>
	#include <stdint.h>
	#include <libcommon.h>
	#include <time.h>

	int lineno;

	#define YYDEBUG 1
	int yylex();
	static uint16_t short_addr, pan_addr;
	static uint8_t hwaddr[8];
	static time_t mystamp;
	void yyerror(char *s)
	{
		printf("Error: %s at line %d\n", s, lineno);
	}
	static void init_data(void)
	{
		memset(hwaddr, 0, 8);
		short_addr = pan_addr = 0;
		mystamp = 0;
	}
	static void dump_data(void)
	{
		int i;
		printf("HW addr: ");
		for(i = 0; i < 8; i++)
			printf("%02x", hwaddr[i]);
		printf(" PAN id %u ", pan_addr);
		printf(" short addr %u", short_addr);
		printf("\n");
	}
	static void dump_addr(unsigned char *s)
	{
		int i;
		printf("HW addr: ");
		for(i = 0; i < 8; i++)
			printf("%02x", s[i]);
	}
	static void set_hwaddr_octets(unsigned char *s, unsigned char a1,
				unsigned char a2,  unsigned char a3,  unsigned char a4,
				unsigned char a5,  unsigned char a6,  unsigned char a7,
				 unsigned char a8)
	{
		s[0] = a1;
		s[1] = a2;
		s[2] = a3;
		s[3] = a4;
		s[4] = a5;
		s[5] = a6;
		s[6] = a7;
		s[7] = a8;
	}
	static void do_set_hw_addr(unsigned char *s)
	{
		memcpy(hwaddr, s, 8);
	}
	static void do_set_pan_addr(unsigned short addr)
	{
		pan_addr = addr;
	}
	static void do_set_short_addr(unsigned short addr)
	{
		short_addr = addr;
	}
	static void do_set_timestamp(time_t value)
	{
		mystamp = value;
	}
	static void do_commit_data()
	{
		addrdb_insert(hwaddr, short_addr, mystamp);
	}

%}

%union {
	unsigned long number;
	time_t timestamp;
	unsigned char hw_addr[8];
}

%error-verbose
%type <hw_addr> hardaddr
%token <number> TOK_NUMBER
%type <hw_addr> cmd_hwaddr
%type <number> num_col cmd_pan cmd_shortaddr
%type <timestamp> cmd_timestamp
%type <number> num 

%token TOK_LEASE
%token TOK_HWADDR
%token TOK_SHORTADDR
%token TOK_PAN
%token TOK_TIMESTAMP
%token TOK_LBRACE TOK_RBRACE TOK_COLON TOK_SEMICOLON

%%
input:  /* empty */
	| block input
	;

block:  lease_begin operators lease_end	{do_commit_data();}
	;

lease_begin: TOK_LEASE TOK_LBRACE {init_data();}
	;
lease_end: TOK_RBRACE TOK_SEMICOLON {dump_data();}
	;

operators: cmd TOK_SEMICOLON
	| cmd TOK_SEMICOLON operators
	;

cmd:      cmd_hwaddr				{do_set_hw_addr($1);}
	| cmd_pan				{do_set_pan_addr($1);}
	| cmd_shortaddr				{do_set_short_addr($1);}
	| cmd_timestamp				{do_set_timestamp($1);}
	;
cmd_hwaddr: TOK_HWADDR hardaddr			{memcpy($$, $2, 8);}
	;
cmd_pan:  TOK_PAN TOK_NUMBER			{$$ = $2;}
	;
cmd_shortaddr:  TOK_SHORTADDR TOK_NUMBER	{$$ = $2;}
	;
cmd_timestamp:  TOK_TIMESTAMP TOK_NUMBER	{$$ = (time_t) $2;}
	;

hardaddr: num_col num_col num_col num_col num_col num_col num_col num
				{set_hwaddr_octets($$, $1, $2, $3, $4, $5, $6, $7, $8);}
	;

num_col: num TOK_COLON {$$ = $1;}
	;
num: TOK_NUMBER
	;


%%
extern FILE * yyin;
int addrdb_parse(const char *fname)
{
	lineno = 1;
	yyin = fopen(fname, "r");
	if (!yyin)
		return -1;
	yyparse();
	fclose(yyin);
	return 0;
}

