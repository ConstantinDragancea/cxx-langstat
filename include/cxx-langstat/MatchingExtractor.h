#ifndef MATCHINGEXTRACTOR_H
#define MATCHINGEXTRACTOR_H

//-----------------------------------------------------------------------------
// Match object to contain data extracted from AST
// contains location, pointer to object representing stmt, decl, or type in AST
// and the ASTContext
template<typename T>
struct Match {
    Match<T>(unsigned location, const T* node, clang::ASTContext* ctxt);
    unsigned location;
    const T* node;
    clang::ASTContext* ctxt;
    bool operator==(Match<T> m){
        return (location == m.location && node == m.node && ctxt == m.ctxt);
    }
};

template<typename T>
using Matches = std::vector<Match<T>>; // allows to do Matches<T>

//-----------------------------------------------------------------------------
// Callback class executed on match
template<typename T>
class MatchingExtractor : public clang::ast_matchers::MatchFinder::MatchCallback {
public:
    MatchingExtractor(std::string id);
    // Run when match is found after extract call with Matcher
    virtual void run(const clang::ast_matchers::MatchFinder::MatchResult &Result);
    Matches<T> matches;
private:
    void resetState();
    std::string matcherid;
};

//-----------------------------------------------------------------------------

#endif // MATCHINGEXTRACTOR_H
