%{
#include "coord-config-parse.h"
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
0x[0-9a-fA-F]+			yylval.number = strtol(yytext, NULL, 16); return TOK_NUMBER;
[0-9A-Fa-f][0-9a-fA-F]		yylval.number = strtol(yytext, NULL, 16); return TOK_NUMBER;
[ \t\n\r]+			return ' ';
%%

