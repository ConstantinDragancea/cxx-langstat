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

int pseudo_main() {
    int x = 42;

    wrapper(x);        // Calls process(int& x)
    wrapper(42);       // Calls process(int&& x)
    wrapper(std::move(x)); // Calls process(int&& x)
    
    return 0;
}
