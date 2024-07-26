#ifndef CONSTEXPRANALYSIS_H
#define CONSTEXPRANALYSIS_H

#include <string>
#include <string_view>
#include <utility>
#include <array>

#include "cxx-langstat/Analysis.h"
#include "cxx-langstat/Utils.h"

//-----------------------------------------------------------------------------

struct CEInfo {
    CEInfo(unsigned Location, std::shared_ptr<std::string> GlobalLocation, bool isConstexpr) 
        : Location(Location), GlobalLocation(GlobalLocation), isConstexpr(isConstexpr) {}
    CEInfo(unsigned Location, const std::string& GlobalLocation, bool isConstexpr):
        Location(Location), GlobalLocation(std::make_shared<std::string>(GlobalLocation)), isConstexpr(isConstexpr) {}
    unsigned Location;
    std::shared_ptr<std::string> GlobalLocation;
    bool isConstexpr;
};
// For variables, could possibly be ameliorated with information whether or
// not the type is instantiation dependent or not, which can be
// the case if the varDecl is part of a varTemplateDecl
struct CEDecl : public CEInfo {
    CEDecl(unsigned Location, std::shared_ptr<std::string> GlobalLocation, bool isConstexpr,
        std::string Id, std::string Type) :
        CEInfo(Location, GlobalLocation, isConstexpr), Identifier(Id), Type(Type){}
    CEDecl(unsigned Location, const std::string& GlobalLocation, bool isConstexpr,
        std::string Id, std::string Type) :
        CEInfo(Location, GlobalLocation, isConstexpr), Identifier(Id), Type(Type){}
    std::string Identifier;
    std::string Type;
};
struct CEIfStmt : public CEInfo {
    CEIfStmt(unsigned Location, std::shared_ptr<std::string> GlobalLocation, bool isConstexpr) : CEInfo(Location, GlobalLocation, isConstexpr){}
    CEIfStmt(unsigned Location, const std::string& GlobalLocation, bool isConstexpr) : CEInfo(Location, GlobalLocation, isConstexpr){}
};

class ConstexprAnalysis : public Analysis {
public:
    ConstexprAnalysis(){
        std::cout << "CEA ctor\n";
    }
    ~ConstexprAnalysis(){
        std::cout << "CEA dtor\n";
    }
    std::string_view getShorthand() override {
        return ShorthandName;
    }
private:
    std::vector<CEDecl> Variables;
    std::vector<CEDecl> NonMemberFunctions;
    std::vector<CEDecl> MemberFunctions;
    std::vector<CEIfStmt> IfStmts;
    void extractFeatures();
    void analyzeFeatures() override;
    void processFeatures(const nlohmann::ordered_json& j) override;
    template<typename T>
    void featuresToJSON(const std::string& Kind, const std::vector<T>& features);
    // JSON keys
    static constexpr auto VarKey = "vars";
    static constexpr auto NonMemberFuncKey = "non-member functions";
    static constexpr auto MemberFuncKey = "member functions";
    static constexpr auto IfKey = "if stmts";
    //
    static constexpr auto ShorthandName = "cea";
public:
    static constexpr std::array<decltype(VarKey), 4> getFeatureKeys(){
        return {VarKey, NonMemberFuncKey, MemberFuncKey, IfKey};
    }
};

//-----------------------------------------------------------------------------

#endif // CONSTEXPRANALYSIS_H
