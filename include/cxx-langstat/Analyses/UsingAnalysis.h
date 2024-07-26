#ifndef USINGANALYSIS_H
#define USINGANALYSIS_H

#include <string>
#include <string_view>
#include <utility>
#include <array>
#include <vector>

#include "cxx-langstat/Analysis.h"
#include "cxx-langstat/Utils.h"

//-----------------------------------------------------------------------------


class UsingAnalysis : public Analysis {
public:
    UsingAnalysis(){
        std::cout << "UA ctor\n";
    }
    ~UsingAnalysis(){
        std::cout << "UA dtor\n";
    }
    std::string_view getShorthand() override {
        return ShorthandName;
    }

    // enum class SynonymKind {
    //     Typedef, Using, Unknown
    // };
    // Stores information about a single Synonym, i.e. a single typedef or type alias.
    struct SynonymInfo {
        SynonymInfo(){}
        // Synonym(unsigned Location, std::shared_ptr<std::string> GlobalLocation, SynonymKind Kind) :
        //     Location(Location),
        //     Kind(Kind) { }
        // Synonym(unsigned Location, const std::string& GlobalLocation, SynonymKind Kind, bool isTemplated)
        //     : Location(Location), GlobalLocation(std::make_shared<std::string>(GlobalLocation)), Kind(Kind) { }
        SynonymInfo(unsigned Location, std::shared_ptr<std::string> GlobalLocation)
            : Location(Location), GlobalLocation(GlobalLocation) { }
        SynonymInfo(unsigned Location, const std::string& GlobalLocation)
            : Location(Location), GlobalLocation(std::make_shared<std::string>(GlobalLocation)) { }
        unsigned Location;
        std::shared_ptr<std::string> GlobalLocation;
        // SynonymKind Kind;
    };

private:
    void extractFeatures();
    void analyzeFeatures() override;
    void processFeatures(const nlohmann::ordered_json& j) override;
    template<typename T>
    void featuresToJSON(const std::string& Kind, const std::vector<T>& features);
    
    // containers for storing matches
    std::vector<SynonymInfo> Typedefs;
    std::vector<SynonymInfo> Usings;
    std::vector<SynonymInfo> TemplatedTypedefs;
    std::vector<SynonymInfo> TemplatedUsings;

    // JSON keys
    static constexpr auto TypedefKey = "typedef";
    static constexpr auto TemplatedTypedefKey = "templated typedef";
    static constexpr auto UsingKey = "using";
    static constexpr auto TemplatedUsingKey = "templated using";
    static constexpr auto AliasKindPrevalenceKey = "prevalence of typedef/using";

    //
    static constexpr auto ShorthandName = "ua";

public:
    static constexpr std::array<decltype(TypedefKey), 4> getFeatureKeys() {
        return {TypedefKey, UsingKey, TemplatedTypedefKey, TemplatedUsingKey};
    }
};

//-----------------------------------------------------------------------------

#endif // USINGANALYSIS_H
