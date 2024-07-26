#include <future>
#include <thread>
#include <atomic>
#include <mutex>

struct Base {

};

std::future<Base> future_base;
std::promise<Base> promise_base;
std::atomic<Base> atomic_base;
std::thread t1;
std::mutex m1;
std::atomic<int> a1;