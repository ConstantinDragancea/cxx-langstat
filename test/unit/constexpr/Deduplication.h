

static constexpr int k_int = 1;

constexpr int get_one(){
    return 1;
}

struct Base {
    constexpr int get_two(){
        return 2;
    }
};

template < unsigned T >
struct Fact {
    static constexpr int value = T * Fact<T - 1>::value;
};

template<>
struct Fact<1> {
    static constexpr int value = 1;
};

void pseudo_main(){
    static_assert(Fact<5>::value == 120, "Fact<5> should be 120.");
}

bool if_stmt(){
    if constexpr (Fact<1>::value != 1){
        return false;
    }
    return true;
}