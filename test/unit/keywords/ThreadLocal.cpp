// RUN: %clangxx %s -emit-ast -o %t1.ast
// RUN: %cxx-langstat --analyses=mka -emit-features -in %t1.ast -out %S/a.ast.json --
// RUN: sed -i '/^[[:space:]]*"GlobalLocation/d' %t1.ast.json
// RUN: diff %t1.ast.json %s.json
// RUN: rm %t1.ast.json || true

#define MAGIC(X) struct X { static thread_local int x; };

thread_local int x = 0;

MAGIC(Struct);
struct ThreadLocalStruct {
    static thread_local int y;
};