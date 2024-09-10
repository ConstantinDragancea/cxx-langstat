// RUN: %clangxx %s -emit-ast -o %t1.ast
// RUN: %cxx-langstat --analyses=ala -emit-features -in %t1.ast -out %S/a.ast.json --
// RUN: sed -i '/^[[:space:]]*"GlobalLocation/d' %t1.ast.json
// RUN: diff %t1.ast.json %s.json
// RUN: rm %t1.ast.json || true

// Small test to check usage of Algorithms library. Notice how some std::move
// are move as in move semantics, other are move as in algorithm?
// Make sure that ALA does only capture use of function template from algorithm lib.

#include <algorithm>
#include <utility>
#include <list>
#include <vector>

template<typename T>
void func(){}

// explicit instantiations
template void std::swap<bool>(bool&, bool&);
template const bool& std::min<bool>(const bool&, const bool&);
template const bool& std::max<bool>(const bool&, const bool&);

int main(int argc, char** argv){
    int a = std::max(3,4);
    int b = std::min(1.0, 2.0);
    func<bool>();
    func<bool>();
    std::min(1,2);
    std::max(3.0,4.0);
    std::max(std::move(3.9), 4.0);
    std::swap(a, b);

    std::vector<int> v = {3, 2, 1};
    std::vector<int> v2 = {3, 2, 1};
    std::list<int> l;
    std::move(v.begin(), v.end(), std::back_inserter(l));

    // implicit instantiation
    sort(v.begin(), v.end());
    sort(v2.begin(), v2.end());

    // still implicit instantiation
    using my_swap = decltype((void(*)(int&, int&))std::swap<int>);
    auto f = (void(*)(double&, double&)) std::swap<double>;
    // using my_sort = decltype(std::sort<__gnu_cxx::__normal_iterator<short *, std::vector<short, std::allocator<short> > >>);
    // auto f = std::sort<__gnu_cxx::__normal_iterator<double *, std::vector<double, std::allocator<double> > >>;

    return std::move(a);
}

// functionDecl(hasName("std::move"), isExpansionInFileMatching("algorithm"))

// callExpr(callee(functionDecl(hasName("std::move"), isTemplateInstantiation())), isExpansionInMainFile())
