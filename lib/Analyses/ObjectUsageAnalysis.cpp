#include "cxx-langstat/Analysis.h"
#include "cxx-langstat/Analyses/ObjectUsageAnalysis.h"
#include "cxx-langstat/Utils.h"

#include "clang/ASTMatchers/ASTMatchers.h"

#include <nlohmann/json.hpp>

#include <functional>

using namespace clang;
using namespace clang::ast_matchers;
using namespace llvm;
using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;

ObjectUsageAnalysis::ObjectUsageAnalysis()
    : ObjectUsageAnalysis(InstKind::Any, anything(), ".*") {}

ObjectUsageAnalysis::ObjectUsageAnalysis(InstKind IK,
    clang::ast_matchers::internal::Matcher<clang::NamedDecl> Names,
    const std::string& HeaderRegex) : IK(IK), Names(Names), HeaderRegex(HeaderRegex) {
    std::cout << "OUA ctor\n";
}



void ObjectUsageAnalysis::analyzeFeatures() {
    // variable delcarations of that type
    auto RelevantDefinitions = cxxRecordDecl(
        Names,
        isExpansionInFileMatching(HeaderRegex)
    ).bind("relevantdef");

    auto type = hasUnqualifiedDesugaredType(
      recordType(hasDeclaration(RelevantDefinitions)));

    // auto type_matcher = anyOf(
    //     hasType(type),
    //     // hasType(references(type)),
    //     // hasType(pointsTo(type)),
    //     // hasType(pointsTo(pointsTo(type))),
    //     // hasType(pointsTo(pointsTo(pointsTo(type))))
    //     hasType(recursivePointsTo(type))
    // );
    auto type_matcher = hasType(recursiveTypeMatcher(type));

    // matches variable, member variable, function parameter declarations
    // of that type or of reference type
    auto variable_matcher = declaratorDecl(
        anyOf(varDecl(), fieldDecl()),
        type_matcher,
        // unless(isImplicit()),
        isExpansionInHomeDirectory(),
        unless(isExpansionInSystemHeader())
    ).bind("var");

    auto VariableResults = Extractor.extract2(*Context, variable_matcher);
    auto VariablesNodes = getASTNodes<clang::DeclaratorDecl>(VariableResults, "var");

    Features[VarDeclFeatureKey] = json::object();
    for (const auto& it : VariablesNodes) {
        std::string object_name = it.getCleanTypeName();
        if (!Features[VarDeclFeatureKey].contains(object_name))
            Features[VarDeclFeatureKey][object_name] = json::array();

        Features[VarDeclFeatureKey][object_name].push_back({
            {"Location", it.Location},
            {"GlobalLocation", normalizeGlobalLocation(*it.GlobalLocation)}
        });
    }
    
    // function return types
    auto function_return_type = functionDecl(
        returns(recursiveTypeMatcher(type)),
        unless(isImplicit()),
        unless(isExpansionInSystemHeader()),
        isExpansionInHomeDirectory()
    ).bind("return-type");

    auto FunctionResults = Extractor.extract2(*Context, function_return_type);
    auto FunctionNodes = getASTNodes<clang::FunctionDecl>(FunctionResults, "return-type");

    Features[FuncRetTypeFeatureKey] = json::object();
    for (const auto& it : FunctionNodes) {
        std::string object_name = it.getCleanTypeName();
        if (!Features[FuncRetTypeFeatureKey].contains(object_name))
            Features[FuncRetTypeFeatureKey][object_name] = json::array();

        Features[FuncRetTypeFeatureKey][object_name].push_back({
            {"Location", it.Location},
            {"GlobalLocation", normalizeGlobalLocation(*it.GlobalLocation)}
        });
    }
    
    // temporary expressions, like std::thread()
    auto temporary_expr = cxxTemporaryObjectExpr(
        type_matcher,
        unless(isExpansionInSystemHeader()),
        isExpansionInHomeDirectory()
    ).bind("temp-expr");

    auto TempExprResults = Extractor.extract2(*Context, temporary_expr);
    auto TempExprNodes = getASTNodes<clang::CXXTemporaryObjectExpr>(TempExprResults, "temp-expr");

    Features[TemporaryExpressionsFeatureKey] = json::object();
    for (const auto& it : TempExprNodes) {
        std::string object_name = it.getCleanTypeName();
        if (!Features[TemporaryExpressionsFeatureKey].contains(object_name))
            Features[TemporaryExpressionsFeatureKey][object_name] = json::array();

        Features[TemporaryExpressionsFeatureKey][object_name].push_back({
            {"Location", it.Location},
            {"GlobalLocation", normalizeGlobalLocation(*it.GlobalLocation)}
        });
    }

    // temporary expressions like new std::thread()
    auto new_expr = cxxNewExpr(
        type_matcher,
        unless(isExpansionInSystemHeader()),
        isExpansionInHomeDirectory()
    ).bind("new-expr");

    auto NewExprResults = Extractor.extract2(*Context, new_expr);
    auto NewExprNodes = getASTNodes<clang::CXXNewExpr>(NewExprResults, "new-expr");

    Features[NewPointerExprFeatureKey] = json::object();
    for (const auto& it : NewExprNodes) {
        std::string object_name = it.getCleanTypeName();
        if (!Features[NewPointerExprFeatureKey].contains(object_name))
            Features[NewPointerExprFeatureKey][object_name] = json::array();

        Features[NewPointerExprFeatureKey][object_name].push_back({
            {"Location", it.Location},
            {"GlobalLocation", normalizeGlobalLocation(*it.GlobalLocation)}
        });
    }
}

void ObjectUsageAnalysis::processFeatures(const ordered_json& j) {
    size_t total_count = 0;
    
    for (const auto& feature_key : getFeatureKeys()) {
        if (!j.contains(feature_key))
            continue;
        
        Statistics[feature_key] = j.at(feature_key).size();
        total_count += j.at(feature_key).size();

        Statistics["total count"] = total_count;
    }
}