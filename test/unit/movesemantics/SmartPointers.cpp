// RUN: %clangxx %s -emit-ast -o %t1.ast -std=c++17
// RUN: %cxx-langstat --analyses=msa -emit-features -in %t1.ast -out %t1.ast.json -- -std=c++17
// RUN: sed -i '/^[[:space:]]*"GlobalLocation/d' %t1.ast.json
// RUN: diff %t1.ast.json %s.json
// RUN: rm %t1.ast.json || true

// Same as with Vector.cpp, same problem with CXXBindTemporaryExpr.
//
//

#include <memory>

class C{};

void func_p(std::unique_ptr<C> c_p);

int main(int argc, char** argv){
    std::unique_ptr<C> c_p = std::make_unique<C>();
    // func_p(c_p); would illegally call copy constructor of unique_ptr
    func_p(std::move(c_p));
    func_p(std::move(std::make_unique<C>()));

}

// can try hasDescendant(cxxConstructExpr()), but don't want to match too much then
// or hasDescendant(cxxConstructExpr) or is(cxxConstructExpr)
