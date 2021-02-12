#include "cxx-langstat/Analyses/MoveSemanticsAnalysis.h"

using namespace clang::ast_matchers;
using ojson = nlohmann::ordered_json;

//-----------------------------------------------------------------------------

namespace msa {

// Functions to convert structs to/from JSON.
void to_json(nlohmann::json& j, const FunctionParamInfo& o){
    j = nlohmann::json{
        {"Func Identifier", o.FuncId},
        {"Func Type", o.FuncType},
        {"Func Location", o.FuncLocation},
        {"Identifier", o.Id},
        {"construction kind", toString.at(o.CK)},
        {"copy/move ctor is compiler-generated", o.CompilerGenerated}
        };
}
void from_json(const nlohmann::json& j, FunctionParamInfo& o){
    j.at("Func Identifier").get_to(o.FuncId);
    j.at("Func Type").get_to(o.FuncType);
    j.at("Func Location").get_to(o.FuncLocation);
    j.at("Identifier").get_to(o.Id);
    o.CK = fromString.at(j.at("CK").get<std::string>());
    j.at("copy/move ctor is compiler-generated").get_to(o.CompilerGenerated);
}
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CallExprInfo, Location);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ConstructInfo, CallExpr, Parameter);


void MoveSemanticsAnalysis::CopyOrMoveAnalyzer::analyzeFeatures() {
    // Gives us a triad of callexpr, a pass-by-value record type function
    // parameter and the expr that is the argument to that parameter.
    auto m = callExpr(isExpansionInMainFile(),
        forEachArgumentWithParam(
            cxxConstructExpr().bind("arg"),
            parmVarDecl(hasType(recordType()), isExpansionInMainFile()).bind("parm")))
            .bind("callexpr");
    auto Res = Extractor.extract2(*Context, m);
    auto Args = getASTNodes<clang::CXXConstructExpr>(Res, "arg");
    auto Parms = getASTNodes<clang::ParmVarDecl>(Res, "parm");
    auto Calls = getASTNodes<clang::CallExpr>(Res, "callexpr");
    assert(Args.size() == Calls.size() && Parms.size() == Calls.size());

    int n = Args.size();
    for(int idx=0; idx<n; idx++) {
        auto p = Parms.at(idx);
        auto a = Args.at(idx);
        auto c = Calls.at(idx);
        auto f = c.Node->getDirectCallee();
        FunctionParamInfo FPI;
        CallExprInfo CEI;
        ConstructInfo CI;
        auto Ctor = a.Node->getConstructor();
        std::cout << a.Location << ", " <<
            Ctor->getQualifiedNameAsString();
        if(Ctor->isCopyConstructor()){
            std::cout << " copy, ";
        } else if(Ctor->isMoveConstructor()){
            std::cout << " move, ";
        } else if(Ctor->isDefaultConstructor()){
            std::cout << " def, ";
        }
        std::cout << a.Node->isElidable() << "\n";

        // what callee should we do here?
        FPI.FuncId = f->getQualifiedNameAsString();
        FPI.FuncType = f->getType().getAsString();
        FPI.Id = p.Node->getQualifiedNameAsString();
        FPI.FuncLocation = Context->getFullLoc(f->getInnerLocStart())
            .getLineNumber();
        if(Ctor->isCopyOrMoveConstructor())
            FPI.CK = static_cast<ConstructKind>(Ctor->isMoveConstructor());
        else
            FPI.CK = ConstructKind::Unknown;
        FPI.CompilerGenerated = Ctor->isImplicit();
        CEI.Location = c.Location;
        CI.Parameter = FPI;
        CI.CallExpr = CEI;
        nlohmann::json c_j = CI;
        Features.emplace_back(c_j);
    }
}
void MoveSemanticsAnalysis::CopyOrMoveAnalyzer::processFeatures(ojson j) {

}

} // namespace msa

//-----------------------------------------------------------------------------
