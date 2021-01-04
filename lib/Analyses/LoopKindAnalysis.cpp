#include <iostream>

#include "cxx-langstat/Analyses/LoopKindAnalysis.h"

using namespace clang::ast_matchers;

using ordered_json = nlohmann::ordered_json;


void LoopKindAnalysis::extract(clang::ASTContext& Context){
    // Analysis of prevalence of different loop statement, i.e. comparing for, while etc.
    auto ForMatches = Extractor.extract(Context, "for",
    forStmt(isExpansionInMainFile())
    .bind("for"));
    auto WhileMatches = Extractor.extract(Context, "while",
    whileStmt(isExpansionInMainFile())
    .bind("while"));
    auto DoWhileMatches = Extractor.extract(Context, "do-while",
    doStmt(isExpansionInMainFile())
    .bind("do-while"));
    auto RangeBasedForMatches = Extractor.extract(Context, "range-for",
    cxxForRangeStmt(isExpansionInMainFile())
    .bind("range-for"));

    ordered_json loops;
    for(auto match : ForMatches)
        loops["for"].emplace_back(match.location);
    for(auto match : WhileMatches)
        loops["while"].emplace_back(match.location);
    for(auto match : DoWhileMatches)
        loops["do-while"].emplace_back(match.location);
    for(auto match : RangeBasedForMatches)
        loops["range-for"].emplace_back(match.location);
    Result = loops;
}

void LoopKindAnalysis::run(llvm::StringRef InFile, clang::ASTContext& Context){
    std::cout << "\033[32mCounting the different kinds of loops:\033[0m"
    << std::endl;
    extract(Context);
}
