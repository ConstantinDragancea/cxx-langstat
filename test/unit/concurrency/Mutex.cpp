// RUN: %clangxx %s -emit-ast -o %t1.ast
// RUN: %cxx-langstat --analyses=cta -emit-features -in %t1.ast -out %t1.ast.json --
// RUN: sed -i '/^[[:space:]]*"GlobalLocation/d' %t1.ast.json
// RUN: diff %t1.ast.json %s.json
// RUN: rm %t1.ast.json || true

#include <mutex>

using mymutex = std::mutex;

struct Base {
    static std::mutex static_member_thread;
    std::mutex member_thread;
};

static std::mutex static_thread;

void foo(){
    static std::mutex static_thread_in_foo;
    std::mutex thread_in_foo;
    std::mutex* thread_in_foo2;
}

void func(std::mutex& thread_in_func){

}

void func2(std::mutex&& thread_in_func2){

}

void func3(std::mutex* thread_in_func){

}

void func3(std::mutex* &thread_in_func){

}

std::mutex createFunc(){
    return std::mutex();
}

std::mutex* createFunc2(){
    return new std::mutex();
}

std::mutex& createFunc3(){
    static std::mutex lol;
    return lol;
}

mymutex createFunc4(){
    return std::mutex();
}

mymutex* createFunc5(){
    // return new std::mutex();
    return new mymutex();
}

mymutex& createFunc6(){
    static std::mutex lol;
    return lol;
}

mymutex** createFunc7(){
    // return new std::mutex();
    return new mymutex*();
}

int main(){
    std::mutex thread_in_main;
    std::mutex* thread_in_main2;
    std::mutex** thread_in_main22;
    std::mutex*** thread_in_main222;
    func(thread_in_main);
    func2(std::move(thread_in_main));

    mymutex thread_in_main3;
    mymutex* thread_in_main4;

    std::mutex** arr = new std::mutex*[10];
}