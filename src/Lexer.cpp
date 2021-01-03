#include "Lexer.h"
#include "Token.h"

extern "C"
{
int yylex();
std::size_t yyget_leng();
char *yyget_text();

void* initBuffer(const char* text, std::size_t size);
void freeBuffer(void* buffer);
}

namespace TXL
{
Lexer::Lexer(std::string_view text)
{
    _buffer = initBuffer(text.data(), text.size());
}

Lexer::~Lexer()
{
    freeBuffer(_buffer);
}

Token Lexer::next()
{
    auto result = yylex();
    return {TokenType(result), std::string_view(yyget_text(), yyget_leng())};
}
} // namespace TXL
