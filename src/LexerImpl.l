%option noyywrap
%option reentrant
%option never-interactive
%option nounistd
%option prefix="txl"

%{
#include "src/TokenType.h"
%}

%x _COMMAND_ _TEXT_ _BEGIN_ENV_ _END_ENV_

%%

"$"{1,2}
[ \t\n]
"\\"    {BEGIN(_COMMAND_);}

"{" |
"["     {return START_GROUP;}

"}" |
"]"     {return END_GROUP;}

"&" |
"'" |
"(" |
")" |
"|" |
"^" |
"," |
"<" |
">" |
"_" |
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
"_" |
"^"         {BEGIN(INITIAL); return TEXT;}
"\\"        {BEGIN(INITIAL); return SIGN;}
"begin"     {BEGIN(_BEGIN_ENV_);}
"end"       {BEGIN(_END_ENV_);}
" " |
"," |
":" |
">" |
";" |
"!" |
"~" |
[a-zA-Z]+   {BEGIN(INITIAL); return COMMAND;}
.           {BEGIN(_TEXT_); yyless(0);}
}

<_BEGIN_ENV_>
{
"{"
"}"         {BEGIN(INITIAL);}
[a-zA-Z]+   {return BEGIN_ENV;}
}

<_END_ENV_>
{
"{"
"}"         {BEGIN(INITIAL);}
[a-zA-Z]+   {return END_ENV;}
}

<_TEXT_>
{
[0-9]+                                        {BEGIN(INITIAL); return DIGIT;}
[^'<>&\$\\{}\(\)\[\]\^\_\+\-\= \t\n0-9\,]+    {BEGIN(INITIAL); return TEXT;}
}

%%

void* initBuffer(const char* text, yy_size_t size, void* const scanCtx)
{
    return yy_scan_bytes(text, size, scanCtx);
}

void freeBuffer(void* buffer, void* const scanCtx)
{
    yy_delete_buffer((YY_BUFFER_STATE)buffer, scanCtx);
}

