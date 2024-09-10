// RUN: rm -f %S/a.ast.json
// RUN: %clangxx %s -emit-ast -o %t1.ast
// RUN: %cxx-langstat --analyses=ula -emit-features -in %t1.ast -out %S/a.ast.json --
// RUN: sed -i '/^[[:space:]]*"GlobalLocation/d' %t1.ast.json
// RUN: diff %t1.ast.json %s.json
// RUN: rm %t1.ast.json || true

#include <memory>

int main(int argc, char** argv){
    std::shared_ptr<int> sp(new int(3));
    std::weak_ptr<double> wp;
    return 0;
}


// - Do we include parameter variable decls? Yes
// - Do we include variable template decls?
// - Declarations vs definitions, how do we count them? We count only decls,
//   as those are given by decl().
// - What about pointers to standard library types?
//   Not counted for now.
// - Qualifiers? access specifiers? static? extern? Don't care, all stripped
//   away.
// - What about templates? std::vector<T>? Ideally, returns that there is a
//   vector and reports that one of them had template type. Still problem with
//   returning base type.
// - Field decls? Indirect field decls? Reported separately.
// - What about references defined through structured bindings? Not relevant,
//   because it is definition, not declaration?
// - What about returning standard library type literals (e.g. return
//   std:vector<int>{1,2}) ?
//   Not done for now.

// Fundamental question: do we match decls with certain types or do we match
// types directly?
// We match declarations that have certain standard library types to be able
// to count those declarations s.t. we know how those library types are used.
