#include <iostream>
#include <vector>

#include "cxx-langstat/Analysis.h"
#include "cxx-langstat/Analyses/UsingAnalysis.h"
#include "cxx-langstat/Utils.h"

using namespace clang;
using namespace clang::ast_matchers;
using ordered_json = nlohmann::ordered_json;
using json = nlohmann::json;
using SynonymInfo = UsingAnalysis::SynonymInfo;

//-----------------------------------------------------------------------------
// Question: Did programmers abandon typedef in favor of aliases (e.g.
// using newType = oldType<int> )?
// What's the reason for that development?
// Is it because alias templates are easier to use than "typedef templates?"?
// Could also be because "typedef templates" require typename or ::type to be
// used.

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SynonymInfo, Location, GlobalLocation);

void UsingAnalysis::extractFeatures() {
    // Synonyms.clear();

    // Type aliases
    // however, only those that are not part of type alias
    // templates. Type alias template contains type alias node in clang AST.
    auto typeAlias = typeAliasDecl(
        // isExpansionInMainFile(),
        isExpansionInHomeDirectory(),
        unless(isExpansionInSystemHeader()),
        unless(hasParent(typeAliasTemplateDecl())))
    .bind("alias");

    // Alias template
    auto typeAliasTemplate = typeAliasTemplateDecl(
        // isExpansionInMainFile())
        isExpansionInHomeDirectory(),
        unless(isExpansionInSystemHeader()))
    .bind("aliastemplate");

    // "Typedef template"
    // Of course there are no typedef templates in C++, but we consider the
    // following idiom mostly used prior to C++11 to be a "typedef template":
    // template<typename T>
    // struct Pair {
    //     typedef Tuple<T> type; // don't have to name this 'type'
    // };
    // We thus say that a class template is a "typedef decl" if it contains only
    // typedefs, nothing else.
    // No requirement about the access specifier of the typedef.
    auto typedefTemplate = classTemplateDecl(has(cxxRecordDecl(
        // isExpansionInMainFile(),
        isExpansionInHomeDirectory(),
        unless(isExpansionInSystemHeader()),
        // Must have typedefDecl
        // has() will ensure only first typedef is matched. if want to allow
        // for multiple typedefs in a template, we have to use forEach()
        forEach(typedefDecl().bind("td")),
        // Must not have decl that is not typedef, access specifier or cxxrecord
        unless(has(decl(
            unless(anyOf(
                typedefDecl(),
                accessSpecDecl(),
                // Must be allowed to contain implicit cxxrecord,
                // instrinsic to clang AST. Same as with VTA.
                cxxRecordDecl(isImplicit())))))))))
    .bind("typedeftemplate");

    // Typedef
    // Count all typedefs that are explicitly written by programmer (high level)
    // Concretely, that means:
    // We want typedefs matched here not to be part of a "typedef template",
    // because we want to study those separately. To do that, have 2 requirements:
    // 1) a typedef matched here may not be part of a class template that is
    // used to create a "typedef template"
    // 2) a typedef matched here may not be part of a class template specialization
    // that occurs when the programmer wants to make use of the "typedef template"
    auto typedef_ = typedefDecl(
        // isExpansionInMainFile(),
        isExpansionInHomeDirectory(),
        unless(isExpansionInSystemHeader()),
        // 2) Don't match typedef in class specializations coming from "typedef templates"
        // 1) will filtered out later
        unless(hasParent(classTemplateSpecializationDecl(
            hasSpecializedTemplate(typedefTemplate)))))
    .bind("typedef");

    // Note: Left works only in clang-query, right works in both CQ and LibTooling
    // has(anyOf(...)) -> has(decl(anyOf(...)))
    // has(unless(...)) -> has(decl(unless(...)))

    Matches<clang::Decl> TypedefDecls = Extractor.extract(*Context, "typedef", typedef_);
    Matches<clang::Decl> TypeAliasDecls = Extractor.extract(*Context, "alias", typeAlias);
    auto typedeftemplateresults = Extractor.extract2(*Context, typedefTemplate);
    Matches<clang::Decl> TypedefTemplateDecls = getASTNodes<Decl>(typedeftemplateresults, "typedeftemplate");
    Matches<clang::Decl> td = getASTNodes<Decl>(typedeftemplateresults, "td");
    Matches<clang::Decl> TypeAliasTemplateDecls = Extractor.extract(*Context, "aliastemplate",
        typeAliasTemplate);

    // Filters as described in 1) in typedef
    // need to do extra work to remove from typedefdecls those decls that occur
    // in typedeftemplatedecls (to get distinction between typedef and typedef
    // templates)
    for(auto& decl : td){
        for(unsigned i=0; i<TypedefDecls.size(); i++){
            auto& d = TypedefDecls[i];
            if (decl == d)
                TypedefDecls.erase(TypedefDecls.begin()+i);
        }
    }
    // Possible improvement: for each typedef/alias, state what type was aliased
    for(auto& m : TypedefDecls)
        Typedefs.emplace_back(m.Location, m.GlobalLocation);
    for(auto& m : TypeAliasDecls)
        Usings.emplace_back(m.Location, m.GlobalLocation);
    // possible improvement: instead of stating a "typedef template" multiple
    // times when it contains multiple typdefs, state it a single time, but
    // have it have a number showing #typedefs it contains
    for(auto& m : TypedefTemplateDecls)
        TemplatedTypedefs.emplace_back(m.Location, m.GlobalLocation);
    for(auto& m : TypeAliasTemplateDecls)
        TemplatedUsings.emplace_back(m.Location, m.GlobalLocation);
}

//-----------------------------------------------------------------------------
// // Helper functions to convert SynonymKind to string and back.
// std::string SKToString(SynonymKind Kind){
//     switch (Kind) {
//         case Typedef:
//             return "typedef";
//         case Using:
//             return "using";
//         default:
//             return "unknown";
//     }
// }
// SynonymKind getSKFromString(llvm::StringRef s){
//     if (s.equals("typedef"))
//         return SynonymKind::Typedef;
//     else if (s.equals("using"))
//         return SynonymKind::Using;
//     else
//         return SynonymKind::Unknown;
// }
// // Functions to convert structs to/from JSON.
// void to_json(nlohmann::json& j, const Synonym& s){
//     j = nlohmann::json{
//         {"Location", s.Location},
//         {"GlobalLocation", *s.GlobalLocation},
//         // {"Kind", SKToString(s.Kind)},
//         // {"isTemplated", s.isTemplated}
//     };
// }
// void from_json(const nlohmann::json& j, Synonym& s){
//     j.at("Location").get_to(s.Location);
//     s.GlobalLocation = std::make_shared<GlobalLocation>(j.at("GlobalLocation").get<std::string>());
//     // s.Kind = getSKFromString(j.at("kind").get<std::string>());
//     // j.at("isTemplated").get_to(s.isTemplated);
// }

//-----------------------------------------------------------------------------
//

template<typename T>
void UsingAnalysis::featuresToJSON(const std::string& Kind, const std::vector<T>& features){
    Features[Kind] = json::array();
    for(auto feature : features){
        json feature_json = feature;
        Features[Kind].emplace_back(feature_json);
    }
}

void UsingAnalysis::analyzeFeatures(){
    extractFeatures();
    featuresToJSON(TypedefKey, Typedefs);
    featuresToJSON(UsingKey, Usings);
    featuresToJSON(TemplatedTypedefKey, TemplatedTypedefs);
    featuresToJSON(TemplatedUsingKey, TemplatedUsings);
    // for(auto s : Synonyms){
    //     nlohmann::json s_j = s;
    //     Features.emplace_back(s_j);
    // }

}

// Computes prevalences of typedefs and aliases (and templates thereof).
void SynonymPrevalence(ordered_json& Stats, ordered_json j){
    // unsigned Typedefs=0, Aliases=0, TypedefTemplates=0, AliasTemplates = 0;
    // for(const auto& s_j : j){
    //     // std::cout << s_j.dump(4) << std::endl;
    //     Synonym s;
    //     from_json(s_j, s);
    //     if(s.Kind == Typedef){
    //         if(s.Templated)
    //             TypedefTemplates++;
    //         else
    //             Typedefs++;
    //     } else if(s.Kind == Alias){
    //         if(s.Templated)
    //             AliasTemplates++;
    //         else
    //             Aliases++;
    //     }
    // }
    // Stats[AliasKindPrevalenceKey]["typedef"] = Typedefs;
    // Stats[AliasKindPrevalenceKey]["alias"] = Aliases;
    // Stats[AliasKindPrevalenceKey]["typedef template"] = TypedefTemplates;
    // Stats[AliasKindPrevalenceKey]["alias templates"] = AliasTemplates;
    // Stats[AliasKindPrevalenceKey][TypedefKey] = Typedefs.size();
    // Stats[AliasKindPrevalenceKey][UsingKey] = Usings.size();
    // Stats[AliasKindPrevalenceKey][TemplatedTypedefKey] = TemplatedTypedefs.size();
    // Stats[AliasKindPrevalenceKey][TemplatedUsingKey] = TemplatedUsings.size();
}

void UsingAnalysis::processFeatures(const nlohmann::ordered_json& features){
    // SynonymPrevalence(Statistics, j);
    Statistics[AliasKindPrevalenceKey][TypedefKey] = features[TypedefKey].size();
    Statistics[AliasKindPrevalenceKey][UsingKey] = features[UsingKey].size();
    Statistics[AliasKindPrevalenceKey][TemplatedTypedefKey] = features[TemplatedTypedefKey].size();
    Statistics[AliasKindPrevalenceKey][TemplatedUsingKey] = features[TemplatedUsingKey].size();
}

//-----------------------------------------------------------------------------
