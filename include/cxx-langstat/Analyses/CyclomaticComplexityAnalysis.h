#ifndef CYCLOMATICCOMPLEXITYANALYSIS_H
#define CYCLOMATICCOMPLEXITYANALYSIS_H

#include <string_view>

#include "cxx-langstat/Analysis.h"

//-----------------------------------------------------------------------------

class CyclomaticComplexityAnalysis : public Analysis {
public:
    CyclomaticComplexityAnalysis(){
        std::cout << "CCA ctor\n";
    }
    ~CyclomaticComplexityAnalysis(){
        std::cout << "CCA dtor\n";
    }
    std::string_view getShorthand() override {
        return ShorthandName;
    }
private:
    // Extracts features, calculates cyclomatic complexity and creates JSON
    // objects all in one go, since that is all pretty simple.
    void analyzeFeatures() override;
    void processFeatures(const nlohmann::ordered_json& j) override;

    // JSON keys
    static constexpr auto CyclomaticComplexityDistributionKey = "distribution of cyclomatic complexity of functions";
    static constexpr auto fdeclKey = "fdecl";

    //
    static constexpr auto ShorthandName = "cca";

public:
    static constexpr std::array<decltype(fdeclKey), 1> getFeatureKeys() {
        return { fdeclKey };
    }
};

#endif // CYCLOMATICCOMPLEXITY_H

//-----------------------------------------------------------------------------
