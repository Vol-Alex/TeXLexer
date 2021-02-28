#include "Lexer.h"
#include "Token.h"

extern "C"
{
int txllex_init(void* scanCtx) ;
int txllex_destroy(void* scanCtx) ;

int txllex(void* scanCtx);
std::size_t txlget_leng(void* scanCtx);
char* txlget_text(void* scanCtx);

void* initBuffer(const char* text, std::size_t size, void* const scanCtx);
void freeBuffer(void* buffer, void* const scanCtx);
}

namespace TXL
{
Lexer::Lexer()
{
    txllex_init(&_scanCtx);
}

Lexer::Lexer(const std::string& text)
{
    txllex_init(&_scanCtx);
    _buffer = initBuffer(text.data(), text.size(), _scanCtx);
}

Lexer::~Lexer()
{
    if (_buffer)
    {
        freeBuffer(_buffer, _scanCtx);
    }

    if (_scanCtx)
    {
        txllex_destroy(_scanCtx);
    }
}

Token Lexer::next()
{
    auto result = txllex(_scanCtx);
    return {TokenType(result), std::string(txlget_text(_scanCtx), txlget_leng(_scanCtx))};
}
} // namespace TXL
