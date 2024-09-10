#include "cxx-langstat/Utils.h"

#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

#include <filesystem>
#include <string>
#include <regex>
#include <optional>

std::string normalizeGlobalLocation(std::string&& GlobalLocation){
    std::string::size_type idx = GlobalLocation.find(" <Spelling=");
    if(idx != std::string::npos){
        std::string path = GlobalLocation.substr(0, idx);
        std::string spelling_path = GlobalLocation.substr(idx + strlen(" <Spelling="), GlobalLocation.find(">", idx) - idx - strlen(" <Spelling="));

        GlobalLocation = std::filesystem::weakly_canonical(path).string() + " <Spelling=" + std::filesystem::weakly_canonical(spelling_path).string() + ">";
    } else {
        GlobalLocation = std::filesystem::weakly_canonical(GlobalLocation).string();
    }
    return GlobalLocation;
}

std::string normalizeGlobalLocation(const std::string& GlobalLocation){
    std::string Result;
    std::string::size_type idx = GlobalLocation.find(" <Spelling=");
    if(idx != std::string::npos){
        std::string path = GlobalLocation.substr(0, idx);
        std::string spelling_path = GlobalLocation.substr(idx + strlen(" <Spelling="), GlobalLocation.find(">", idx) - idx - strlen(" <Spelling="));
        Result = std::filesystem::weakly_canonical(path).string() + " <Spelling=" + std::filesystem::weakly_canonical(spelling_path).string() + ">";
    } else {
        Result = std::filesystem::weakly_canonical(GlobalLocation).string();
    }
    return Result;
}

bool isSourceLocationInHomeDirectory(const clang::SourceLocation& Loc,
    const clang::SourceManager& SM){
    if (Loc.isInvalid())
        return false;
    if (SM.isInSystemHeader(Loc))
        return false;
    auto FileEntry = SM.getFileEntryRefForID(SM.getFileID(Loc));
    if (!FileEntry) {
        return false;
    }
    
    auto Filename = FileEntry->getName();
    // normalize the filename
    std::string CanonicalFilename = std::filesystem::weakly_canonical(Filename.str());
    return std::regex_search(CanonicalFilename, k_UserFilePrefixRegex);
}

bool isSourceLocationInFileMatching(const clang::SourceLocation& Loc,
    const clang::SourceManager& SM, const std::string& FileRegex){
    if (Loc.isInvalid())
        return false;
    auto FileEntry = SM.getFileEntryRefForID(SM.getFileID(Loc));
    if (!FileEntry) {
        return false;
    }
    
    auto Filename = FileEntry->getName();
    // normalize the filename
    std::string CanonicalFilename = std::filesystem::weakly_canonical(Filename.str());
    return std::regex_search(CanonicalFilename, std::regex(FileRegex));

}

clang::ast_matchers::TypeMatcher recursiveTypeMatcher_internal(clang::ast_matchers::TypeMatcher type_matcher, int levels, bool first_call = false){
    using namespace clang;
    using namespace clang::ast_matchers;

    if (levels == 0) {
        return type_matcher;
    }

    if (first_call){
        return anyOf(
            type_matcher,
            references(type_matcher),
            references(recursiveTypeMatcher_internal(type_matcher, levels - 1, false)),
            pointsTo(type_matcher),
            pointsTo(recursiveTypeMatcher_internal(type_matcher, levels - 1, false))
        );
    }
    else {
        return anyOf(
            type_matcher,
            pointsTo(type_matcher),
            pointsTo(recursiveTypeMatcher_internal(type_matcher, levels - 1, false))
        );
    }
}

clang::ast_matchers::TypeMatcher recursiveTypeMatcher(clang::ast_matchers::TypeMatcher type_matcher, int levels){
    return recursiveTypeMatcher_internal(type_matcher, levels, true);
}

clang::QualType removePointersAndReference(const clang::QualType& QT){
    using namespace clang;
    using namespace clang::ast_matchers;

    const Type* type = QT.getTypePtr();

    if (type->isPointerType())
        return removePointersAndReference(type->getPointeeType());
    else if (type->isReferenceType())
        return removePointersAndReference(type->getPointeeType());
    return QT;
}

std::optional<int> getIntValueOfExpr(const clang::Expr* Expression, const clang::ASTContext& Context){
    using namespace clang;
    using namespace clang::ast_matchers;

    Expr::EvalResult EvalResult;
    if (Expression && !Expression->isValueDependent() && Expression->EvaluateAsInt(EvalResult, Context)){
        return *EvalResult.Val.getInt().getRawData();
    }

    return std::nullopt;
}
