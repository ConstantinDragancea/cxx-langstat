// RUN: %clangxx %s -emit-ast -o %t1.ast
// RUN: %cxx-langstat --analyses=mka -emit-features -in %t1.ast -out %t1.ast.json --
// RUN: sed -i '/^[[:space:]]*"GlobalLocation/d' %t1.ast.json
// RUN: diff %t1.ast.json %s.json
// RUN: rm %t1.ast.json || true

// Test what is the behaviour when the keywords appears in the expansion
// of some macro. The Location field stops working properly, but the
// GlobalLocation contains a Spelling= field which specifies the 
// argument passed to the macro.


struct Base {
    virtual void f() const {};
    virtual void g() const {};
};

#define MAGIC(X) class X : public Base { virtual void f() const override final {} virtual void g() const override final {} };

struct Derived : public Base {
    void f() const final override {};
};

MAGIC(Placeholder1) // produces 2 final keywords
MAGIC(Placeholder2) // produces another 2 final keywords