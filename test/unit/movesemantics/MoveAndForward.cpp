// RUN: %clangxx %s -emit-ast -o %t1.ast -std=c++17
// RUN: %cxx-langstat --analyses=msa -emit-features -in %t1.ast -out %t1.ast.json -- -std=c++17
// RUN: sed -i '/^[[:space:]]*"GlobalLocation/d' %t1.ast.json
// RUN: diff %t1.ast.json %s.json
// RUN: rm %t1.ast.json || true


#include <iostream>
#include <utility> // For std::forward



void process(int& x) {
    std::cout << "Lvalue reference called with " << x << std::endl;
}

void process(int&& x) {
    std::cout << "Rvalue reference called with " << x << std::endl;
}

// Template function that uses a universal reference and forwards the argument
template<typename T>
void wrapper(T&& arg) {
    process(std::forward<T>(arg));
}

int main() {
    int x = 42;

    wrapper(x);        // Calls process(int& x)
    wrapper(42);       // Calls process(int&& x)
    wrapper(std::move(x)); // Calls process(int&& x)
    
    return 0;
}
