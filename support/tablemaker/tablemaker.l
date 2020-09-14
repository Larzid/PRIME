%{
#include "tablemaker.tab.h"

char *names[4] =
{
        "DEFINE",
        "DECLARE",
        "TEMPLATE",
        ""
};

%}

QUOTED           \"[^\"]+\"
LIST             \([^\)]+\)
IDENTIFIER       [[:graph:]]+

%option noyywrap

%%
"COMMENT"[\t\n ]*"{"[^}]*"}" { yylval.ival = yyleng; return COMMENT; }
"DEFINE"          { strcpy(yylval.token, names[0]); return DEFINE; }
"DECLARE"         { strcpy(yylval.token, names[1]); return DECLARE; }
"TEMPLATE"        { strcpy(yylval.token, names[2]); return TEMPLATE; }
"\{"              { return LP; }
"\}"              { return RP; }
";"               { return ';'; }
{QUOTED}          { strncpy(yylval.token, yytext, 70); yylval.token[69] = 0; return WORD; }
{LIST}            { strncpy(yylval.token, yytext, 70); yylval.token[69] = 0; return WORD; }
{IDENTIFIER}      { strncpy(yylval.token, yytext, 70); yylval.token[69] = 0; return WORD; }
[\t\n ]+          { }
.                 { printf("problem with: '%s'", yytext); exit(1); }
%%
