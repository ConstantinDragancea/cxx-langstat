#ifndef MATCHINGEXTRACTOR_H
#define MATCHINGEXTRACTOR_H

#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/AST/PrettyPrinter.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/raw_os_ostream.h"

#include <iostream>
#include <string>
#include <utility>

// forward declaration for Utils function
clang::QualType removePointersAndReference(const clang::QualType& QT);

namespace MatchingExtractorUtil {

template <typename T>
std::enable_if_t<std::is_base_of_v<clang::NamedDecl, T>, std::string>
getNamedDeclTypeName(const T* Decl, const clang::PrintingPolicy& Policy) {
    std::string Result;
    llvm::raw_string_ostream stream(Result);
    Decl->printQualifiedName(stream, Policy);
    return Result;
}

template <typename T>
std::enable_if_t<std::is_base_of_v<clang::ValueDecl, T>, std::string>
getValueDeclTypeName(const T* Node, const clang::PrintingPolicy& Policy) {
    const clang::Type* type_pointer = Node->getType().getTypePtr();
    if (type_pointer->isPointerType() || type_pointer->isReferenceType()){
        clang::QualType qual_type = Node->getType().getTypePtr()->getPointeeType();
        qual_type = removePointersAndReference(qual_type);
        type_pointer = qual_type.getTypePtr();
    }
    
    if (const auto* as_decl = type_pointer->getAsRecordDecl())
        return getNamedDeclTypeName(as_decl, Policy);
    else
        return Node->getType().getAsString(Policy);
}

template <typename T>
std::enable_if_t<std::is_base_of_v<clang::FunctionDecl, T>, std::string>
getFunctionDeclReturnTypeName(const T* Node, const clang::PrintingPolicy& Policy) {
    const clang::Type* type_pointer = Node->getReturnType().getTypePtr();
    if (type_pointer->isPointerType() || type_pointer->isReferenceType()){
        clang::QualType qual_type = type_pointer->getPointeeType();
        qual_type = removePointersAndReference(qual_type);
        type_pointer = qual_type.getTypePtr();
    }
    
    if (const auto* as_decl = type_pointer->getAsRecordDecl())
        return getNamedDeclTypeName(as_decl, Policy);
    else
        return Node->getType().getAsString(Policy);
}

template <typename T>
std::string getDeclTypeName(const T* Node, const clang::PrintingPolicy& Policy){
    if (const clang::FunctionDecl* node_as_func_decl = llvm::dyn_cast<clang::FunctionDecl>(Node)){
        return MatchingExtractorUtil::getFunctionDeclReturnTypeName(node_as_func_decl, Policy);
    }
    else if (const clang::ValueDecl* node_as_val_decl = llvm::dyn_cast<clang::ValueDecl>(Node)){
        return MatchingExtractorUtil::getValueDeclTypeName(node_as_val_decl, Policy);
    }
    else if(const clang::NamedDecl* node_as_decl = llvm::dyn_cast<clang::NamedDecl>(Node)){
        return MatchingExtractorUtil::getNamedDeclTypeName(node_as_decl, Policy);
    } else {
        assert(false);
    }
}

template <typename T>
std::string getExprReturnTypeName(const T* Node, const clang::PrintingPolicy& Policy){
    const clang::Type* type_pointer = Node->getType().getTypePtr();
    if (type_pointer->isPointerType() || type_pointer->isReferenceType()){
        clang::QualType qual_type = Node->getType().getTypePtr()->getPointeeType();
        type_pointer = qual_type.getTypePtr();
    }
    
    if (const auto* as_decl = type_pointer->getAsRecordDecl())
        return getNamedDeclTypeName(as_decl, Policy);
    else
        return Node->getType().getAsString(Policy);
}

template <typename T>
std::string getExprReturnCleanTypeName(const T* Node, const clang::PrintingPolicy& Policy){
    const clang::Type* type_pointer = Node->getType().getTypePtr();
    if (type_pointer->isPointerType() || type_pointer->isReferenceType()){
        clang::QualType qual_type = Node->getType().getTypePtr()->getPointeeType();
        qual_type = removePointersAndReference(qual_type);
        type_pointer = qual_type.getTypePtr();
    }
    
    if (const auto* as_decl = type_pointer->getAsRecordDecl())
        return getNamedDeclTypeName(as_decl, Policy);
    else
        return Node->getType().getAsString(Policy);
}

template <typename T>
std::string getTypeName(const T* Node, const clang::PrintingPolicy& Policy){
    if constexpr (std::is_base_of_v<clang::Expr, T>)
        return getExprReturnTypeName(Node, Policy);
    else if constexpr (std::is_base_of_v<clang::Decl, T>)
        return getDeclTypeName(Node, Policy);
    else
        static_assert(false, "Match<T>::getName not implemented for this type");
}

template <typename T>
std::string getCleanTypeName(const T* Node, const clang::PrintingPolicy& Policy){
    if constexpr (std::is_base_of_v<clang::Expr, T>){
        return getExprReturnCleanTypeName(Node, Policy);
    }
    else if constexpr (std::is_base_of_v<clang::Decl, T>) {
        return getDeclTypeName(Node, Policy);
    }
    else
        static_assert(false, "Match<T>::getName not implemented for this type");
}

} // MatchingExtractorUtil namespace


// Helper class to abstract MatchCallback away to function that analyses can
// call to extract MatchResults. Analyses should then apply getASTNodes
// with the bound id to get the actual nodes in the AST.
// In the future, might be more handy to implement MatchCallback directly inside
// of the concrete analyses instead of hiding it after the BaseExtractor interface.

//-----------------------------------------------------------------------------
// Match object to contain data extracted from AST
// contains location, pointer to object representing stmt, decl, or type in AST
// and the ASTContext
template<typename T>
struct Match {
    Match<T>(unsigned Location, std::shared_ptr<std::string> GlobalLocation, const T* Node) :
        Location(Location),
        GlobalLocation(std::move(GlobalLocation)),
        Node(Node){
    }
    Match<T>(unsigned Location, const std::string& GlobalLocation, const T* Node) :
        Location(Location),
        GlobalLocation(std::make_shared<std::string>(GlobalLocation)),
        Node(Node){
    }
    unsigned Location;
    std::shared_ptr<std::string> GlobalLocation;
    const T* Node;
    bool operator==(const Match<T>& m) const {
        return (Location == m.Location && *GlobalLocation == *m.GlobalLocation && Node == m.Node);
    }
    // https://stackoverflow.com/questions/2758080/how-to-sort-an-stl-vector
    bool operator<(const Match<T>& other) const{
        return (Location < other.Location ||
            (Location == other.Location && *GlobalLocation < *other.GlobalLocation) ||
            (Location == other.Location && *GlobalLocation == *other.GlobalLocation && Node < other.Node));
    }
    // If T is of type clang::Decl, and can be cast to clang::NamedDecl
    // or if T is Expr whose return type is a templated class
    std::string getTypeName(const clang::PrintingPolicy& Policy) const{
        return MatchingExtractorUtil::getTypeName(this->Node, Policy);
    }

    std::string getCleanTypeName(const clang::PrintingPolicy& Policy) const {
        return MatchingExtractorUtil::getCleanTypeName(this->Node, Policy);
    }
    std::string getCleanTypeName() const {
        clang::LangOptions LO;
        clang::PrintingPolicy PP(LO);
        PP.PrintCanonicalTypes = true;
        PP.SuppressTagKeyword = true;
        PP.SuppressScope = false;
        PP.SuppressUnwrittenScope = true;
        PP.FullyQualifiedName = true;
        PP.Bool = true;
        PP.SuppressTemplateArgsInCXXConstructors = true;
        return getCleanTypeName(PP);
    }
    // std::string getDeclTypeName(const clang::PrintingPolicy& Policy) const;
    // std::string getExprReturnTypeDeclName(const clang::PrintingPolicy& Policy) const;

    std::string getIdentifierName() const {
        if (const clang::NamedDecl* decl = llvm::dyn_cast<clang::NamedDecl>(Node)) {
            return decl->getQualifiedNameAsString();
        // } else if (const clang::Expr* expr = llvm::dyn_cast<clang::Expr>(Node)) {
        //     return expr->getStmtClassName();
        } else {
            assert(false && "Node is not a NamedDecl");
        }
    }

};


template<typename T>
using Matches = std::vector<Match<T>>; // allows to do Matches<T>

//-----------------------------------------------------------------------------
// Callback class executed on match
class MatchingExtractor : public clang::ast_matchers::MatchFinder::MatchCallback {
public:
    // Run when match is found after extract call with Matcher
    virtual void run(const clang::ast_matchers::MatchFinder::MatchResult& Result);
    std::vector<clang::ast_matchers::MatchFinder::MatchResult> Results;
};

//-----------------------------------------------------------------------------

#endif // MATCHINGEXTRACTOR_H
