// RUN: %clangxx %S/Deduplication1.cpp -emit-ast -o %S/Deduplication1.ast
// RUN: %clangxx %S/Deduplication2.cpp -emit-ast -o %S/Deduplication2.ast
// RUN: %cxx-langstat --analyses=ua -emit-features -in %S/Deduplication1.ast -in %S/Deduplication2.ast -outdir %S --
// RUN: %cxx-langstat --analyses=ua -emit-statistics -in %S/Deduplication1.ast.json -in %S/Deduplication2.ast.json -out %S/overall_stats.tmp --
// RUN: sed -i '/^[[:space:]]*"GlobalLocation/d' %S/Deduplication1.ast.json
// RUN: sed -i '/^[[:space:]]*"GlobalLocation/d' %S/Deduplication2.ast.json
// RUN: diff %S/Deduplication1.ast.json %S/Deduplication1.cpp.json
// RUN: diff %S/Deduplication2.ast.json %S/Deduplication2.cpp.json
// RUN: diff %S/overall_stats.tmp %S/overall_stats.deduplication
// RUN: rm %S/Deduplication1.ast %S/Deduplication2.ast %S/Deduplication1.ast.json %S/Deduplication2.ast.json %S/overall_stats.tmp



// Testing that matches in a user file are only counted once in the final statistics
// even if they appear multiple times in ASTs of different translation units due
// to being defined in a single header that is included in multiple places.

#include "Deduplication.h"
#include <vector>

template < typename T >
struct MyPair {
    T x;
    T y;
};

template < typename T >
using MyPairs = std::vector<MyPair<T>>;