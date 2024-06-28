/*
 * Copyright 2022 Nico Sonack <nsonack@herrhotzenplotz.de>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided
 * with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

%option noyywrap

%{
#include <stdio.h>
#include <stdlib.h>

#include <gcli/pgen.h>
#include "y.tab.h"

int yycol = 1, yyrow = 1;
char *yyfile = NULL;

void
yyerror(const char *msg)
{
	fprintf(stderr, "%s:%d:%d: error: %s\n", yyfile, yyrow, yycol, msg);
	exit(1);
}

%}

%%
\n                                 { yycol = 1; yyrow += 1;             }
[ \t]                              { yycol += 1;                        }
parser                             { yycol += yyleng; return PARSER;    }
include                            { yycol += yyleng; return INCLUDE;   }
is                                 { yycol += yyleng; return IS;        }
object                             { yycol += yyleng; return OBJECT;    }
with                               { yycol += yyleng; return WITH;      }
as                                 { yycol += yyleng; return AS;        }
use                                { yycol += yyleng; return USE;       }
array                              { yycol += yyleng; return ARRAY;     }
of                                 { yycol += yyleng; return OF;        }
select                             { yycol += yyleng; return SELECT;    }
struct                             { yycol += yyleng; return STRUCT;    }
=>                                 { yycol += yyleng; return FATARROW;  }
"("                                { yycol += yyleng; return OPAREN;    }
")"                                { yycol += yyleng; return CPAREN;    }
";"                                { yycol += yyleng; return SEMICOLON; }
","                                { yycol += yyleng; return COMMA;     }
[A-Za-z][A-Za-z0-9_*.]*            {
                                       yycol += yyleng;
                                       yylval.ident.text = strdup(yytext);
                                       return IDENT;
                                   }
\"[^\"]*\"                         {
                                       yycol += yyleng;
                                       yylval.strlit.text = strdup(yytext + 1);
                                       yylval.strlit.text[strlen(yytext + 1) - 1] = '\0';
                                       return STRLIT;
                                   }
.                                  { yyerror("unrecognized character"); }
%%
