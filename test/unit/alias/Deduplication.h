
// Typedef & alias
struct tuple_int {
    int x;
    int y;
};

typedef tuple_int pair_typedef; // typedef // √
using pair_alias = tuple_int; // alias // √

template<typename T>
struct tuple {
    T x;
    T y;
};

// Best example of a "typedef template". typedef public by default
template<typename T> // √
struct Tpair_typedef {
    typedef tuple<T> type;
};

// alias template
template<typename T> // √
using Tpair_alias = tuple<T>;