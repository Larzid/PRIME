%{
#include <stdio.h>
extern int yylex();

// pascal routines
extern void begin_defn(char *name, char *abbr, char *tablnam, char *enmabbr);
extern void begin_rec(char *name);
extern void add_name_val(char *new_name, char *new_value);
extern void end_rec();
extern void end_defn();
extern void begin_templ(char *name, char *abbr);
extern void find_rec(char *name, int templ);
extern void override_name_val(char *new_name, char *new_value);
extern void exit_rec();
extern void end_templ();
extern void empty_inst(char *d_name, char *i_name);
extern void begin_inst(char *d_name, char *i_name);
extern void add_inst_val(char *new_name, char *new_value);
extern void end_inst();

// better error reporting
#define YYERROR_VERBOSE

// bison requires that you supply this function
extern char *yytext;
void yyerror(const char *msg)
{
      fprintf(stderr, "%s\n%s\n", msg, yytext);
}

%}

%union
{
        double dval;
        int ival;
        char cval;
        char token[81];
}

%token <ival> COMMENT
%token <cval> LP RP
%token <token> DECLARE DEFINE TEMPLATE WORD

%%

parse :
      | parse decl
      ;

decl : COMMENT
     | block_defn
     | block_templ
     | block_inst
     ;

defn_core : DEFINE WORD WORD WORD WORD          { begin_defn($2, $3, $4, $5); };

block_defn : defn_core LP defn_entries RP       { end_defn(); };

defn_entries : defn_entry
             | defn_record
             | defn_entries defn_entry
             | defn_entries defn_record
             ;

defn_entry : WORD WORD                          { add_name_val($1, $2); };

r_core : WORD                                   { begin_rec($1); };

defn_record : r_core LP defn_entries RP         { end_rec(); };



templ_core : TEMPLATE WORD WORD                 { begin_templ($2, $3); };

block_templ : templ_core LP templ_entries RP    { end_templ(); };

templ_entries : templ_entry
              | templ_record
              | templ_entries templ_entry
              | templ_entries templ_record
              ;

templ_entry : WORD WORD                         { override_name_val($1, $2); };

templ_r_core : WORD                             { find_rec($1, 1); };

templ_record : templ_r_core LP templ_entries RP { exit_rec(); };



inst_core : WORD WORD                           { begin_inst($1, $2); };

block_inst : inst_core LP inst_entries RP       { end_inst(); }
           | inst_core                          { end_inst(); }
           | DECLARE WORD WORD                  { empty_inst($2, $3); }
           ;

inst_entries : inst_entry
             | inst_entries inst_entry
             ;

i_r_core : WORD                                 { find_rec($1, 0); };

inst_entry : WORD WORD                          { add_inst_val($1, $2); }
           | i_r_core LP inst_entries RP        { exit_rec(); }
           ;

%%
