#ifndef CONCURRENCYTOOLSANALYSIS_H
#define CONCURRENCYTOOLSANALYSIS_H

#include <string_view>
#include <unordered_map>
#include <array>

#include "cxx-langstat/Analysis.h"
#include "cxx-langstat/Analyses/TemplateInstantiationAnalysis.h"
#include "cxx-langstat/Analyses/ObjectUsageAnalysis.h"

//-----------------------------------------------------------------------------

class ConcurrencyToolsAnalysis : public Analysis {
public:
    ConcurrencyToolsAnalysis(){
        std::cout << "CTA ctor\n";
    }
    ~ConcurrencyToolsAnalysis(){
        std::cout << "CTA dtor\n";
    }
    std::string_view getShorthand() override {
        return ShorthandName;
    }
protected:
    class PromiseFutureAnalyzer : public TemplateInstantiationAnalysis {
    public:
        PromiseFutureAnalyzer();
        ~PromiseFutureAnalyzer(){
            std::cout << "CTA::PromiseFutureAnalyzer dtor\n";
        }
        // void processFeatures(const nlohmann::ordered_json& j) override;
        static constexpr auto ShorthandName = "cta::pfa";
    };

    class AsyncAnalyzer : public TemplateInstantiationAnalysis {
    public:
        AsyncAnalyzer();
        ~AsyncAnalyzer(){
            std::cout << "CTA::AsyncAnalyzer dtor\n";
        }
        // void processFeatures(const nlohmann::ordered_json& j) override;
        static constexpr auto ShorthandName = "cta::async_analyzer";
    };

    class AtomicAnalyzer : public TemplateInstantiationAnalysis {
    public:
        AtomicAnalyzer();
        ~AtomicAnalyzer(){
            std::cout << "CTA::AtomicAnalyzer dtor\n";
        }
        // void processFeatures(const nlohmann::ordered_json& j) override;
        static constexpr auto ShorthandName = "cta::atomic_analyzer";
    };

    class ThreadAnalyzer : public ObjectUsageAnalysis {
    public:
        ThreadAnalyzer();
        ~ThreadAnalyzer(){
            std::cout << "CTA::ThreadAnalyzer dtor\n";
        }
        static constexpr auto ShorthandName = "cta::thread_analyzer";
    };

    class MutexAnalyzer : public ObjectUsageAnalysis {
    public:
        MutexAnalyzer();
        ~MutexAnalyzer(){
            std::cout << "CTA::MutexAnalyzer dtor\n";
        }
        static constexpr auto ShorthandName = "cta::mutex_analyzer";
    };

    // class TestAnalyzer : public TemplateInstantiationAnalysis {
    // public:
    //     TestAnalyzer();
    //     ~TestAnalyzer(){
    //         std::cout << "CTA::TestAnalyzer dtor\n";
    //     }
    //     static constexpr auto ShorthandName = "cta::test_analyzer";
    // };

    void analyzeFeatures() override;
    void processFeatures(const nlohmann::ordered_json& j) override;

    // Sub-analyzers
    PromiseFutureAnalyzer promiseFutureAnalyzer;
    AsyncAnalyzer asyncAnalyzer;
    AtomicAnalyzer atomicAnalyzer;
    ThreadAnalyzer threadAnalyzer;
    MutexAnalyzer mutexAnalyzer;
    // TestAnalyzer testAnalyzer;

    // JSON keys
    static constexpr auto ConcurrencyToolsStatisticsKey = "concurrency tools usage";
    static constexpr auto PromiseFutureFeatureKey = "promise, future usage";
    static constexpr auto AsyncFeatureKey = "async usage";
    static constexpr auto AtomicFeatureKey = "atomic usage";
    static constexpr auto ThreadFeatureKey = "thread usage";
    static constexpr auto MutexFeatureKey = "mutex usage";
    static constexpr auto TestFeatureKey = "test usage";
    
    //
    static constexpr auto ShorthandName = "cta";
public:
    static constexpr std::array<decltype(PromiseFutureFeatureKey), 5> getFeatureKeys() {
        return {PromiseFutureFeatureKey, AsyncFeatureKey, AtomicFeatureKey, ThreadFeatureKey, MutexFeatureKey};
    }

    const std::unordered_map<std::string, Analysis*> m_analyzerMap = {
        {PromiseFutureFeatureKey, &promiseFutureAnalyzer},
        {AsyncFeatureKey, &asyncAnalyzer},
        {AtomicFeatureKey, &atomicAnalyzer},
        {ThreadFeatureKey, &threadAnalyzer},
        {MutexFeatureKey, &mutexAnalyzer}
        // {TestFeatureKey, &testAnalyzer}
    };
};

//-----------------------------------------------------------------------------

#endif // CONCURRENCYTOOLSANALYSIS_H
