// RUN: %clangxx %s -emit-ast -o %t1.ast
// RUN: %cxx-langstat --analyses=mka -emit-features -in %t1.ast -out %t1.ast.json --
// RUN: sed -i '/^[[:space:]]*"GlobalLocation/d' %t1.ast.json
// RUN: diff %t1.ast.json %s.json
// RUN: rm %t1.ast.json || true

#include <utility>

// empty class doesn't generate any default members
struct A {
};

// class with a virtual member function causes default operator= and the default destructors to be created
struct B {
    virtual void f();
};

struct C {

};

// Triggering the default constructors to be created. It's sufficient to require
// only the default constructor for the others to get generated
C c;

// No other constructor gets implicitly created
struct D {
    D() {}
};

struct E {
    E() {}
};

// The other constructors get implicitly created
E e;

// All the constructors, the destructors, and the operator= get implicitly created
struct F {
    virtual void f(){}
};

F f;