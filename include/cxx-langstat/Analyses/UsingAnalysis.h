#ifndef USINGANALYSIS_H
#define USINGANALYSIS_H

#include "cxx-langstat/Analysis.h"

//-----------------------------------------------------------------------------

// need analysis object since we want other analysis to inherit interface
class UsingAnalysis : public Analysis {
public:
    UsingAnalysis()=default;
    void extract();
    void gatherStatistics();
    void run(llvm::StringRef InFile, clang::ASTContext& Context) override;
private:
    Matches<clang::Decl> TypedefDecls;
    Matches<clang::Decl> TypeAliasDecls;
    Matches<clang::Decl> TypeAliasTemplateDecls;
    Matches<clang::Decl> TypedefTemplateDecls;
    Matches<clang::Decl> td;

    nlohmann::ordered_json Result;
};

//-----------------------------------------------------------------------------

#endif // USINGANALYSIS_H
