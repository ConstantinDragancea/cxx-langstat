// RUN: %clangxx %s -emit-ast -o %t1.ast
// RUN: %cxx-langstat --analyses=ala,msa -emit-features -in %t1.ast -out %t1.ast.json --
// RUN: sed -i '/^[[:space:]]*"GlobalLocation/d' %t1.ast.json
// RUN: diff %t1.ast.json %s.json
// RUN: rm %t1.ast.json || true

// Test to check the different kinds of std::move are correctly distinguished.
//
//

#include <algorithm>
#include <utility>
#include <list>
#include <vector>

template<typename T>
void func(){}

// should be detected only by msa
template typename std::remove_reference<int>::type&& std::move( int&& t ) noexcept;

// should be detected only by ala
template std::vector<int>::iterator std::move( std::vector<int>::iterator first, std::vector<int>::iterator last, std::vector<int>::iterator d_first );

int main(int argc, char** argv){
    //
    std::vector<int> v;
    std::list<int> l;
    std::move(v.begin(), v.end(), std::back_inserter(l)); // ATTENTION: this call to algorithm:std::move will cause three move-constructed parameter to be detected
    //
    return std::move(0);

}
