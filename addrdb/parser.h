#ifndef PARSER_H
#define PARSER_H

#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif

#ifndef YY_EXTRA_TYPE
#define YY_EXTRA_TYPE void *
#endif

int addrdb_parser_init(yyscan_t *scanner, const char *fname);
void addrdb_parser_destroy(yyscan_t scanner);

int yylex (YYSTYPE * yylval_param ,YYLTYPE * yylloc_param ,yyscan_t yyscanner);
#endif
