// RUN: true

#include "Deduplication.h"

std::future<Base> future_base2;
std::promise<Base> promise_base2;
std::atomic<Base> atomic_base2;
std::thread t2;
std::mutex m2;