#ifndef OBJECTUSAGEANALYSIS_H
#define OBJECTUSAGEANALYSIS_H

#include <array>
#include <string_view>

#include "cxx-langstat/Analysis.h"

//-----------------------------------------------------------------------------

class ObjectUsageAnalysis : public Analysis {
public:
    enum class InstKind {
        Variable,
        Function,
        Any
    };

    // Ctor to let TIA look for any kind of instantiations
    ObjectUsageAnalysis();
    // Ctor to instrument TIA to look only for instantiations of kind IK
    // named any of "Names".
    ObjectUsageAnalysis(InstKind IK,
        clang::ast_matchers::internal::Matcher<clang::NamedDecl> Names,
        const std::string& HeaderRegex);
    ~ObjectUsageAnalysis(){
        std::cout << "OUA dtor\n";
    }
    std::string_view getShorthand() override {
        return ShorthandName;
    }
protected:
    InstKind IK;
    clang::ast_matchers::internal::Matcher<clang::NamedDecl> Names;
    std::string HeaderRegex;
    
    // Call exprs that cause an implicit function instantiation or refer to it.
    Matches<clang::CallExpr> Callers;
    // DeclRefExprs that refer to a function template, causing an explicit instantiation
    Matches<clang::DeclRefExpr> Instantiators;
    //
    Matches<clang::VarTemplateSpecializationDecl> VarInsts;
    
protected:
    // Responsible to fill vectors of matches
    void analyzeFeatures() override;
    void processFeatures(const nlohmann::ordered_json& j) override;
    
    // JSON keys
    static constexpr auto VarDeclFeatureKey = "as variable declarations";
    static constexpr auto FuncRetTypeFeatureKey = "as function return types";
    static constexpr auto TemporaryExpressionsFeatureKey = "as temporary expressions";
    static constexpr auto NewPointerExprFeatureKey = "as new pointer expressions";
    //
    const std::string ShorthandName = "oua";
public:
    static constexpr std::array<decltype(VarDeclFeatureKey), 4> getFeatureKeys() {
        return { VarDeclFeatureKey, FuncRetTypeFeatureKey, TemporaryExpressionsFeatureKey, NewPointerExprFeatureKey };
    }
};

//-----------------------------------------------------------------------------

#endif // OBJECTUSAGEANALYSIS_H
