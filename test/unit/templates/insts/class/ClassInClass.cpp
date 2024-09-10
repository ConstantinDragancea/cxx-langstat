// RUN: rm %t1.ast.json || true
// RUN: %clangxx %s -emit-ast -o %t1.ast
// RUN: %cxx-langstat --analyses=tia -emit-features -in %t1.ast -out %S/a.ast.json --
// RUN: sed -i '/^[[:space:]]*"GlobalLocation/d' %t1.ast.json
// RUN: diff %t1.ast.json %s.json

template <typename T>
struct A{
    struct B{
        T x;
    };
};

void function(){
    A<int> a;
    A<int>::B b;
}