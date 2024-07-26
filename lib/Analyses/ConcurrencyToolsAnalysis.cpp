#include <iostream>
#include <vector>

#include "cxx-langstat/Analysis.h"
#include "cxx-langstat/Analyses/ConcurrencyToolsAnalysis.h"
#include "cxx-langstat/Utils.h"
#include "cxx-langstat/Stats.h"

using namespace clang;
using namespace clang::ast_matchers;
using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;

//-----------------------------------------------------------------------------

ConcurrencyToolsAnalysis::PromiseFutureAnalyzer::PromiseFutureAnalyzer(): TemplateInstantiationAnalysis(
    TemplateInstantiationAnalysis::InstKind::Class,
    hasAnyName(
        "std::promise", "std::future", "std::shared_future"
    ),
    "future$|"
    "/bits/.*$"
    ){
    std::cout << "CTA::PromiseFutureAnalyzer ctor\n";
}

ConcurrencyToolsAnalysis::AsyncAnalyzer::AsyncAnalyzer(): TemplateInstantiationAnalysis(
    TemplateInstantiationAnalysis::InstKind::Function,
    hasAnyName(
        "std::async"
    ),
    "future$|"
    "/bits/.*$"
    ){
    std::cout << "CTA::AsyncAnalyzer ctor\n";
}

ConcurrencyToolsAnalysis::AtomicAnalyzer::AtomicAnalyzer(): TemplateInstantiationAnalysis(
    TemplateInstantiationAnalysis::InstKind::Class,
    hasAnyName(
        "std::atomic"
    ),
    // libc++:
    "atomic$|"
    // libstdc++
    // "stdatomic\\.h$|bits/atomic_base\\.h$"
    "/bits/.*$"
    ){
    std::cout << "CTA::AtomicAnalyzer ctor\n";
}

ConcurrencyToolsAnalysis::ThreadAnalyzer::ThreadAnalyzer(): ObjectUsageAnalysis(
    ObjectUsageAnalysis::InstKind::Variable,
    hasAnyName("std::thread", "std::jthread"),
    "thread$|"
    "/bits/.*$"
    ){
    std::cout << "CTA::ThreadAnalyzer ctor\n";
}

ConcurrencyToolsAnalysis::MutexAnalyzer::MutexAnalyzer(): ObjectUsageAnalysis(
    ObjectUsageAnalysis::InstKind::Variable,
    hasAnyName(
        "std::mutex", "std::recursive_mutex", "std::timed_mutex", "std::recursive_timed_mutex"
    ),
    // libc++:
    "mutex$|"
    "shared_mutex$|"
    // libstdc++
    // "bits/std_mutex\\.h$"
    "/bits/.*$"
    ){
    std::cout << "CTA::MutexAnalyzer ctor\n";
}

ConcurrencyToolsAnalysis::TestAnalyzer::TestAnalyzer():
    TemplateInstantiationAnalysis(
        TemplateInstantiationAnalysis::InstKind::Class,
        // hasAnyName("my_namespace::my_class"),
        anything(),
        // "header.hpp$"
        ".*"
    ){
    std::cout << "CTA::TestAnalyzer ctor\n";
}

void ConcurrencyToolsAnalysis::analyzeFeatures() {
    for (const auto& key : ConcurrencyToolsAnalysis::getFeatureKeys()) {
        // std::cout << "going to analyze features for " << m_analyzerMap.at(key)->getShorthand() << std::endl;
        Features[key] = m_analyzerMap.at(key)->getFeatures(InFile, *Context);
        // std::cout << "analyzed features for " << m_analyzerMap.at(key)->getShorthand() << std::endl;
    }
}

void ConcurrencyToolsAnalysis::processFeatures(const ordered_json& j) {
    for (const auto& key : ConcurrencyToolsAnalysis::getFeatureKeys()) {
        if(j.contains(key)){
            // std::cout << "going to process features for " << m_analyzerMap.at(key)->getShorthand() << std::endl;
            Statistics[key] = m_analyzerMap.at(key)->getStatistics(j.at(key));
            // std::cout << "processed features for " << m_analyzerMap.at(key)->getShorthand() << std::endl;
        }
    }
}

//-----------------------------------------------------------------------------
