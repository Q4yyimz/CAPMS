#include "InputUtils.h"

#include <iostream>
#include <limits>

void clearInputBuffer()
{
    std::cin.ignore(
        std::numeric_limits<std::streamsize>::max(),
        '\n'
    );
}