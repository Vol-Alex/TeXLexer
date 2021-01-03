#pragma once

#include "src/Lexer.h"

#include <ostream>

namespace TXL
{
class MathMLGenerator final
{
public:
    MathMLGenerator(std::ostream& out);
    ~MathMLGenerator();

    void generate(const std::string& tex);

private:
    std::ostream& _out;
};
} // namespace TXL
