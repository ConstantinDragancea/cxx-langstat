// RUN: true

#include <algorithm>
#include "Deduplication.h"

void not_duplicate(){
    int a = 1;
    int b = 2;
    std::swap(a, b);
}