// RUN: %clangxx %s -emit-ast -o %t1.ast
// RUN: %cxx-langstat --analyses=cta -emit-features -in %t1.ast -out %t1.ast.json --
// RUN: sed -i '/^[[:space:]]*"GlobalLocation/d' %t1.ast.json
// RUN: diff %t1.ast.json %s.json
// RUN: rm %t1.ast.json || true

#include <atomic>
#include <vector>

template <typename T>
using myatomic = std::atomic<T>;

using atomic_bool = std::atomic<bool>;
using atomic_char = std::atomic<char>;
using atomic_long = std::atomic<long>;
using atomic_double = std::atomic<double>;

struct Base {
    static myatomic<int> static_member_atomic;
    myatomic<int> member_atomic;
    // std::atomic<int*> member_atomic;
};

using atomic_base = std::atomic<Base>;

struct D {
int x;
};

template class std::atomic<D>;

static myatomic<float> static_atomic;

void foo(){
    static myatomic<Base> static_atomic_in_foo;
    myatomic<Base> atomic_in_foo;
    myatomic<Base>* atomic_in_foo2;
}

void func(atomic_base& atomic_in_func){

}

void func2(atomic_base&& atomic_in_func2){

}

void func3(atomic_base* atomic_in_func){

}

void func4(atomic_base* &atomic_in_func){

}

atomic_base createFunc(){
    return atomic_base();
}

atomic_base* createFunc2(){
    return new atomic_base();
}

atomic_base& createFunc3(){
    static atomic_base lol;
    return lol;
}

atomic_base createFunc4(){
    return std::atomic<Base>();
}

std::atomic<short>* createFunc5(){
    return new std::atomic<short>();
}

atomic_base& createFunc6(){
    static atomic_base lol;
    return lol;
}

atomic_base** createFunc7(){
    return new atomic_base*();
}

std::atomic<Base> createFunc8(){
    return std::atomic<Base>();
}

int main(){
    atomic_base atomic_in_main;
    atomic_base* atomic_in_main2;
    atomic_base** atomic_in_main22;
    atomic_base*** atomic_in_main222;
    func(atomic_in_main);
    func2(std::move(atomic_in_main));

    atomic_base** arr = new atomic_base*[10];
    std::atomic<int*> lol;
    std::atomic<Base*> lol2;
    std::atomic<Base*>* lol3 = new std::atomic<Base*>();
}