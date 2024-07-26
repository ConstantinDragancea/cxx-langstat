#include <algorithm>
#include <vector>
#include <utility>

template void std::swap<bool>(bool&, bool&);

template < typename T >
void process_container(std::vector<T>& arr){
    sort(arr.begin(), arr.end());
}

void pseudo_main(){
    std::vector<int> arr = {2, 1, 3, 4, 2, 1, 3};
    process_container(arr);
    std::vector<double> arr2 = {2, 1, 3, 4, 2, 1, 3};
    process_container(arr2);
    std::vector<char> arr3 = {2, 1, 3, 4, 2, 1, 3};
    process_container(arr3);
}

std::vector<int> move_utility(std::vector<int>&& in){
    return in;
}

void pseudo_main2(){
    std::vector<int> arr = {2, 1, 3, 4, 2, 1, 3};
    std::vector<int> arr2 = move_utility(std::move(arr)); // this should not be detected as an "algorithm" call
}

struct Base {
    std::vector<int> arr;
};

void pseudo_main3(){
    std::vector<Base> vec1;
    std::vector<Base> vec2;
    std::move(vec1.begin(), vec1.end(), std::back_inserter(vec2));
}