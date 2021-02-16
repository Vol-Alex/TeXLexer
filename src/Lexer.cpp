#include "Lexer.h"
#include "Token.h"

extern "C"
{
int txllex();
std::size_t txlget_leng();
char *txlget_text();

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
    auto result = txllex();
    return {TokenType(result), std::string(txlget_text(), txlget_leng())};
}
} // namespace TXL
