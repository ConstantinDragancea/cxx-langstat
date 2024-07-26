
struct Base {
    Base() = default;
    virtual void foo() = 0;
    void bar() noexcept {}
};

struct Derived final : public Base {
    void foo() override {}
};

