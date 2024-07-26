// RUN: %clangxx %s -emit-ast -o %t1.ast
// RUN: %cxx-langstat --analyses=ala -emit-features -in %t1.ast -out %t1.ast.json --
// RUN: sed -i '/^[[:space:]]*"GlobalLocation/d' %t1.ast.json
// RUN: diff %t1.ast.json %s.json
// RUN: rm %t1.ast.json || true

// Test to check that the specialized std::swap with std::functions
// is detected
//

#include <functional>
#include <iostream>
 
void foo(const char* str, int x)
{
    std::cout << "foo(\"" << str << "\", " << x << ")\n";
}
 
void bar(const char* str, int x)
{
    std::cout << "bar(\"" << str << "\", " << x << ")\n";
}
 
int main()
{
    std::function<void(const char*, int)> f1{foo};
    std::function<void(const char*, int)> f2{bar};
 
    f1("f1", 1);
    f2("f2", 2);
 
    std::cout << "std::swap(f1, f2);\n";
    std::swap(f1, f2);
 
    f1("f1", 1);
    f2("f2", 2);
}