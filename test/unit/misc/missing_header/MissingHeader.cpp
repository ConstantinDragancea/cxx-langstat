// RUN: rm -f %S/a.ast.json
// RUN: mv %S/header.hpp.bak %S/header.hpp || true
// RUN: mv %S/header2.hpp.bak %S/header2.hpp || true
// RUN: %clangxx -emit-ast -o %t1.ast %s
// RUN: mv %S/header.hpp %S/header.hpp.bak
// RUN: mv %S/header2.hpp %S/header2.hpp.bak
// RUN: %cxx-langstat --analyses=cta -emit-features -in %t1.ast -out %S/a.ast.json --
// RUN: mv %S/header.hpp.bak %S/header.hpp
// RUN: mv %S/header2.hpp.bak %S/header2.hpp
// RUN: diff %t1.ast.json %s.json
// RUN: rm %t1.ast.json || true

// Test what happens when trying to analyze a feature that was defined/came from a file
// that no longer exists

#include "header.hpp"
// #include "header2.hpp"

int main(){
    my_namespace::my_class<int> c;
}