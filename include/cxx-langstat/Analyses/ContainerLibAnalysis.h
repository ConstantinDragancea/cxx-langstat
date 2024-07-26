#ifndef CONTAINERLIBANALYSIS_H
#define CONTAINERLIBANALYSIS_H

#include <string_view>

#include "cxx-langstat/Analysis.h"
#include "cxx-langstat/Analyses/TemplateInstantiationAnalysis.h"

//-----------------------------------------------------------------------------

class ContainerLibAnalysis : public TemplateInstantiationAnalysis {
public:
    ContainerLibAnalysis();
    ~ContainerLibAnalysis(){
        std::cout << "CLA dtor\n";
    }
    std::string_view getShorthand() override {
        return ShorthandName;
    }
protected:
    void processFeatures(const nlohmann::ordered_json& j) override;
    // JSON keys
    const std::string ContainerPrevalenceKey = "container type prevalence";
    const std::string ContainedTypesPrevalenceKey =
        "container instantiation type arguments";
    //
    static constexpr auto ShorthandName = "cla";
};

//-----------------------------------------------------------------------------

#endif // CONTAINERLIBANALYSIS_H
