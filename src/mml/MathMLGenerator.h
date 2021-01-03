#pragma once

#include <ostream>

namespace TXL
{
class Lexer;

class MathMLGenerator final
{
public:
    MathMLGenerator(std::ostream& out);
    ~MathMLGenerator();

    void generate(const std::string& tex);
    void generateFromIN();

private:
    void generate(Lexer& in);

private:
    std::ostream& _out;
};
} // namespace TXL
