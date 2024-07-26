// RUN: %clangxx %s -emit-ast -o %t1.ast
// RUN: %cxx-langstat --analyses=mka -emit-features -in %t1.ast -out %t1.ast.json --
// RUN: sed -i '/^[[:space:]]*"GlobalLocation/d' %t1.ast.json
// RUN: diff %t1.ast.json %s.json
// RUN: rm %t1.ast.json || true


struct Base {
    explicit Base(int a) {}
    virtual void func(){

    }

    virtual void g() const {}
};

struct Derived final : public Base {
    Derived() noexcept : Base(1) {}

    void func() override {

    }

    void g() const final {}
};