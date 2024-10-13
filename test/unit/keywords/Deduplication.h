

struct Base {
    Base() = default;
    virtual void foo() = 0;
    void bar() noexcept {}
};

#define AMAZING_MACRO(X) \
struct X : public Base { \
    void foo() override {} \
}; 

struct Derived final : public Base {
    void foo() override {}
};

