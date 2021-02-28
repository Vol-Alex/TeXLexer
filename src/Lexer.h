#pragma once

#include "Token.h"

namespace TXL
{
class Lexer final
{
public:
    Lexer();
    Lexer(const std::string& text);
    ~Lexer();

    Token next();

private:
    void* _scanCtx = nullptr;
    void* _buffer = nullptr;
};
} // namespace TXL
