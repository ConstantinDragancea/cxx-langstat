// RUN: %clangxx %s -emit-ast -o %t1.ast
// RUN: %cxx-langstat --analyses=cta -emit-features -in %t1.ast -out %S/a.ast.json --
// RUN: sed -i '/^[[:space:]]*"GlobalLocation/d' %t1.ast.json
// RUN: diff %t1.ast.json %s.json
// RUN: rm %t1.ast.json || true

#include <thread>
#include <utility>

using mythread = std::thread;

void funccccc(const std::thread* const * const * const * const x){

}
void funccccc2(const std::thread* const * const * const & x){

}

struct Base {
    static std::thread static_member_thread;
    std::thread member_thread;
};

static std::thread static_thread;

void foo(){
    static std::thread static_thread_in_foo;
    std::thread thread_in_foo;
    std::thread* thread_in_foo2;
}

void func(std::thread& thread_in_func){

}

void func2(std::thread&& thread_in_func2){

}

void func3(std::thread* thread_in_func){

}

void func3(std::thread* &thread_in_func){

}

std::thread createFunc(){
    return std::thread();
}

std::thread* createFunc2(){
    return new std::thread();
}

std::thread& createFunc3(){
    static std::thread lol;
    return lol;
}

mythread createFunc4(){
    return std::thread();
}

mythread* createFunc5(){
    // return new std::thread();
    return new mythread();
}

mythread& createFunc6(){
    static std::thread lol;
    return lol;
}

mythread** createFunc7(){
    // return new std::thread();
    return new mythread*();
}

int main(){
    std::thread thread_in_main;
    std::thread* thread_in_main2;
    std::thread** thread_in_main22;
    std::thread*** thread_in_main222;
    func(thread_in_main);
    func2(std::move(thread_in_main));

    mythread thread_in_main3;
    mythread* thread_in_main4;

    std::thread** arr = new std::thread*[10];
}