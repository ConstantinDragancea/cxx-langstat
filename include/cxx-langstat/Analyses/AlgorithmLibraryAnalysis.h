#ifndef ALGORITHMLIBRARYANALYSIS_H
#define ALGORITHMLIBRARYANALYSIS_H

#include <string_view>

#include "cxx-langstat/Analysis.h"
#include "cxx-langstat/Analyses/TemplateInstantiationAnalysis.h"

//-----------------------------------------------------------------------------
// A standard library analysis is a template instantiation analysis.
class AlgorithmLibraryAnalysis : public TemplateInstantiationAnalysis {
public:
    AlgorithmLibraryAnalysis();
    ~AlgorithmLibraryAnalysis(){
        std::cout << "ALA dtor\n";
    }
    std::string_view getShorthand() override {
        return ShorthandName;
    }
protected:
    void analyzeFeatures() override;
    void processFeatures(const nlohmann::ordered_json& j) override;
    // JSON keys
    const std::string AlgorithmPrevalenceKey = "algorithm type prevalence";
    //
    static constexpr auto ShorthandName = "ala";
};

//-----------------------------------------------------------------------------

#endif // ALGORITHMLIBRARYANALYSIS_H
