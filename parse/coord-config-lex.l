%{
#include "coord-config-parse.h"

extern int lineno;

static void process_lineno(char *s)
{
	while(*s) {
		if(*s == '\n') {
			lineno++;
		}
		s++;
	}
}
%}
%%
\{				return '{';
\}				return '}';
:				return ':';
;				return ';';
lease				return TOK_LEASE;
hwaddr				return TOK_HWADDR;
shortaddr			return TOK_SHORTADDR;
pan				return TOK_PAN;
timestamp			return TOK_TIMESTAMP;
0x[0-9a-fA-F]+			yylval.number = strtol(yytext, NULL, 16); return TOK_NUMBER;
[0-9A-Fa-f][0-9a-fA-F]		yylval.number = strtol(yytext, NULL, 16); return TOK_NUMBER;
[ \t\n\r]+			process_lineno(yytext); return ' ';
%%

