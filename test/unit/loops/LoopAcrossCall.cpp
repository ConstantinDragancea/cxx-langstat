// RUN: %clangxx %s -emit-ast -o %t1.ast
// RUN: %cxx-langstat --analyses=lda,lka -emit-features -in %t1.ast -out %t1.ast.json --
// RUN: sed -i '/^[[:space:]]*"GlobalLocation/d' %t1.ast.json
// RUN: diff %t1.ast.json %s.json
// RUN: rm %t1.ast.json || true

void callee1(){ //functionDecl(hasBody(compoundStmt(hasDescendant(forStmt()))))
    for(;;){
    }
}

void callee2(){ //functionDecl(hasBody(compoundStmt(hasDescendant(forStmt()))))
    if(true){
        for(;;){
        }
    }
}

void callee3(){ //functionDecl(hasBody(compoundStmt(hasDescendant(forStmt()))))
    if(true){

    }
    for(;;){
    }
}

void callee4(){ //functionDecl(hasBody(compoundStmt(hasDescendant(forStmt()))))
    if(true){

    }
    callee1();
    for(;;){
    }
}


void caller(){ //functionDecl(hasBody(compoundStmt(hasDescendant(forStmt()))))
    for(;;){        // forStmt(hasDescendant(callExpr()))
        callee1();
    }
}



int main(int argc, char** argv){
}
