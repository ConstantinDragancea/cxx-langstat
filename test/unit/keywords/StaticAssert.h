
void f(){
    constexpr int a = 2;
    static_assert(a == 2, "a is not 2");
}