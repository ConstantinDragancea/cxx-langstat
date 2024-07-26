// RUN: %clangxx %s -emit-ast -o %t1.ast
// RUN: %cxx-langstat --analyses=cta -emit-features -in %t1.ast -out %t1.ast.json --
// RUN: sed -i '/^[[:space:]]*"GlobalLocation/d' %t1.ast.json
// RUN: diff %t1.ast.json %s.json
// RUN: rm %t1.ast.json || true

#include <mutex>

using mymutex = std::recursive_mutex;

struct Base {
    static std::recursive_mutex static_member_thread;
    std::recursive_mutex member_thread;
};

static std::recursive_mutex static_thread;

void foo(){
    static std::recursive_mutex static_thread_in_foo;
    std::recursive_mutex thread_in_foo;
    std::recursive_mutex* thread_in_foo2;
}

void func(std::recursive_mutex& thread_in_func){

}

void func2(std::recursive_mutex&& thread_in_func2){

}

void func3(std::recursive_mutex* thread_in_func){

}

void func3(std::recursive_mutex* &thread_in_func){

}

std::recursive_mutex createFunc(){
    return std::recursive_mutex();
}

std::recursive_mutex* createFunc2(){
    return new std::recursive_mutex();
}

std::recursive_mutex& createFunc3(){
    static std::recursive_mutex lol;
    return lol;
}

mymutex createFunc4(){
    return std::recursive_mutex();
}

mymutex* createFunc5(){
    // return new std::recursive_mutex();
    return new mymutex();
}

mymutex& createFunc6(){
    static std::recursive_mutex lol;
    return lol;
}

mymutex** createFunc7(){
    // return new std::recursive_mutex();
    return new mymutex*();
}

int main(){
    std::recursive_mutex thread_in_main;
    std::recursive_mutex* thread_in_main2;
    std::recursive_mutex** thread_in_main22;
    std::recursive_mutex*** thread_in_main222;
    func(thread_in_main);
    func2(std::move(thread_in_main));

    mymutex thread_in_main3;
    mymutex* thread_in_main4;

    std::recursive_mutex** arr = new std::recursive_mutex*[10];
}