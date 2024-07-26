#ifndef UTILS_H
#define UTILS_H

#include "cxx-langstat/MatchingExtractor.h"

#include "llvm/Support/Casting.h"
// #include "llvm/ADT/StringRef.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

#include "nlohmann/json.hpp"

#include <iostream>
#include <regex>
#include <string>
#include <optional>


//-----------------------------------------------------------------------------
// Given a clang GlobalLocation, return a normalized path
// if GlobalLocation is "/a/../b" then return "/b"
// if GlobalLocation is "/a/../b <Spelling=/c/../d>" then return "/b <Spelling=/d>"
std::string normalizeGlobalLocation(std::string&& GlobalLocation);
std::string normalizeGlobalLocation(const std::string& GlobalLocation);

static constexpr auto k_UserFilePrefix = "/home/.*";
static const auto k_UserFilePrefixRegex = std::regex(k_UserFilePrefix);

bool isSourceLocationInHomeDirectory(const clang::SourceLocation& Loc,
    const clang::SourceManager& SM);
bool isSourceLocationInFileMatching(const clang::SourceLocation& Loc,
    const clang::SourceManager& SM, const std::string& FileRegex);

clang::ast_matchers::TypeMatcher recursiveTypeMatcher(clang::ast_matchers::TypeMatcher type_matcher,
    int levels = 5);

clang::QualType removePointersAndReference(const clang::QualType& QT);

std::optional<int> getIntValueOfExpr(const clang::Expr* Expression, const clang::ASTContext& Context);

//-----------------------------------------------------------------------------
// For a match whose node is a decl that can be a nameddecl, return its
// human-readable name.
template<typename T>
std::string getMatchDeclName(const Match<T>& Match){
    if(auto n = llvm::dyn_cast<clang::NamedDecl>(Match.Node)){
        return n->getQualifiedNameAsString(); // includes namespace qualifiers
    } else {
        std::cout << "Decl @ " << Match.Location << "cannot be resolved\n";
        return "INVALID";
    }
}
// For a list containing matches that are derived from NamedDecl, print them.
template<typename T>
void printMatches(const std::string& text, const Matches<T>& Matches){
    std::cout << "\033[33m" << text << ":\033[0m " << Matches.size() << "\n";
    for(auto match : Matches)
        std::cout << getMatchDeclName(match) << " @ " << match.Location << "\n";
}

template<typename T>
std::enable_if_t<std::is_base_of_v<clang::FunctionDecl, T>, std::string>
getFunctionHeader(const T* Node){
    // Extract and print the return type
    // QualType ReturnType = MethodDecl->getReturnType();
    // std::string ReturnTypeStr = ReturnType.getAsString();
    
    // Extract and print the method name
    std::string MethodName = Node->getNameAsString();
    
    // Extract and print the parameter types
    std::string Params;
    for (const auto *Param : Node->parameters()) {
        if (!Params.empty()) {
            Params += ", ";
        }
        Params += Param->getType().getAsString();
    }
    
    // Construct and print the function header
    std::string FunctionHeader = MethodName + "(" + Params + ")";
    return FunctionHeader;
}

//-----------------------------------------------------------------------------
// Given a vector of Matchresults and an id, extract the actual nodes in the
// Clang AST by returning pointers to them.
// We thus assume that the ASTContext holding those nodes has a longer lifetime
// than an analysis using them.
template<typename NodeType>
Matches<NodeType>
getASTNodes(std::vector<clang::ast_matchers::MatchFinder::MatchResult> Results,
    std::string id){
    Matches<NodeType> Matches;
    unsigned TimesidNotBound = 0;
    for(auto Result : Results){
        auto Mapping = Result.Nodes.getMap();
        if(!Mapping.count(id))
            TimesidNotBound++;
    }
    if(Results.size()==TimesidNotBound){
        // std::cout << "No nodes were bound to given id \"" << id << "\"\n";
        return Matches;
    }
    // We now know that id must be bound to some node in some result in the
    // Results vector, so below in can only fail if it cannot be gotten as
    // the desired type.
    for(auto Result : Results){
        if(const NodeType* Node = Result.Nodes.getNodeAs<NodeType>(id)) {
            if (Node->getBeginLoc().isInvalid()){
                continue;
            }
            unsigned Location = Result.Context->getFullLoc(
                Node->getBeginLoc()).getLineNumber();
            // 
            std::string GlobalLocation = Result.Context->getFullLoc(
                Node->getBeginLoc()).printToString(Result.Context->getSourceManager());
            GlobalLocation = normalizeGlobalLocation(std::move(GlobalLocation));

            // Match<NodeType> m(Location, GlobalLocation, Node);
            Matches.emplace_back(Location, GlobalLocation, Node);
        }
    }
    if(!Matches.size()){
        // std::cout << "Cannot get node as type for id \"" << id << "\"\n";
    }
    return Matches;
}

//-----------------------------------------------------------------------------

template<typename T>
void removeDuplicateMatches(Matches<T>& Matches){
    if(!std::is_sorted(Matches.begin(), Matches.end())){
        std::sort(Matches.begin(), Matches.end());
    }
    unsigned s = Matches.size();
    for(unsigned idx=0; idx<s-1; idx++){
        if(Matches[idx]==Matches[idx+1]){
            Matches.erase(Matches.begin()+idx+1);
            idx--;
            s--;
        }
    }
}

//-----------------------------------------------------------------------------

namespace nlohmann
{
template <typename T>
struct adl_serializer<std::shared_ptr<T>>
{
    static void to_json(nlohmann::json& j, const std::shared_ptr<T>& opt)
    {
        if (opt)
        {
            j = *opt;
        }
        else
        {
            j = nullptr;
        }
    }

    static void from_json(const nlohmann::json& j, std::shared_ptr<T>& opt)
    {
        if (j.is_null())
        {
            opt = nullptr;
        }
        else
        {
            opt.reset(new T(j.get<T>()));
        }
    }
};
}

//-----------------------------------------------------------------------------

inline auto 
  isExpansionInHomeDirectory() {
    return clang::ast_matchers::isExpansionInFileMatching(k_UserFilePrefix);
}

//-----------------------------------------------------------------------------

#endif // UTILS_H
