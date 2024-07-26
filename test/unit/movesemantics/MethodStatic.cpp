// RUN: %clangxx %s -emit-ast -o %t1.ast -std=c++17
// RUN: %cxx-langstat --analyses=msa -emit-features -in %t1.ast -out %t1.ast.json -- -std=c++17
// RUN: sed -i '/^[[:space:]]*"GlobalLocation/d' %t1.ast.json
// RUN: diff %t1.ast.json %s.json
// RUN: rm %t1.ast.json || true

// Small test with static member function.
//
//

#include <utility>

struct D{
    D();
    D(D&& d);
};

struct C{
    static void method(D d);
};

int main(int argc, char** argv){
    D d;
    C::method(std::move(d));
}
