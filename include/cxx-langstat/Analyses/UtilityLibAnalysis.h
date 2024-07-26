#ifndef UTILITYLIBANALYSIS_H
#define UTILITYLIBANALYSIS_H

#include <string_view>

#include "cxx-langstat/Analysis.h"
#include "cxx-langstat/Analyses/TemplateInstantiationAnalysis.h"

//-----------------------------------------------------------------------------

class UtilityLibAnalysis : public TemplateInstantiationAnalysis {
public:
    UtilityLibAnalysis();
    ~UtilityLibAnalysis(){
        std::cout << "ULA dtor\n";
    }
    std::string_view getShorthand() override {
        return ShorthandName;
    }
protected:
    void processFeatures(const nlohmann::ordered_json& j) override;
    // JSON keys
    static constexpr auto UtilityPrevalenceKey = "utility type prevalence";
    static constexpr auto UtilitiedTypesPrevalenceKey =
        "utility instantiation type arguments";
    
    //
    static constexpr auto ShorthandName = "ula";
};

//-----------------------------------------------------------------------------

#endif // UTILITYLIBANALYSIS_H
