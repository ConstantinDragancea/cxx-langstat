#ifndef MODERNKEYWORDSANALYSIS_H
#define MODERNKEYWORDSANALYSIS_H

#include <string_view>

#include "cxx-langstat/Analysis.h"

//-----------------------------------------------------------------------------

// A standard library analysis is a template instantiation analysis.
class ModernKeywordsAnalysis : public Analysis {
public:
    ModernKeywordsAnalysis(){
        std::cout << "MKA ctor\n";
    }
    ~ModernKeywordsAnalysis(){
        std::cout << "MKA dtor\n";
    }
    std::string_view getShorthand() override {
        return ShorthandName;
    }

    struct MKInfo {
        MKInfo(unsigned Location = 0, const std::string& GlobalLocation = "") 
            : Location(Location), GlobalLocation(GlobalLocation) {}
        unsigned Location;
        std::string GlobalLocation;
    };

    struct MKAlignment : public MKInfo {
        MKAlignment(unsigned Location = 0, const std::string& GlobalLocation = "", size_t Alignment = 0) 
            : MKInfo(Location, GlobalLocation), Alignment(Alignment) {}
        size_t Alignment; // in bytes
    };

    struct MKFinal : public MKInfo {
        enum class FinalKind { Class, Method, Unknown };
        MKFinal(unsigned Location = 0, const std::string& GlobalLocation = "", FinalKind Kind = FinalKind::Unknown)
            : MKInfo(Location, GlobalLocation), Kind(Kind) {}
        FinalKind Kind;
    };

    struct MKFunctionSpecifiers : public MKInfo {
        enum class FunctionDeclKind { Constructor, CopyConstructor, MoveConstructor, MemberFunction, Function, Unknown };

        MKFunctionSpecifiers(unsigned Location = 0, const std::string& GlobalLocation = "", FunctionDeclKind Kind = FunctionDeclKind::Unknown, const std::string& FunctionHeader = "")
            : MKInfo(Location, GlobalLocation), Kind(Kind), FunctionHeader(FunctionHeader) {}
        FunctionDeclKind Kind;
        std::string FunctionHeader;
    };

    struct MKNoexcept : public MKInfo {
        enum class NoexceptKind { Specifier, Operator, Unknown };
        MKNoexcept(unsigned Location = 0, const std::string& GlobalLocation = "", NoexceptKind Kind = NoexceptKind::Unknown, const std::string& FunctionHeader = "")
            : MKInfo(Location, GlobalLocation), Kind(Kind), FunctionHeader(FunctionHeader) {}
        NoexceptKind Kind;
        std::string FunctionHeader;
    };

    struct MKThreadLocal : public MKInfo {
        MKThreadLocal(unsigned Location = 0, const std::string& GlobalLocation = "", const std::string& Type = "")
            : MKInfo(Location, GlobalLocation), Type(Type) {}
        std::string Type;
    };

    struct MKOverride : public MKInfo {
        MKOverride(unsigned Location = 0, const std::string& GlobalLocation = "", bool isOverride = false)
            : MKInfo(Location, GlobalLocation), isOverride(isOverride) {}
        bool isOverride;
    };
private:
    void extractFeatures();
    void analyzeFeatures() override;
    void processFeatures(const nlohmann::ordered_json& j) override;
    template<typename T>
    void featuresToJSON(const std::string& Kind, const std::vector<T>& features);
    template<typename T>
    void ProcessFeaturesKind(const std::string& Kind, const std::vector<T>& features);

    // containers for storing matches
    std::vector<MKAlignment>            AlignAs;
    std::vector<MKAlignment>            AlignOf;
    std::vector<MKFunctionSpecifiers>   AllDefault;
    std::vector<MKFunctionSpecifiers>   ExplicitDefault;
    std::vector<MKFunctionSpecifiers>   Deleted;
    std::vector<MKFinal>                Final;
    std::vector<MKInfo>                 InlineNamespace;
    std::vector<MKNoexcept>             ExplicitNoexcept;
    std::vector<MKNoexcept>             AllNoexcept;
    std::vector<MKInfo>                 Nullptr;
    std::vector<MKThreadLocal>          ThreadLocal;
    std::vector<MKOverride>             Override;
    std::vector<MKInfo>                 StaticAsserts;

    // JSON keys
    static constexpr auto ModernKeywordsPrevalenceKey = "modern keywords prevalence";
    static constexpr auto AlignAsKey = "alignas";
    static constexpr auto AlignOfKey = "alignof";
    static constexpr auto AllDefaultKey = "all default";
    static constexpr auto ExplicitDefaultKey = "explicit default";
    static constexpr auto DeleteKey = "delete";
    static constexpr auto FinalKey = "final";
    static constexpr auto InlineNamespaceKey = "inline namespace";
    static constexpr auto ExplicitNoexceptKey = "explicit noexcept";
    static constexpr auto AllNoexceptKey = "all noexcept";
    static constexpr auto NullptrKey = "nullptr";
    static constexpr auto ThreadLocalKey = "thread_local";
    static constexpr auto OverrideKey = "override";
    static constexpr auto StaticAssertKey = "static_assert";

    //
    static constexpr auto ShorthandName = "mka";

public:
    static constexpr std::array<decltype(AlignAsKey), 13> getFeatureKeys() {
        return { AlignAsKey, AlignOfKey, AllDefaultKey, ExplicitDefaultKey, DeleteKey, FinalKey, InlineNamespaceKey, AllNoexceptKey, ExplicitNoexceptKey, NullptrKey, ThreadLocalKey, OverrideKey, StaticAssertKey };
    }
};

//-----------------------------------------------------------------------------

#endif // MODERNKEYWORDSANALYSIS_H
