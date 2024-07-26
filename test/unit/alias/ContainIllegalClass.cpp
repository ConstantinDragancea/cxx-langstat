// RUN: %clangxx %s -emit-ast -o %t1.ast
// RUN: %cxx-langstat --analyses=ua -emit-features -in %t1.ast -out %t1.ast.json --
// RUN: sed -i '/^[[:space:]]*"GlobalLocation/d' %t1.ast.json
// RUN: diff %t1.ast.json %s.json
// RUN: rm %t1.ast.json || true

// Test to ensure that classes inside class template cause the class template
// not to be registered as the typedef template idiom.
//

// "Typedef template"
template<typename T>
class C {
    typedef T type;
};

template<typename T>
class D {
    typedef T type; // just a typedef
    class E {

    };
};
