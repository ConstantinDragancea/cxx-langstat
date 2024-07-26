#ifndef LOOPKINDANALYSIS_H
#define LOOPKINDANALYSIS_H

#include <string_view>

#include "cxx-langstat/Analysis.h"

//-----------------------------------------------------------------------------

class LoopKindAnalysis : public Analysis {
public:
    LoopKindAnalysis(){
        std::cout << "LKA ctor\n";
    }
    ~LoopKindAnalysis(){
        std::cout << "LKA dtor\n";
    }
    std::string_view getShorthand() override {
        return ShorthandName;
    }
private:
    void analyzeFeatures() override;
    void processFeatures(const nlohmann::ordered_json& j) override;

    // JSON keys
    static constexpr auto LoopKindPrevalencesKey = "loop kind prevalences";
    static constexpr auto ForLoopKey = "for";
    static constexpr auto WhileLoopKey = "while";
    static constexpr auto DoWhileLoopKey = "do-while";
    static constexpr auto RangeBasedForLoopKey = "range-for";

    //
    static constexpr auto ShorthandName = "lka";
public:
    static constexpr std::array<decltype(ForLoopKey), 4> getFeatureKeys() {
        return { ForLoopKey, WhileLoopKey, DoWhileLoopKey, RangeBasedForLoopKey };
    };
};

//-----------------------------------------------------------------------------

#endif /* LOOPKINDANALYSIS_H */
