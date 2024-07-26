#ifndef MOVESEMANTICSANALYSIS_H
#define MOVESEMANTICSANALYSIS_H

#include <string_view>
#include <utility>
#include <iostream>
#include <array>

#include "cxx-langstat/Analyses/TemplateInstantiationAnalysis.h"
#include "cxx-langstat/Utils.h"

namespace msa {

// Enum of the parmVarDecl of a functionDecl was constructed from the argument
// of a callExpr. Also, maps for prettier printing.
enum class ConstructKind { 
    Copy,
    Move,
    Unknown // it might be elided, might not be elided. depends on the compiler
};
constexpr std::array<std::string_view, 3> S = {"copy", "move", "unknown"};
const std::unordered_map<ConstructKind, std::string_view> toString = {
    {ConstructKind::Copy, S[0]},
    {ConstructKind::Move, S[1]},
    {ConstructKind::Unknown, S[2]}
};
const std::unordered_map<std::string_view, ConstructKind> fromString = {
    {S[0], ConstructKind::Copy},
    {S[1], ConstructKind::Move},
    {S[2], ConstructKind::Unknown}
};

// Holds relevant information about a parmVarDecl.
struct FunctionParamInfo {
    std::string FuncId;
    std::string FuncType;
    unsigned FuncLocation;
    std::string Id;
    ConstructKind CK;
    bool CompilerGenerated;
    std::string ParmType;
};
struct CallExprInfo {
    unsigned Location;
    std::shared_ptr<std::string> GlobalLocation;
};
// Holds info about parmVarDecl and the corresponding callExpr that causes it
// to be copied or moved to.
struct ConstructInfo {
    CallExprInfo CallExpr;
    FunctionParamInfo Parameter;
};

// Need to make MSA that adheres to Analysis interface, but that can call two
// independent other analyses.
// MSA is guaranteed to be called through analyzeFeatures and processFeatures.
class MoveSemanticsAnalysis : public Analysis {
public:
    MoveSemanticsAnalysis(){
        std::cout << "MSA ctor\n";
    }
    ~MoveSemanticsAnalysis(){
        std::cout << "MSA dtor\n";
    }

private:
    void analyzeFeatures() override {
        Features[MoveAndForwardUsageKey] = MoveForwardAnalyzer.getFeatures(InFile, *Context);
        Features[CopyOrMoveConstrKey] = CopyMoveConstrAnalyzer.getFeatures(InFile, *Context);
    }
    void processFeatures(const nlohmann::ordered_json& j) override {
        Statistics[MoveAndForwardUsageKey] = MoveForwardAnalyzer.getStatistics(j);
        Statistics[CopyOrMoveConstrKey] = CopyMoveConstrAnalyzer.getStatistics(j);
    }
    std::string_view getShorthand() override {
        return ShorthandName;
    }
    // Find how often std::move, std::forward from <utility> are used.
    struct StdMoveStdForwardUsageAnalyzer : public TemplateInstantiationAnalysis {
        StdMoveStdForwardUsageAnalyzer();
        void analyzeFeatures() override;
        void processFeatures(const nlohmann::ordered_json& j) override;
        std::string_view getShorthand() override { 
            using namespace std::literals::string_view_literals;
            return "msap1"sv; 
        }
    };
    // Examine when calling functions that pass by value, how often copy and
    // move constructors are used to construct the value of the callee.
    struct CopyOrMoveAnalyzer : public Analysis {
        CopyOrMoveAnalyzer() { std::cout << "CopyOrMoveAnalyzer CTOR\n"; }
        void analyzeFeatures() override;
        void processFeatures(const nlohmann::ordered_json& j) override;
        std::string_view getShorthand() override { 
            using namespace std::literals::string_view_literals;
            return "msap2"sv;
        }
    };

    // Analyzers run by MSA
    StdMoveStdForwardUsageAnalyzer MoveForwardAnalyzer;
    CopyOrMoveAnalyzer CopyMoveConstrAnalyzer;

    // JSON keys
    static constexpr auto MoveAndForwardUsageKey = "std::move, std::forward usage";
    static constexpr auto CopyOrMoveConstrKey = "copy or move construction";

    static constexpr auto ShorthandName = "msa";
public:
    static constexpr std::array<decltype(MoveAndForwardUsageKey), 2> getFeatureKeys(){
        return { MoveAndForwardUsageKey, CopyOrMoveConstrKey };
    }
};

} // namespace msa

#endif // MOVESEMANTICSANALYSIS_H
