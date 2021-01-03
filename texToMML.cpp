#include "src/mml/MathMLGenerator.h"

#include <iostream>

using namespace TXL;

int main()
{
    MathMLGenerator gen(std::cout);
    gen.generateFromIN();
    return 0;
}
