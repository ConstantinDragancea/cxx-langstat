// RUN: %clangxx %s -emit-ast -o %t1.ast
// RUN: %cxx-langstat --analyses=cta -emit-features -in %t1.ast -out %t1.ast.json --
// RUN: sed -i '/^[[:space:]]*"GlobalLocation/d' %t1.ast.json
// RUN: diff %t1.ast.json %s.json
// RUN: rm %t1.ast.json || true

#include <atomic>
#include <vector>
#include <memory>

// template class std::atomic<int>;
template <typename T>
using myatomic = std::atomic<T>;

struct Base {
    std::atomic<char> member_atomic_char;
    myatomic<int> member_atomic_int;
};

struct NotDetected {

};

template <>
class std::atomic<NotDetected> {

};

struct ExplicitUserInst {

};

template class std::atomic<ExplicitUserInst>;

std::atomic<NotDetected> createAtomicUndetected(){
    return std::atomic<NotDetected>();
}

int main(){
    std::atomic<int> a;
    std::atomic<Base> b;
    std::atomic<NotDetected> bb;
    std::atomic<ExplicitUserInst> c;
    return 0;
}