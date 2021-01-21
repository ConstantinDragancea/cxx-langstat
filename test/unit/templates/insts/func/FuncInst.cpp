// RUN: clang++ %s -emit-ast -o %t1.ast
// RUN: %S/../../../../../build/cxx-langstat --analyses=tia -emit-features -in %t1.ast -out %t1.ast.json --
// RUN: diff %t1.ast.json %s.json

// Basic tests with function template instantiation that don't depend on each
// other.
//

template<typename T>
class Widget1 {

};

//-----------------------------------------------------------------------------
// Checking what happens when instantiating multiple times
template<typename T>
void f1(){}
// Explicit instantiation
template void f1<bool>();
// template void f1<bool>(); // duplicate explicit inst not legal
// Implicit instantiations
void caller1(){
    f1<int>(); //
    f1<int>(); // (same insts with same args only counted once)
}

// Checking when instantiating once explicit, implicit each to see which location
// is reported (as each reported only once)
template<typename T>
void f11(){}
template void f11<bool>();
void caller11(){
    f11<bool>();
    f11<int>();
}
template void f11<int>();

// Template template parameter
template<template<typename T> class C = Widget1>
void f2(){
}
template void f2<Widget1>(); // #3

// Non-type ptr
template<void* N>
void f3(){};
template void f3<nullptr>(); // #4

// With parameter pack
template<char... Cs>
void f4(){};
template void f4<'c'>(); // #5
template void f4<'p','p'>(); // #6

// With universal reference
template<typename T>
void f5(T&& t);
void caller5(){
    int a;
    f5(a); // univ. ref. -> T above will be deduced to int&
    f5(6); // univ. ref -> T will be deduced to int because 6 is rvalue
    // so above are same as:
    // f5<int&>(a);
    // f5<int>(6);
    // Note that the deduced type T of f5 will be counterintuitive
    // because of reference collapsing.
}
