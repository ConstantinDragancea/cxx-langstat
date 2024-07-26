// RUN: %clangxx %s -emit-ast -o %t1.ast -std=c++17
// RUN: %cxx-langstat --analyses=msa -emit-features -in %t1.ast -out %t1.ast.json -- -std=c++17
// RUN: sed -i '/^[[:space:]]*"GlobalLocation/d' %t1.ast.json
// RUN: diff %t1.ast.json %s.json
// RUN: rm %t1.ast.json || true

// Same as the Move*.cpp test, but with method/member functions instead of
// non-member functions.
//

#include <utility>

struct D{
    D();
    D(const D& other);
};

struct E{
    E();
    E(const E& other);
    E(const E&& other);
};

struct C{
    void method(D d);
    void method2(E e);
};

int main(int argc, char** argv){
    C c;
    D d;
    E e;
    // Cause copy-construction in both cases because explicit copy constructor
    // of d causes no-compiler-generated move-constructor of D.
    c.method(d);
    c.method(std::move(d));
    c.method2(std::move(e));
}
