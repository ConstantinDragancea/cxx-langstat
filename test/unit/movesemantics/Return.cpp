// RUN: %clangxx %s -emit-ast -o %t1.ast -std=c++14
// RUN: %cxx-langstat --analyses=msa -emit-features -in %t1.ast -out %t1.ast.json -- -std=c++14
// RUN: sed -i '/^[[:space:]]*"GlobalLocation/d' %t1.ast.json
// RUN: diff %t1.ast.json %s.json
// RUN: rm %t1.ast.json || true

// MSA has not (yet) the ability to analyze if returned values are copied, moved
// or elided.
//


class C {

};

C f(){
    return C();
}

int main(int argc, char** argv){
    C c = f();
}
