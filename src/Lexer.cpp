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
Lexer::Lexer() = default;

Lexer::Lexer(const std::string& text)
{
    _buffer = initBuffer(text.data(), text.size());
}

Lexer::~Lexer()
{
    if (_buffer)
    {
        freeBuffer(_buffer);
    }
}

Token Lexer::next()
{
    auto result = yylex();
    return {TokenType(result), std::string(yyget_text(), yyget_leng())};
}
} // namespace TXL
