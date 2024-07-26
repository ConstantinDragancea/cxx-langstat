// RUN: %clangxx %s -emit-ast -o %t1.ast
// RUN: %cxx-langstat --analyses=mka -emit-features -in %t1.ast -out %t1.ast.json --
// RUN: sed -i '/^[[:space:]]*"GlobalLocation/d' %t1.ast.json
// RUN: diff %t1.ast.json %s.json
// RUN: rm %t1.ast.json || true


struct Dummy {
    Dummy(){}
    ~Dummy() noexcept {}
};

struct Dummy2 {
    Dummy2(){}
    ~Dummy2() {}
};

struct Dummy3 {
    Dummy3(){}
    ~Dummy3() noexcept(true) {}
};

struct Dummy4 {
    Dummy4(){}
    ~Dummy4() noexcept(false) {}
};

constexpr bool flag = false;

struct Dummy5 {
    Dummy5(){}
    ~Dummy5() noexcept(flag) {}
};

void func() noexcept(false) {}

struct Dummy6 {
    Dummy6() {}
    ~Dummy6() noexcept {
        func();
    }
};

constexpr bool flag2 = false;

struct Dummy7 {
    Dummy7() {}
    ~Dummy7() noexcept(flag2) {
    }
};

namespace noexcept_expr {
    bool a() noexcept;
    bool b() noexcept(true);
    bool c() noexcept(false);
    bool d() noexcept(noexcept(a()));
    bool e = noexcept(b()) || noexcept(c());
}


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