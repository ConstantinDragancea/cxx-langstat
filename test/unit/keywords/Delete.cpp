// RUN: %clangxx %s -emit-ast -o %t1.ast
// RUN: %cxx-langstat --analyses=mka -emit-features -in %t1.ast -out %t1.ast.json --
// RUN: sed -i '/^[[:space:]]*"GlobalLocation/d' %t1.ast.json
// RUN: diff %t1.ast.json %s.json
// RUN: rm %t1.ast.json || true

#include <cstddef>

struct some_type
{
    void* operator new(std::size_t) = delete;
    void* operator new[](std::size_t) = delete;
};

void deleted_function(int) = delete;