#include <iostream>

#include "cxx-langstat/Analysis.h"
#include "cxx-langstat/Analyses/LoopKindAnalysis.h"
#include "cxx-langstat/Utils.h"

using namespace clang::ast_matchers;
using ordered_json = nlohmann::ordered_json;

//-----------------------------------------------------------------------------

void LoopKindAnalysis::analyzeFeatures(){
    // Analysis of prevalence of different loop statement, i.e. comparing for, while etc.
    auto ForMatches = Extractor.extract(*Context, "for",
    // forStmt(isExpansionInMainFile())
    forStmt(isExpansionInHomeDirectory(), unless(isExpansionInSystemHeader()))
    .bind("for"));
    auto WhileMatches = Extractor.extract(*Context, "while",
    // whileStmt(isExpansionInMainFile())
    whileStmt(isExpansionInHomeDirectory(), unless(isExpansionInSystemHeader()))
    .bind("while"));
    auto DoWhileMatches = Extractor.extract(*Context, "do-while",
    // doStmt(isExpansionInMainFile())
    doStmt(isExpansionInHomeDirectory(), unless(isExpansionInSystemHeader()))
    .bind("do-while"));
    auto RangeBasedForMatches = Extractor.extract(*Context, "range-for",
    // cxxForRangeStmt(isExpansionInMainFile())
    cxxForRangeStmt(isExpansionInHomeDirectory(), unless(isExpansionInSystemHeader()))
    .bind("range-for"));

    ordered_json loops;
    for(auto match : ForMatches)
        loops[ForLoopKey].push_back({
            {"Location", match.Location},
            {"GlobalLocation", *match.GlobalLocation}
        });
    for(auto match : WhileMatches)
        loops[WhileLoopKey].push_back({
            {"Location", match.Location},
            {"GlobalLocation", *match.GlobalLocation}
        });
    for(auto match : DoWhileMatches)
        loops[DoWhileLoopKey].push_back({
            {"Location", match.Location},
            {"GlobalLocation", *match.GlobalLocation}
        });
    for(auto match : RangeBasedForMatches)
        loops[RangeBasedForLoopKey].push_back({
            {"Location", match.Location},
            {"GlobalLocation", *match.GlobalLocation}
        });
    Features = loops;
}

void LoopKindAnalysis::processFeatures(const nlohmann::ordered_json& j){
    std::map<std::string, unsigned> m;
    for(const auto& [loopkind, locations] : j.items()){
        m.try_emplace(loopkind, locations.size());
    }
    Statistics[LoopKindPrevalencesKey] = m;
}

//-----------------------------------------------------------------------------
