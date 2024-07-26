#ifndef TEMPLATEINSTANTIATIONANALYSIS_H
#define TEMPLATEINSTANTIATIONANALYSIS_H

#include <array>
#include <string>
#include <string_view>
#include <unordered_map>

#include "cxx-langstat/Analysis.h"

#include "nlohmann/json.hpp"

//-----------------------------------------------------------------------------

class TemplateInstantiationAnalysis : public Analysis {
public:
    enum class InstKind {
        Class, Function, Variable, Any
    };

    // Ctor to let TIA look for any kind of instantiations
    TemplateInstantiationAnalysis();
    // Ctor to instrument TIA to look only for instantiations of kind IK
    // named any of "Names".
    TemplateInstantiationAnalysis(InstKind IK,
        clang::ast_matchers::internal::Matcher<clang::NamedDecl> Names,
        std::string HeaderRegex);
    ~TemplateInstantiationAnalysis(){
        std::cout << "TIA dtor\n";
    }
    std::string_view getShorthand() override {
        return ShorthandName;
    }
    static std::unordered_map<std::string, std::unordered_map<nlohmann::json, int>>* getDeduplicatingMap() {
        if (DeduplicatingMap == nullptr)
            DeduplicatingMap = new std::unordered_map<std::string, std::unordered_map<nlohmann::json, int>>();
        return DeduplicatingMap;
    }

    static void cleanDeduplicatingMap() {
        if (DeduplicatingMap != nullptr) {
            delete DeduplicatingMap;
            DeduplicatingMap = nullptr;
        }
    }

protected:
    struct MatchInfo {
        MatchInfo(unsigned Location = 0, std::shared_ptr<std::string> GlobalLocation = nullptr)
            : Location(Location), GlobalLocation(GlobalLocation) {}
        MatchInfo(unsigned Location = 0, std::string GlobalLocation = "")
            : Location(Location), GlobalLocation(std::make_shared<std::string>(GlobalLocation)) {}
        unsigned Location;
        std::shared_ptr<std::string> GlobalLocation;
    };
private:
    static std::unordered_map<std::string, std::unordered_map<nlohmann::json, int>>* DeduplicatingMap;

    InstKind IK;
    clang::ast_matchers::internal::Matcher<clang::NamedDecl> Names;
    
    // CTSDs that appeared in explicit instantiation.
    // Matches<clang::ClassTemplateSpecializationDecl> TemplatedClassUses;
    Matches<clang::DeclaratorDecl> TemplatedClassUses;
    Matches<clang::Expr> TemplatedClassUses_expr;
    // CTSDs that appeared in implicit instantiation or where used by a variable
    // that has CTSD type.
    Matches<clang::ClassTemplateSpecializationDecl> ClassExplicitInsts;
    // Declarations that declare a variable of type CTSD from the list above.
    Matches<clang::DeclaratorDecl> Variables;

    // Implicit function instantiations caused by a call expression
    Matches<clang::FunctionDecl> ImplFuncInsts;
    // Explicit function instantiations
    Matches<clang::FunctionDecl> ExplFuncInsts;
    
    // Call exprs that cause an implicit function instantiation or refer to it.
    // Matches<clang::CallExpr> Callers;
    Matches<clang::DeclRefExpr> Callers;
    // DeclRefExprs that refer to a function template, causing an explicit instantiation
    std::vector<MatchInfo> Instantiators;
    //
    Matches<clang::VarTemplateSpecializationDecl> VarInsts;
    // Responsible to fill vectors of matches defined above
    void extractFeatures();

    
    // Get location of instantiation
    template<typename T>
    std::pair<unsigned, std::shared_ptr<std::string> > 
        getInstantiationLocation(const Match<T>& Match, bool isImplicit);

    // Get the location of the definition
    // Get location of instantiation
    template<typename T>
    std::string getDefinitionLocation(const Match<T>& Match);

    // Specializations to get locations of class and func template uses.
    // template<>
    // std::pair<unsigned, std::shared_ptr<std::string> > 
    //     getInstantiationLocation
    //     (const Match<clang::ClassTemplateSpecializationDecl>& Match,
    //         bool isImplicit);
    // template<>
    // std::pair<unsigned, std::shared_ptr<std::string> >
    //     getInstantiationLocation(const Match<clang::FunctionDecl>& Match,
    //         bool isImplicit);
    // Given matches representing the instantiations of some kind, gather
    // for each instantiation the instantiation arguments.
    template<typename T>
    void gatherInstantiationData(Matches<T>& Insts, const std::string& InstKind,
        bool AreImplicit);
    //
    unsigned VariablesCounter = 0;
    unsigned CallersCounter = 0;
    unsigned InstantiatorsCounter = 0;
    // Regex specifying the header that the template comes from, i.e. usually
    // where it is *defined*. This is important s.t. when the analyzer looks for
    // instantiations of some template, it is ensured that the instantiation is
    // not a false positive of some other template having the same name. Note
    // that when analyzing C++ library templates you might have to dig
    // deeper to find the file containing the definition of the template, which
    // can be an internal header file you usually don't include yourself.
    std::string HeaderRegex;
protected:
    // Responsible to fill vectors of matches
    void analyzeFeatures() override;
    void processFeatures(const nlohmann::ordered_json& j) override;
    
    // JSON keys
    static constexpr auto ExplicitClassKey = "explicit class insts";
    static constexpr auto ImplicitClassKey = "templated class uses";
    static constexpr auto ExplFuncKey = "explicit func insts";
    static constexpr auto ImplFuncKey = "templated func uses";
    static constexpr auto VarKey = "var insts";
    //
    const std::string ShorthandName = "tia";
public:
    static constexpr std::array<decltype(ExplicitClassKey), 5> getFeatureKeys() {
        return {ExplicitClassKey, ImplicitClassKey, ExplFuncKey, ImplFuncKey, VarKey};
    }
};

template<typename T>
using StringMap = std::map<std::string, T>;

// For "Type", gets how many type arguments of the instantiation we want to know.
int getNumRelevantTypes(llvm::StringRef Type, const StringMap<int>& SM);

// For "Type", gets from "Types" all relevant types of the
// instantantiation.
std::string getRelevantTypesAsString(llvm::StringRef Type,
    nlohmann::json Types, const StringMap<int>& SM);

// For each template in "in", computes how often it was used in some "implicit"
// instantiation.
void templatePrevalence(const nlohmann::ordered_json& in,
    nlohmann::ordered_json& out);

// For each set of template type arguments used to create any instantiation in
// "in", computes how often that set occured in total among all instantiations
// in "in".
void templateTypeArgPrevalence(const nlohmann::ordered_json& in,
    nlohmann::ordered_json& out, const StringMap<StringMap<int>>& SM);

//-----------------------------------------------------------------------------

#endif // TEMPLATEINSTANTIATIONANALYSIS_H
