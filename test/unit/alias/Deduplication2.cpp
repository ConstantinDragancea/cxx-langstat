// RUN: true

#include "Deduplication.h"
#include <vector>

template < typename T >
struct MyPair {
    T x;
    T y;
};

template < typename T >
using MyPairs = std::vector<MyPair<T>>;