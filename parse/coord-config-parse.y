%{
	#include <stdio.h>
	#include <unistd.h>
	#include <string.h>
	#include <stdint.h>

	#define YYDEBUG 1
	int yylex();
	static uint16_t short_addr, pan_addr;
	static uint8_t hwaddr[8];
	void yyerror(char *s)
	{
		printf("Error: %s\n", s);
	}
	static void init_data(void)
	{
		memset(hwaddr, 0, 8);
		short_addr = pan_addr = 0;
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
	static void do_commit_data()
	{
	}
%}

%union {
	unsigned long number;
	unsigned char hw_addr[8];
}

%error-verbose
%type <hw_addr> hardaddr
%token <number> TOK_NUMBER
%type <hw_addr> cmd_hwaddr
%type <number> num_col cmd_pan cmd_shortaddr
%type <number> num 

%token TOK_LEASE
%token TOK_HWADDR
%token TOK_SHORTADDR
%token TOK_PAN

%left ':' ';' '{' '}'

%%
input:  /* empty */
	| block input
	;

block:  lease_begin operators lease_end	{do_commit_data();}
	;

lease_begin: TOK_LEASE lbrace {init_data();}
	;
lease_end: rbrace semicolon {dump_data();}
	;

operators: cmd semicolon
	| cmd semicolon operators
	;

cmd:      cmd_hwaddr				{do_set_hw_addr($1);}
	| cmd_pan				{do_set_pan_addr($1);}
	| cmd_shortaddr				{do_set_short_addr($1);}
	;
cmd_hwaddr: TOK_HWADDR ' ' hardaddr		{memcpy($$, $3, 8);}
	;
cmd_pan:  TOK_PAN ' ' TOK_NUMBER		{$$ = $3;}
	;
cmd_shortaddr:  TOK_SHORTADDR ' ' TOK_NUMBER	{$$ = $3;;}
	;

colon: space_or_not ':' space_or_not
	;
semicolon: space_or_not ';' space_or_not
	;
lbrace: space_or_not '{' space_or_not
	;

rbrace: space_or_not '}' space_or_not
	;
space_or_not: /* nothing */
	| ' '
	;

hardaddr: num_col num_col num_col num_col num_col num_col num_col num
				{set_hwaddr_octets($$, $1, $2, $3, $4, $5, $6, $7, $8);}
	;

num_col: num colon {$$ = $1;}
	;
num: TOK_NUMBER
	;


%%
extern FILE * yyin;
void do_parse()
{
	yyin = fopen(LEASE_FILE, "r");
	if (!yyin)
		return;
	yyparse();
	fclose(yyin);
}

