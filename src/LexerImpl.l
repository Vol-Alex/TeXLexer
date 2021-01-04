%option noyywrap

%{
#include "src/TokenType.h"
%}

%x _COMMAND_ _TEXT_

%%

"$"{1,2}
[ \t\n]
"\\"    {BEGIN(_COMMAND_);}

"{" |
"["     {return START_GROUP;}

"}" |
"]"     {return END_GROUP;}

"^" |
"+" |
"-" |
"="     {return SIGN;}

EOF     {return END;}

.       {BEGIN(_TEXT_); yyless(0);}

<_COMMAND_>
{
"{" |
"}" |
"[" |
"]" |
"^"          {BEGIN(INITIAL); return TEXT;}
[a-zA-Z]+    {BEGIN(INITIAL); return COMMAND;}
}

<_TEXT_>
{
[0-9]+                            {BEGIN(INITIAL); return DIGIT;}
[^\$\\{}\[\]\^\+\-\= \t\n0-9]+    {BEGIN(INITIAL); return TEXT;}
}

%%

void* initBuffer(const char* text, yy_size_t size)
{
    return yy_scan_bytes(text, size);
}

void freeBuffer(void* buffer)
{
    yy_delete_buffer((YY_BUFFER_STATE)buffer);
}

