#include <iostream>
#include <algorithm>

#include "clang/ASTMatchers/ASTMatchers.h"
#include "cxx-langstat/Analyses/ModernKeywordsAnalysis.h"
#include "cxx-langstat/Utils.h"

#include "llvm/Support/Casting.h"

using namespace clang;
using namespace clang::ast_matchers;

using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;

using MKInfo = ModernKeywordsAnalysis::MKInfo;
using MKAlignment = ModernKeywordsAnalysis::MKAlignment;
using MKFinal = ModernKeywordsAnalysis::MKFinal;
using MKFunctionSpecifiers = ModernKeywordsAnalysis::MKFunctionSpecifiers;
using MKNoexcept = ModernKeywordsAnalysis::MKNoexcept;
using MKThreadLocal = ModernKeywordsAnalysis::MKThreadLocal;
using MKOverride = ModernKeywordsAnalysis::MKOverride;

using FinalKind = MKFinal::FinalKind;
using NoexceptKind = MKNoexcept::NoexceptKind;
using FunctionDeclKind = MKFunctionSpecifiers::FunctionDeclKind;

//-----------------------------------------------------------------------------
void to_json(json& j, const FinalKind& p){
    switch(p){
        case FinalKind::Class:
            j = "Class";
            break;
        case FinalKind::Method:
            j = "Method";
            break;
        case FinalKind::Unknown:
            j = "Unknown";
            break;
    }
}

void from_json(const json& j, FinalKind& p){
    std::string s = j.get<std::string>();
    if (s == "Class"){
        p = FinalKind::Class;
    }
    else if (s == "Method"){
        p = FinalKind::Method;
    }
    else if (s == "Unknown"){
        p = FinalKind::Unknown;
    }
}

void to_json(json& j, const FunctionDeclKind& p){
    switch(p){
        case FunctionDeclKind::Constructor:
            j = "Constructor";
            break;
        case FunctionDeclKind::CopyConstructor:
            j = "CopyConstructor";
            break;
        case FunctionDeclKind::MoveConstructor:
            j = "MoveConstructor";
            break;
        case FunctionDeclKind::MemberFunction:
            j = "MemberFunction";
            break;
        case FunctionDeclKind::Function:
            j = "Function";
            break;
        case FunctionDeclKind::Unknown:
            j = "Unknown";
            break;
    }
}

void from_json(const json& j, FunctionDeclKind& p){
    std::string s = j.get<std::string>();
    if (s == "Constructor"){
        p = FunctionDeclKind::Constructor;
    }
    else if (s == "CopyConstructor"){
        p = FunctionDeclKind::CopyConstructor;
    }
    else if (s == "MoveConstructor"){
        p = FunctionDeclKind::MoveConstructor;
    }
    else if (s == "MemberFunction"){
        p = FunctionDeclKind::MemberFunction;
    }
    else if (s == "Function"){
        p = FunctionDeclKind::Function;
    }
    else if (s == "Unknown"){
        p = FunctionDeclKind::Unknown;
    }
}

void to_json(json& j, const NoexceptKind& p){
    switch(p){
        case NoexceptKind::Specifier:
            j = "Specifier";
            break;
        case NoexceptKind::Operator:
            j = "Operator";
            break;
        case NoexceptKind::Unknown:
            j = "Unknown";
            break;
    }
}

void from_json(const json& j, NoexceptKind& p){
    std::string s = j.get<std::string>();
    if (s == "Specifier"){
        p = NoexceptKind::Specifier;
    }
    else if (s == "Operator"){
        p = NoexceptKind::Operator;
    }
    else if (s == "Unknown"){
        p = NoexceptKind::Unknown;
    }
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MKInfo, Location, GlobalLocation);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MKAlignment, Location, GlobalLocation, Alignment);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MKFinal, Location, GlobalLocation, Kind);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MKFunctionSpecifiers, Location, GlobalLocation, Kind, FunctionHeader);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MKNoexcept, Location, GlobalLocation, Kind, FunctionHeader);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MKThreadLocal, Location, GlobalLocation, Type);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MKOverride, Location, GlobalLocation, isOverride);

namespace {

inline FunctionDeclKind getConstructorType(const CXXConstructorDecl* ConstructorDecl){
    if (ConstructorDecl->isMoveConstructor())
        return FunctionDeclKind::MoveConstructor;
    if (ConstructorDecl->isCopyConstructor())
        return FunctionDeclKind::CopyConstructor;
    return FunctionDeclKind::Constructor;
}

} // anonymous namespace

void ModernKeywordsAnalysis::extractFeatures(){
    SourceManager& SM = Context->getSourceManager();

    // alignas <- store alignment value ?
    // https://en.cppreference.com/w/cpp/language/object#Alignment
    // The weakest alignment (the smallest alignment requirement) is the alignment of char, signed char, and unsigned char, which equals 1
    auto alignas_matches = Extractor.extract2(*Context,
        decl(hasAttr(clang::attr::Aligned), isExpansionInHomeDirectory(), unless(isExpansionInSystemHeader())).bind("alignas"));
    auto alignas_nodes = getASTNodes<clang::Decl>(alignas_matches, "alignas");
    for (auto& it : alignas_nodes){
        for (const auto *Attr : it.Node->attrs()){
            if (const auto *AA = llvm::dyn_cast<AlignedAttr>(Attr)){
                if (!AA->isAlignmentExpr())
                    continue;
                
                const Expr* AlignmentExpr = AA->getAlignmentExpr();
                Expr::EvalResult EvalResult;
                if (AlignmentExpr && !AlignmentExpr->isValueDependent() && AlignmentExpr->EvaluateAsInt(EvalResult, *Context)){
                    AlignAs.emplace_back(MKAlignment(it.Location, it.Node->getLocation().printToString(SM), *EvalResult.Val.getInt().getRawData()));
                }
                else {
                    AlignAs.emplace_back(MKAlignment(it.Location, it.Node->getLocation().printToString(SM), 0));
                }
            }
        }
    }

    // alignof
    // https://en.cppreference.com/w/cpp/language/object#Alignment
    // The weakest alignment (the smallest alignment requirement) is the alignment of char, signed char, and unsigned char, which equals 1
    auto alignof_matches = Extractor.extract2(*Context,
        unaryExprOrTypeTraitExpr(isExpansionInHomeDirectory(), unless(isExpansionInSystemHeader())).bind("alignof"));
    auto alignof_nodes = getASTNodes<clang::UnaryExprOrTypeTraitExpr>(alignof_matches, "alignof");

    for (auto& it : alignof_nodes){
        if (it.Node->getKind() == UETT_AlignOf){
            Expr::EvalResult EvalResult;
            if (!it.Node->isValueDependent() && it.Node->EvaluateAsInt(EvalResult, *Context)) {
                AlignOf.emplace_back(MKAlignment(it.Location, it.Node->getExprLoc().printToString(SM), *EvalResult.Val.getInt().getRawData()));
            }
            else {
                AlignOf.emplace_back(MKAlignment(it.Location, it.Node->getExprLoc().printToString(SM), 0));
            }
        }
    }

    // default
    auto explicit_default_matches = Extractor.extract2(*Context,
        functionDecl(
            isExpansionInHomeDirectory(),
            unless(isExpansionInSystemHeader()),
            isDefaulted(), 
            unless(isImplicit())
        )
    .bind("defaultDecl-explicit"));
    auto explicit_default_nodes = getASTNodes<clang::FunctionDecl>(explicit_default_matches, "defaultDecl-explicit");
    for (auto& it : explicit_default_nodes){
        MKFunctionSpecifiers fs(it.Location, it.Node->getLocation().printToString(SM), FunctionDeclKind::Unknown, getFunctionHeader(it.Node));

        if (const auto *ConstructorDecl = llvm::dyn_cast<CXXConstructorDecl>(it.Node)){
            fs.Kind = getConstructorType(ConstructorDecl);
        }
        else if (const auto *MethodDecl = llvm::dyn_cast<CXXMethodDecl>(it.Node)){
            fs.Kind = FunctionDeclKind::MemberFunction;
        }
        else {
            fs.Kind = FunctionDeclKind::Function;
        }

        ExplicitDefault.push_back(fs);
    }

    auto all_default_matches = Extractor.extract2(*Context,
        functionDecl(
            isExpansionInHomeDirectory(),
            unless(isExpansionInSystemHeader()),
            isDefaulted()
        )
    .bind("defaultDecl-all"));
    auto all_default_nodes = getASTNodes<clang::FunctionDecl>(all_default_matches, "defaultDecl-all");
    for (auto& it : all_default_nodes){
        MKFunctionSpecifiers fs(it.Location, it.Node->getLocation().printToString(SM), FunctionDeclKind::Unknown, getFunctionHeader(it.Node));

        if (const auto *ConstructorDecl = llvm::dyn_cast<CXXConstructorDecl>(it.Node)){
            fs.Kind = getConstructorType(ConstructorDecl);
        }
        else if (const auto *MethodDecl = llvm::dyn_cast<CXXMethodDecl>(it.Node)){
            fs.Kind = FunctionDeclKind::MemberFunction;
        }
        else {
            fs.Kind = FunctionDeclKind::Function;
        }

        AllDefault.push_back(fs);
    }

    // delete
    auto delete_matches = Extractor.extract2(*Context,
        functionDecl(isExpansionInHomeDirectory(), unless(isExpansionInSystemHeader()), isDeleted(), unless(isImplicit())).bind("deleteDecl"));
    auto delete_nodes = getASTNodes<clang::FunctionDecl>(delete_matches, "deleteDecl");
    for (auto& it : delete_nodes){
        MKFunctionSpecifiers fs(it.Location, it.Node->getLocation().printToString(SM), FunctionDeclKind::Unknown, getFunctionHeader(it.Node));

        if (const auto *ConstructorDecl = llvm::dyn_cast<CXXConstructorDecl>(it.Node)){
            fs.Kind = getConstructorType(ConstructorDecl);
        }
        else if (const auto *MethodDecl = llvm::dyn_cast<CXXMethodDecl>(it.Node)){
            fs.Kind = FunctionDeclKind::MemberFunction;
        }
        else {
            fs.Kind = FunctionDeclKind::Function;
        }

        Deleted.push_back(fs);
    }

    // inline namespace
    auto inline_namespace_matches = Extractor.extract2(*Context,
        namespaceDecl(isExpansionInHomeDirectory(), unless(isExpansionInSystemHeader()), isInline()).bind("inlineNamespace"));
    auto inline_namespace_nodes = getASTNodes<clang::NamespaceDecl>(inline_namespace_matches, "inlineNamespace");
    for (auto& it : inline_namespace_nodes){
        InlineNamespace.emplace_back(MKInfo(it.Location, it.Node->getLocation().printToString(SM)));
    }

    // final
    auto final_matches = Extractor.extract2(*Context,
        decl(hasAttr(clang::attr::Final), isExpansionInHomeDirectory(), unless(isExpansionInSystemHeader())).bind("finalDecl"));
    auto final_nodes = getASTNodes<clang::Decl>(final_matches, "finalDecl");
    for (auto& it : final_nodes){
        if (const auto *RecordDecl = llvm::dyn_cast<CXXRecordDecl>(it.Node)){
            Final.emplace_back(MKFinal(it.Location, it.Node->getLocation().printToString(SM), FinalKind::Class));
        }
        else if (const auto *MethodDecl = llvm::dyn_cast<CXXMethodDecl>(it.Node)){
            Final.emplace_back(MKFinal(it.Location, it.Node->getLocation().printToString(SM), FinalKind::Method));
        }
        else{
            Final.emplace_back(MKFinal(it.Location, it.Node->getLocation().printToString(SM), FinalKind::Unknown));
        }
    }

    // noexcept - specifier
    auto noexcept_specifier_explicit_matches = Extractor.extract2(*Context,
        functionDecl(
            isExpansionInHomeDirectory(), 
            unless(isExpansionInSystemHeader()), 
            unless(isImplicit()), 
            unless(isDefaulted())
        )
    .bind("noexcept-specifier-explicit"));
    auto noexcept_specifier_explicit_nodes = getASTNodes<clang::FunctionDecl>(noexcept_specifier_explicit_matches, "noexcept-specifier-explicit");

    auto noexcept_specifier_all_matches = Extractor.extract2(*Context,
        functionDecl(isExpansionInHomeDirectory(), unless(isExpansionInSystemHeader()))
    .bind("noexcept-specifier-all"));
    auto noexcept_specifier_all_nodes = getASTNodes<clang::FunctionDecl>(noexcept_specifier_all_matches, "noexcept-specifier-all");


    // noexcept - operator
    auto noexcept_operator_matches = Extractor.extract2(*Context,
        cxxNoexceptExpr(isExpansionInHomeDirectory(), unless(isExpansionInSystemHeader())).bind("noexceptExpr"));
    auto noexcept_operator_nodes = getASTNodes<clang::CXXNoexceptExpr>(noexcept_operator_matches, "noexceptExpr");
    
    for (auto& it : noexcept_specifier_explicit_nodes){
        if (it.Node->getExceptionSpecType() != EST_None)
            ExplicitNoexcept.emplace_back(MKNoexcept(
                it.Location, it.Node->getLocation().printToString(SM), NoexceptKind::Specifier, getFunctionHeader(it.Node)));
    }
    for (auto& it : noexcept_specifier_all_nodes){
        if (it.Node->getExceptionSpecType() != EST_None)
            AllNoexcept.emplace_back(MKNoexcept(
                it.Location, it.Node->getLocation().printToString(SM), NoexceptKind::Specifier, getFunctionHeader(it.Node)));
    }

    for (auto& it : noexcept_operator_nodes){
        ExplicitNoexcept.emplace_back(MKNoexcept(
            it.Location, it.Node->getExprLoc().printToString(SM), NoexceptKind::Operator, ""));
        AllNoexcept.emplace_back(MKNoexcept(
            it.Location, it.Node->getExprLoc().printToString(SM), NoexceptKind::Operator, ""));
    }
    
    // nullptr
    auto nullptr_matches = Extractor.extract2(*Context,
        cxxNullPtrLiteralExpr(isExpansionInHomeDirectory(), unless(isExpansionInSystemHeader())).bind("nullptr"));
    auto nullptr_nodes = getASTNodes<clang::CXXNullPtrLiteralExpr>(nullptr_matches, "nullptr");
    for (auto& it : nullptr_nodes){
        Nullptr.emplace_back(MKInfo(it.Location, it.Node->getExprLoc().printToString(SM)));
    }

    // thread_local
    auto thread_local_matches = Extractor.extract2(*Context,
        varDecl(hasThreadStorageDuration(), isExpansionInHomeDirectory(), unless(isExpansionInSystemHeader())).bind("thread_local"));
    auto thread_local_nodes = getASTNodes<clang::VarDecl>(thread_local_matches, "thread_local");
    for (auto& it : thread_local_nodes){
        ThreadLocal.emplace_back(MKThreadLocal(it.Location, it.Node->getLocation().printToString(SM), it.Node->getType().getAsString()));
    }

    // override, count how many functions override another one,
    // and how many actually contain the override keyword
    auto override_matches = Extractor.extract2(*Context,
        cxxMethodDecl(isExpansionInHomeDirectory(), unless(isExpansionInSystemHeader()), isOverride()).bind("override"));
    auto override_nodes = getASTNodes<clang::CXXMethodDecl>(override_matches, "override");
    for (auto& it : override_nodes){
        Override.emplace_back(MKOverride(it.Location, it.Node->getLocation().printToString(SM), it.Node->hasAttr<OverrideAttr>()));
    }

    // static_assert
    auto static_asserts_matches = Extractor.extract2(*Context,
        staticAssertDecl(isExpansionInHomeDirectory(), unless(isExpansionInSystemHeader())) // we want to catch static_asserts that were included from some header file
        .bind("static_assert"));
    auto static_asserts_nodes = getASTNodes<clang::StaticAssertDecl>(static_asserts_matches, "static_assert");
    for (auto& it : static_asserts_nodes){
        StaticAsserts.emplace_back(MKInfo(it.Location, it.Node->getLocation().printToString(SM)));
    }
}

template<typename T>
void ModernKeywordsAnalysis::featuresToJSON(const std::string& Kind, const std::vector<T>& features){
    Features[Kind] = json::array();
    for(auto feature : features){
        json feature_json = feature;
        Features[Kind].emplace_back(feature_json);
    }
}

void ModernKeywordsAnalysis::analyzeFeatures(){
    extractFeatures();
    featuresToJSON(AlignAsKey, AlignAs);
    featuresToJSON(AlignOfKey, AlignOf);
    featuresToJSON(AllDefaultKey, AllDefault);
    featuresToJSON(ExplicitDefaultKey, ExplicitDefault);
    featuresToJSON(DeleteKey, Deleted);
    featuresToJSON(InlineNamespaceKey, InlineNamespace);
    featuresToJSON(FinalKey, Final);
    featuresToJSON(AllNoexceptKey, AllNoexcept);
    featuresToJSON(ExplicitNoexceptKey, ExplicitNoexcept);
    featuresToJSON(NullptrKey, Nullptr);
    featuresToJSON(ThreadLocalKey, ThreadLocal);
    featuresToJSON(OverrideKey, Override);
    featuresToJSON(StaticAssertKey, StaticAsserts);
}

template<typename T>
void ModernKeywordsAnalysis::ProcessFeaturesKind(const std::string& Kind, const std::vector<T>& features){
    Statistics[ModernKeywordsPrevalenceKey][Kind] = json::object();
    Statistics[ModernKeywordsPrevalenceKey][Kind]["total count"] = features.size();
}

template<>
void ModernKeywordsAnalysis::ProcessFeaturesKind<MKAlignment>(const std::string& Kind, const std::vector<MKAlignment>& features){
    Statistics[ModernKeywordsPrevalenceKey][Kind] = json::object();
    Statistics[ModernKeywordsPrevalenceKey][Kind]["total count"] = features.size();

    std::unordered_map<size_t, size_t> alignment_values;
    for (auto& it : features){
        alignment_values[it.Alignment]++;
    }
    Statistics[ModernKeywordsPrevalenceKey][Kind]["values to count"] = json::object();
    for (auto& [alignment_value, alignment_count]: alignment_values){
        Statistics[ModernKeywordsPrevalenceKey][Kind]["values to count"][std::to_string(alignment_value)] = alignment_count;
    }
}

template<>
void ModernKeywordsAnalysis::ProcessFeaturesKind<MKFinal>(const std::string& Kind, const std::vector<MKFinal>& features){
    Statistics[ModernKeywordsPrevalenceKey][Kind] = json::object();
    Statistics[ModernKeywordsPrevalenceKey][Kind]["total count"] = features.size();
    Statistics[ModernKeywordsPrevalenceKey][Kind]["final methods"] = std::count_if(features.begin(), features.end(),
        [](const MKFinal& f){ return f.Kind == FinalKind::Method; });
    Statistics[ModernKeywordsPrevalenceKey][Kind]["final classes"] = std::count_if(features.begin(), features.end(),
        [](const MKFinal& f){ return f.Kind == FinalKind::Class; });
}

template<>
void ModernKeywordsAnalysis::ProcessFeaturesKind<MKFunctionSpecifiers>(const std::string& Kind, const std::vector<MKFunctionSpecifiers>& features){
    Statistics[ModernKeywordsPrevalenceKey][Kind] = json::object();
    Statistics[ModernKeywordsPrevalenceKey][Kind]["total count"] = features.size();
    Statistics[ModernKeywordsPrevalenceKey][Kind][Kind + " constructors"] = std::count_if(features.begin(), features.end(),
        [](const MKFunctionSpecifiers& f){ return f.Kind == FunctionDeclKind::Constructor; });
    Statistics[ModernKeywordsPrevalenceKey][Kind][Kind + " member functions"] = std::count_if(features.begin(), features.end(),
        [](const MKFunctionSpecifiers& f){ return f.Kind == FunctionDeclKind::MemberFunction; });
    Statistics[ModernKeywordsPrevalenceKey][Kind][Kind + " functions"] = std::count_if(features.begin(), features.end(),
        [](const MKFunctionSpecifiers& f){ return f.Kind == FunctionDeclKind::Function; });
}

template<>
void ModernKeywordsAnalysis::ProcessFeaturesKind<MKNoexcept>(const std::string& Kind, const std::vector<MKNoexcept>& features){
    Statistics[ModernKeywordsPrevalenceKey][Kind] = json::object();
    Statistics[ModernKeywordsPrevalenceKey][Kind]["total count"] = features.size();
    Statistics[ModernKeywordsPrevalenceKey][Kind]["specifier"] = std::count_if(features.begin(), features.end(),
        [](const MKNoexcept& f){ return f.Kind == NoexceptKind::Specifier; });
    Statistics[ModernKeywordsPrevalenceKey][Kind]["operator"] = std::count_if(features.begin(), features.end(),
        [](const MKNoexcept& f){ return f.Kind == NoexceptKind::Operator; });
}

template<>
void ModernKeywordsAnalysis::ProcessFeaturesKind<MKThreadLocal>(const std::string& Kind, const std::vector<MKThreadLocal>& features){
    Statistics[ModernKeywordsPrevalenceKey][Kind] = json::object();
    Statistics[ModernKeywordsPrevalenceKey][Kind]["total count"] = features.size();

    std::unordered_map<std::string, size_t> thread_local_data_types;
    for (auto& it : features){
        thread_local_data_types[it.Type]++;
    }

    Statistics[ModernKeywordsPrevalenceKey][Kind]["data types"] = json::object();
    for (auto& [data_type, data_type_count]: thread_local_data_types){
        Statistics[ModernKeywordsPrevalenceKey][Kind]["data types"][data_type] = data_type_count;
    }
}

template<>
void ModernKeywordsAnalysis::ProcessFeaturesKind<MKOverride>(const std::string& Kind, const std::vector<MKOverride>& features){
    Statistics[ModernKeywordsPrevalenceKey][Kind] = json::object();
    Statistics[ModernKeywordsPrevalenceKey][Kind]["contain override keyword"] = std::count_if(features.begin(), features.end(),
        [](const MKOverride& f){ return f.isOverride; });
    Statistics[ModernKeywordsPrevalenceKey][Kind]["total count"] = Statistics[ModernKeywordsPrevalenceKey][Kind]["contain override keyword"];
    Statistics[ModernKeywordsPrevalenceKey][Kind]["number of functions that override"] = features.size();
}

void ModernKeywordsAnalysis::processFeatures(const ordered_json& features){
    // alignas, aligonf: count occurences and frequency of each alignment value
    auto alignas_features = features.at(AlignAsKey).get<std::vector<MKAlignment>>();
    ProcessFeaturesKind(AlignAsKey, alignas_features);

    auto alignof_features = features.at(AlignOfKey).get<std::vector<MKAlignment>>();
    ProcessFeaturesKind(AlignOfKey, alignof_features);

    auto all_default_features = features.at(AllDefaultKey).get<std::vector<MKFunctionSpecifiers>>();
    ProcessFeaturesKind(AllDefaultKey, all_default_features);
    auto explicit_default_features = features.at(ExplicitDefaultKey).get<std::vector<MKFunctionSpecifiers>>();
    ProcessFeaturesKind(ExplicitDefaultKey, explicit_default_features);

    auto delete_features = features.at(DeleteKey).get<std::vector<MKFunctionSpecifiers>>();
    ProcessFeaturesKind(DeleteKey, delete_features);

    auto final_features = features.at(FinalKey).get<std::vector<MKFinal>>();
    ProcessFeaturesKind(FinalKey, final_features);

    auto inline_namespace_features = features.at(InlineNamespaceKey).get<std::vector<MKInfo>>();
    ProcessFeaturesKind(InlineNamespaceKey, inline_namespace_features);
    
    auto all_noexcept_features = features.at(AllNoexceptKey).get<std::vector<MKNoexcept>>();
    ProcessFeaturesKind(AllNoexceptKey, all_noexcept_features);
    auto explicit_noexcept_features = features.at(ExplicitNoexceptKey).get<std::vector<MKNoexcept>>();
    ProcessFeaturesKind(ExplicitNoexceptKey, explicit_noexcept_features);

    auto nullptr_features = features.at(NullptrKey).get<std::vector<MKInfo>>();
    ProcessFeaturesKind(NullptrKey, nullptr_features);

    auto thread_local_features = features.at(ThreadLocalKey).get<std::vector<MKThreadLocal>>();
    ProcessFeaturesKind(ThreadLocalKey, thread_local_features);

    auto override_features = features.at(OverrideKey).get<std::vector<MKOverride>>();
    ProcessFeaturesKind(OverrideKey, override_features);

    auto static_assert_features = features.at(StaticAssertKey).get<std::vector<MKInfo>>();
    ProcessFeaturesKind(StaticAssertKey, static_assert_features);

    // extern template: count occurences
}

//-----------------------------------------------------------------------------
