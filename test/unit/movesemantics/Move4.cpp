// RUN: %clangxx %s -emit-ast -o %t1.ast -std=c++17
// RUN: %cxx-langstat --analyses=msa -emit-features -in %t1.ast -out %t1.ast.json -- -std=c++17
// RUN: sed -i '/^[[:space:]]*"GlobalLocation/d' %t1.ast.json
// RUN: diff %t1.ast.json %s.json
// RUN: rm %t1.ast.json || true

// Test checking if by-value of template type parameter type parameter's
// construction is reported correctly. Special case of Move3.cpp, because now
// the constructed type is a template specialization type.

#include <utility>
#include <iostream>

template<typename T>
class C{
public:
    C(){std::cout << "ctor\n";}
    C(const C& c){std::cout << "copy ctor\n";}
    C(C&& c){std::cout << "move ctor\n";}
};

template<typename T>
void func(T c){
}


int main(int argc, char** argv){
    C<int> c;
    func(c); // copy ctor
    func(std::move(c)); // move ctor

}
