#ifndef FEATURESPROCESSING_H
#define FEATURESPROCESSING_H

#include <nlohmann/json.hpp>
#include <unordered_map>
#include <iostream>

#include "cxx-langstat/AnalysisRegistry.h"
#include "cxx-langstat/AnalysisList.h"
#include "cxx-langstat/Analysis.h"

#include "cxx-langstat/Analyses/AlgorithmLibraryAnalysis.h"
#include "cxx-langstat/Analyses/ConstexprAnalysis.h"
#include "cxx-langstat/Analyses/CyclomaticComplexityAnalysis.h"
#include "cxx-langstat/Analyses/ConcurrencyToolsAnalysis.h"
#include "cxx-langstat/Analyses/ContainerLibAnalysis.h"
#include "cxx-langstat/Analyses/FunctionParameterAnalysis.h"
#include "cxx-langstat/Analyses/LoopDepthAnalysis.h"
#include "cxx-langstat/Analyses/LoopKindAnalysis.h"
#include "cxx-langstat/Analyses/ModernKeywordsAnalysis.h"
#include "cxx-langstat/Analyses/MoveSemanticsAnalysis.h"
#include "cxx-langstat/Analyses/TemplateInstantiationAnalysis.h"
#include "cxx-langstat/Analyses/TemplateParameterAnalysis.h"
#include "cxx-langstat/Analyses/UsingAnalysis.h"
#include "cxx-langstat/Analyses/UtilityLibAnalysis.h"
#include "cxx-langstat/Analyses/VariableTemplateAnalysis.h"

namespace FeaturesPostProcessing {

using nlohmann::json;
using nlohmann::ordered_json;
using std::unordered_map;

ordered_json filterFeatures(const ordered_json& instances, unordered_map<json, int>& existing_instances){
    ordered_json new_instances = json::array();

    for (const auto& info : instances){
        if (!existing_instances.count(info)){
            existing_instances[info] = 1;
            new_instances.push_back(info);
        }
    }

    return new_instances;
}


using JsonMap = unordered_map<json, int>;
using JsonMap2 = unordered_map<std::string, JsonMap>;
using JsonMap3 = unordered_map<std::string, JsonMap2>;
using JsonMap4 = unordered_map<std::string, JsonMap3>;

#define POSTPROCESSING_ONE_LAYER(AnalysisType) \
template <> \
ordered_json postProcessFeatures<AnalysisType>(ordered_json&& Features){ \
    static unordered_map<std::string, unordered_map<json, int>> already_matched; \
    \
    for (const auto& keyword : AnalysisType::getFeatureKeys()){ \
        ordered_json new_instances = filterFeatures(Features[keyword], already_matched[keyword]); \
        Features[keyword] = new_instances; \
    } \
    \
    return Features; \
}

#define POSTPROCESSING_ONE_LAYER_MULTIPLE_INSTANCES(AnalysisType) \
template <> \
ordered_json postProcessFeatures<AnalysisType>(ordered_json&& Features){ \
    static std::unordered_map<std::string, \
        std::unordered_map<std::string, \
            std::unordered_map<json, int> > > already_matched; \
    for (const auto& feature_key : AnalysisType::getFeatureKeys()){ \
        if (!Features.contains(feature_key)) \
            continue; \
        for (const auto& [feature_key_instance, feature_instances] : Features[feature_key].items()){ \
            if (!Features[feature_key].contains(feature_key_instance)) \
                continue; \
            ordered_json new_instances = filterFeatures(Features[feature_key][feature_key_instance], already_matched[feature_key][feature_key_instance]); \
            Features[feature_key][feature_key_instance] = new_instances; \
        } \
    } \
    return Features; \
}

#define POSTPROCESS_ANALYSIS_INHERITING_FROM_TEMPLATE(AnalysisType) \
template <> \
ordered_json postProcessFeatures<AnalysisType>(ordered_json&& Features){ \
    static std::unordered_map<std::string, \
        std::unordered_map<std::string, \
            std::unordered_map<json, int> > > already_matched; \
    for (const auto& TIA_feature_key : TemplateInstantiationAnalysis::getFeatureKeys()){ \
        if (!Features.contains(TIA_feature_key)) \
            continue; \
        for (const auto& feature_key : AnalysisType::getFeatureKeys()){ \
            if (!Features[TIA_feature_key].contains(feature_key)) \
                continue; \
            ordered_json new_instances = filterFeatures(Features[TIA_feature_key][feature_key], already_matched[TIA_feature_key][feature_key]); \
            Features[feature_key][TIA_feature_key][feature_name] = new_instances; \
        } \
    } \
    return Features; \
}

#define POSTPROCESS_ANALYSIS_DEPTH_3(AnalysisType) \
template <> \
ordered_json postProcessFeatures<AnalysisType>(ordered_json&& Features){ \
    static std::unordered_map<std::string, \
        std::unordered_map<std::string, \
            std::unordered_map<std::string, \
                std::unordered_map<json, int> > > > already_matched; \
    for (auto& [key1, value1]: Features.items()){ \
        for (auto& [key2, value2]: value1.items()){ \
            for (auto& [key3, value3]: value2.items()){ \
                ordered_json new_instances = filterFeatures(value3, already_matched[key1][key2][key3]); \
                Features[key1][key2][key3] = new_instances; \
            } \
        } \
    } \
    return Features; \
}

template <typename AnalysisType>
ordered_json postProcessFeatures(ordered_json&& Features){
    return Features;
}

// template <>
// ordered_json postProcessFeatures<ModernKeywordsAnalysis>(ordered_json&& Features){
//     // static std::unordered_map<std::string, std::unordered_map<std::string, int>> used_global_locations;
//     static std::unordered_map<std::string, std::unordered_map<json, int>> used_global_locations;

//     for (const auto& keyword : ModernKeywordsAnalysis::getFeatureKeys()){
//         ordered_json new_instances = json::array();

//         for (const auto& info : Features[keyword]){
//             if (!used_global_locations[keyword].count(info)){
//                 used_global_locations[keyword][info] = 1;
//                 new_instances.push_back(info);
//             }
//         }

//         Features[keyword] = new_instances;
//     }

//     return Features;
// }

// template <>
// ordered_json postProcessFeatures<TemplateInstantiationAnalysis>(ordered_json&& Features){
//     static std::unordered_map<std::string, 
//         std::unordered_map<std::string, 
//             std::unordered_map<json, int> > > used_global_locations;

//     for (const auto& feature_key : TemplateInstantiationAnalysis::getFeatureKeys()){
//         if (!Features.contains(feature_key))
//             continue;
//         for (const auto& [feature_name, instances] : Features[feature_key].items()){
//             ordered_json new_instances = json::array();

//             for (const auto& info : instances){
//                 if (!used_global_locations[feature_key][feature_name].count(info)){
//                     used_global_locations[feature_key][feature_name][info] = 1;
//                     new_instances.push_back(info);
//                 }
//                 // if (!used_global_locations[feature_key][feature_name].count(info["GlobalLocation"])){
//                 //     used_global_locations[feature_key][feature_name][info["GlobalLocation"]] = 1;
//                 //     new_instances.push_back(info);
//                 // }
//             }

//             Features[feature_key][feature_name] = new_instances;
//         }
//     }

//     return Features;
// }

// template <>
// ordered_json postProcessFeatures<ConstexprAnalysis>(ordered_json&& Features){
//     static std::unordered_map<std::string, 
//             std::unordered_map<json, int> > used_global_locations;
    
//     for (const auto& feature_key : ConstexprAnalysis::getFeatureKeys()){
//         if (!Features.contains(feature_key))
//             continue;

//         ordered_json new_instances = json::array();

//         for (const auto& info : Features[feature_key]){
//             if (!used_global_locations[feature_key].count(info)){
//                 used_global_locations[feature_key][info] = 1;
//                 new_instances.push_back(info);
//             }
//         }

//         Features[feature_key] = new_instances;
//     }

//     return Features;
// }

// template <>
// ordered_json postProcessFeatures<LoopKindAnalysis>(ordered_json&& Features){
//     static std::unordered_map<std::string, 
//             std::unordered_map<json, int> > used_global_locations;
    
//     for (const auto& feature_key : LoopKindAnalysis::getFeatureKeys()){
//         if (!Features.contains(feature_key))
//             continue;

//         ordered_json new_instances = json::array();

//         for (const auto& info : Features[feature_key]){
//             if (!used_global_locations[feature_key].count(info)){
//                 used_global_locations[feature_key][info] = 1;
//                 new_instances.push_back(info);
//             }
//         }

//         Features[feature_key] = new_instances;
//     }

//     return Features;
// }

template <>
ordered_json postProcessFeatures<msa::MoveSemanticsAnalysis>(ordered_json&& Features){
    static unordered_map<std::string, 
        unordered_map<std::string, 
            unordered_map<std::string,
                unordered_map<json, int > > > > used_global_locations;
    
    for (const auto& feature_key : msa::MoveSemanticsAnalysis::getFeatureKeys()){
        if (!Features.contains(feature_key))
            continue;
        for (const auto& TIA_feature_key : TemplateInstantiationAnalysis::getFeatureKeys()){
            if (!Features[feature_key].contains(TIA_feature_key))
                continue;
            for (const auto& [feature_name, instances] : Features[feature_key][TIA_feature_key].items()){
                ordered_json new_instances = json::array();
                auto& conflicting_instances = used_global_locations[feature_key][TIA_feature_key][feature_name];

                for (const auto& info : instances){
                    if (!conflicting_instances.count(info)){
                        conflicting_instances[info] = 1;
                        new_instances.push_back(info);
                    }
                }

                Features[feature_key][TIA_feature_key][feature_name] = new_instances;
            }
        }
    }

    return Features;
}

POSTPROCESSING_ONE_LAYER(ModernKeywordsAnalysis);
POSTPROCESSING_ONE_LAYER(ConstexprAnalysis);
POSTPROCESSING_ONE_LAYER_MULTIPLE_INSTANCES(TemplateInstantiationAnalysis);
POSTPROCESSING_ONE_LAYER(LoopKindAnalysis);
POSTPROCESSING_ONE_LAYER(UsingAnalysis);
POSTPROCESSING_ONE_LAYER_MULTIPLE_INSTANCES(UtilityLibAnalysis);
POSTPROCESS_ANALYSIS_DEPTH_3(ConcurrencyToolsAnalysis);

#undef POSTPROCESSING_ONE_LAYER
#undef POSTPROCESSING_ONE_LAYER_MULTIPLE_INSTANCES
#undef POSTPROCESS_ANALYSIS_INHERITING_FROM_TEMPLATE
#undef POSTPROCESS_ANALYSIS_DEPTH_3

ordered_json postProcessFeaturesWrapper(std::string_view AnalysisShorthand, ordered_json Features){
    if (AnalysisShorthand == "cea")
        return postProcessFeatures<ConstexprAnalysis>(std::move(Features));
    else if (AnalysisShorthand == "tia" || AnalysisShorthand == "ala" || AnalysisShorthand == "cla")
        return postProcessFeatures<TemplateInstantiationAnalysis>(std::move(Features));
    // else if (AnalysisShorthand == "cca")
    //     return postProcessFeatures<CyclomaticComplexityAnalysis>(std::move(Features));
    else if (AnalysisShorthand == "lka")
        return postProcessFeatures<LoopKindAnalysis>(std::move(Features));
    else if (AnalysisShorthand == "mka")
        return postProcessFeatures<ModernKeywordsAnalysis>(std::move(Features));
    else if (AnalysisShorthand == "msa")
        return postProcessFeatures<msa::MoveSemanticsAnalysis>(std::move(Features));
    else if (AnalysisShorthand == "ua")
        return postProcessFeatures<UsingAnalysis>(std::move(Features));
    else if (AnalysisShorthand == "ula")
        return postProcessFeatures<UtilityLibAnalysis>(std::move(Features));
    else if (AnalysisShorthand == "cta")
        return postProcessFeatures<ConcurrencyToolsAnalysis>(std::move(Features));
    else
        return postProcessFeatures<Analysis>(std::move(Features));
}

}   // FeaturesPostProcessing namespace

#endif // FEATURESPROCESSING_H