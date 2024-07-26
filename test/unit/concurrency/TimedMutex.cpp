// RUN: %clangxx %s -emit-ast -o %t1.ast
// RUN: %cxx-langstat --analyses=cta -emit-features -in %t1.ast -out %t1.ast.json --
// RUN: sed -i '/^[[:space:]]*"GlobalLocation/d' %t1.ast.json
// RUN: diff %t1.ast.json %s.json
// RUN: rm %t1.ast.json || true

#include <mutex>

using mymutex = std::timed_mutex;

struct Base {
    static std::timed_mutex static_member_thread;
    std::timed_mutex member_thread;
};

static std::timed_mutex static_thread;

void foo(){
    static std::timed_mutex static_thread_in_foo;
    std::timed_mutex thread_in_foo;
    std::timed_mutex* thread_in_foo2;
}

void func(std::timed_mutex& thread_in_func){

}

void func2(std::timed_mutex&& thread_in_func2){

}

void func3(std::timed_mutex* thread_in_func){

}

void func3(std::timed_mutex* &thread_in_func){

}

std::timed_mutex createFunc(){
    return std::timed_mutex();
}

std::timed_mutex* createFunc2(){
    return new std::timed_mutex();
}

std::timed_mutex& createFunc3(){
    static std::timed_mutex lol;
    return lol;
}

mymutex createFunc4(){
    return std::timed_mutex();
}

mymutex* createFunc5(){
    // return new std::timed_mutex();
    return new mymutex();
}

mymutex& createFunc6(){
    static std::timed_mutex lol;
    return lol;
}

mymutex** createFunc7(){
    // return new std::timed_mutex();
    return new mymutex*();
}

int main(){
    std::timed_mutex thread_in_main;
    std::timed_mutex* thread_in_main2;
    std::timed_mutex** thread_in_main22;
    std::timed_mutex*** thread_in_main222;
    func(thread_in_main);
    func2(std::move(thread_in_main));

    mymutex thread_in_main3;
    mymutex* thread_in_main4;

    std::timed_mutex** arr = new std::timed_mutex*[10];
}