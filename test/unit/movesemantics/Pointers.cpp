// RUN: %clangxx %s -emit-ast -o %t1.ast -std=c++17
// RUN: %cxx-langstat --analyses=msa -emit-features -in %t1.ast -out %t1.ast.json -- -std=c++17
// RUN: sed -i '/^[[:space:]]*"GlobalLocation/d' %t1.ast.json
// RUN: diff %t1.ast.json %s.json
// RUN: rm %t1.ast.json || true

// Test to see if pointers are reported or not.
// They don't, because they don't produce a CXXConstructExpr.
// They are just a basic data type.

#include <utility>

class C{};

void func(C c);

void func_p(C* c);

int main(int argc, char** argv){
    C c;
    func(std::move(c));

    func_p(&c);

}
