#include <iostream>
#include <algorithm>

#include "cxx-langstat/Analysis.h"
#include "cxx-langstat/Analyses/ConstexprAnalysis.h"
#include "cxx-langstat/Utils.h"

using namespace clang;
using namespace clang::ast_matchers;
using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;

//-----------------------------------------------------------------------------

void ConstexprAnalysis::extractFeatures(){
    // Printing policy for printing parameter types
    LangOptions LO;
    PrintingPolicy PP(LO);
    // Causes some problems with instantiation-dependent types, namely
    // that no qualifications are printed
    PP.PrintCanonicalTypes = true;
    PP.SuppressTagKeyword = false;
    PP.SuppressScope = false;
    PP.SuppressUnwrittenScope = true;
    PP.FullyQualifiedName = true; // only effect for function decls!
    PP.Bool = true;
    //
    internal::VariadicDynCastAllOfMatcher<Decl, DecompositionDecl>
        decompositionDecl;
    
    // Extractic constexpr variables
    auto vr = Extractor.extract2(*Context,
        // varDecl(isExpansionInMainFile(),
        varDecl(isExpansionInHomeDirectory(),
            unless(isExpansionInSystemHeader()),
            // Inherit from VarDecl, but not constexpr-able, so don't count these
            unless(anyOf(parmVarDecl(), decompositionDecl())),
            hasInitializer(anything()))
            // Could add extra requirements here to get better relative usage
            // of constexpr variables, i.e. better comparision of variables
            // that are constexpr vs those that would be eligible to be constexpr
        .bind("v"));
    auto Var = getASTNodes<clang::VarDecl>(vr, "v");
    // This we need because of buggy VarTemplateSpecializationDecl
    if(!Var.empty())
        removeDuplicateMatches(Var);
    for(auto v : Var)
        Variables.emplace_back(CEDecl(v.Location, v.GlobalLocation, v.Node->isConstexpr(),
            v.getIdentifierName(), v.Node->getType().getAsString(PP)));
    
    // Extracting constexpr functions
    auto fr = Extractor.extract2(*Context,
        // functionDecl(isExpansionInMainFile(),
        functionDecl(
            isExpansionInHomeDirectory(),
            unless(isExpansionInSystemHeader()),
            // DeductionGuideDecl inherits from FunctionDecl, but not constexpr-able,
            // so don't count here. MethodDecl want to count separately
            unless(anyOf(cxxMethodDecl(), cxxDeductionGuideDecl())))
        .bind("f"));
    auto Func = getASTNodes<clang::FunctionDecl>(fr, "f");
    for(auto v : Func)
        NonMemberFunctions.emplace_back(CEDecl(v.Location, v.GlobalLocation, v.Node->isConstexpr(),
            v.getIdentifierName(), v.Node->getType().getAsString(PP)));
    
    // Extracting constexpr member functions
    auto mr = Extractor.extract2(*Context,
        cxxMethodDecl(
            // isExpansionInMainFile(),
            isExpansionInHomeDirectory(),
            unless(isExpansionInSystemHeader()),
            // Don't want to match compiler-generated ctors and operator='s
            unless(isImplicit()))
        .bind("m"));
    auto Method = getASTNodes<clang::FunctionDecl>(mr, "m");
    for(auto v : Method)
        MemberFunctions.emplace_back(CEDecl(v.Location, v.GlobalLocation, v.Node->isConstexpr(),
            v.getIdentifierName(), v.Node->getType().getAsString(PP)));
    
    // Extracting constexpr if statements
    auto ir = Extractor.extract2(*Context,
        // ifStmt(isExpansionInMainFile())
        ifStmt(isExpansionInHomeDirectory(), unless(isExpansionInSystemHeader()))
        .bind("i"));
    auto If = getASTNodes<clang::IfStmt>(ir, "i");
    for(auto v : If)
        IfStmts.emplace_back(CEIfStmt(v.Location, v.GlobalLocation, v.Node->isConstexpr()));
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CEInfo, Location, GlobalLocation, isConstexpr);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CEIfStmt, Location, GlobalLocation, isConstexpr);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CEDecl, Location, GlobalLocation, isConstexpr, Identifier,
    Type);

template<typename T>
void ConstexprAnalysis::featuresToJSON(const std::string& Kind, const std::vector<T>& features){
    for(auto feature : features){
        json feature_json = feature;
        Features[Kind].emplace_back(feature_json);
    }
}

void ConstexprAnalysis::analyzeFeatures(){
    extractFeatures();
    // Fill JSON with data about function (templates)
    featuresToJSON(VarKey, Variables);
    featuresToJSON(NonMemberFuncKey, NonMemberFunctions);
    featuresToJSON(MemberFuncKey, MemberFunctions);
    featuresToJSON(IfKey, IfStmts);
}

unsigned countConstexprOccurences(const ordered_json& j){
    return std::count_if(j.begin(), j.end(),
        [](ordered_json k){
            return k.at("isConstexpr").get<bool>();
        });
}

void constexprPopToJSON(ordered_json& Statistics, const ordered_json& j, llvm::StringRef t){
    auto cco = countConstexprOccurences(j);
    Statistics["constexpr usage"][t.str()]["yes"] = cco;
    Statistics["constexpr usage"][t.str()]["no"] = j.size() - cco;
}

void ConstexprAnalysis::processFeatures(const ordered_json& features){
    for(auto kind : {VarKey, NonMemberFuncKey, MemberFuncKey, IfKey}){
        if(features.contains(kind))
            constexprPopToJSON(Statistics, features.at(kind), kind);
    }
}

//-----------------------------------------------------------------------------
