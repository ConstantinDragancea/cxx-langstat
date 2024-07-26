// RUN: %clangxx %s -emit-ast -o %t1.ast
// RUN: %cxx-langstat --analyses=cta -emit-features -in %t1.ast -out %t1.ast.json --
// RUN: sed -i '/^[[:space:]]*"GlobalLocation/d' %t1.ast.json
// RUN: diff %t1.ast.json %s.json || true
// RUN: rm %t1.ast.json || true

#include <future>
#include <thread>
#include <mutex>

struct Counters { int a; int b; }; // user-defined trivially-copyable type

int main(){
    std::future<int> f;
    std::shared_future<int> sf;
    std::promise<int> p;
    std::async(std::launch::async, [](){ return 42; });

    std::atomic<Counters> cnt;

    std::thread t;
    std::mutex m;
}