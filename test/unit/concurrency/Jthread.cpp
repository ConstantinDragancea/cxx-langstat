// RUN: %clangxx %s -emit-ast -std=c++20 -o %t1.ast
// RUN: %cxx-langstat --analyses=cta -emit-features -in %t1.ast -out %t1.ast.json -- -std=c++20
// RUN: sed -i '/^[[:space:]]*"GlobalLocation/d' %t1.ast.json
// RUN: diff %t1.ast.json %s.json
// RUN: rm %t1.ast.json || true

#include <thread>
#include <utility>

using mythread = std::jthread;

struct Base {
    static std::jthread static_member_thread;
    std::jthread member_thread;
};

static std::jthread static_thread;

void foo(){
    static std::jthread static_thread_in_foo;
    std::jthread thread_in_foo;
    std::jthread* thread_in_foo2;
}

void func(std::jthread& thread_in_func){

}

void func2(std::jthread&& thread_in_func2){

}

void func3(std::jthread* thread_in_func){

}

void func3(std::jthread* &thread_in_func){

}

std::jthread createFunc(){
    return std::jthread();
}

std::jthread* createFunc2(){
    return new std::jthread();
}

std::jthread& createFunc3(){
    static std::jthread lol;
    return lol;
}

mythread createFunc4(){
    return std::jthread();
}

mythread* createFunc5(){
    // return new std::jthread();
    return new mythread();
}

mythread& createFunc6(){
    static std::jthread lol;
    return lol;
}

mythread** createFunc7(){
    // return new std::jthread();
    return new mythread*();
}

int main(){
    std::jthread thread_in_main;
    std::jthread* thread_in_main2;
    std::jthread** thread_in_main22;
    std::jthread*** thread_in_main222;
    func(thread_in_main);
    func2(std::move(thread_in_main));

    mythread thread_in_main3;
    mythread* thread_in_main4;

    std::jthread** arr = new std::jthread*[10];
}