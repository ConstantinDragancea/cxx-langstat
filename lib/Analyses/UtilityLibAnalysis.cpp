#include <iostream>
#include <vector>

#include "cxx-langstat/Analyses/UtilityLibAnalysis.h"
#include "cxx-langstat/Utils.h"
#include "cxx-langstat/Stats.h"

using namespace clang;
using namespace clang::ast_matchers;
using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;

//-----------------------------------------------------------------------------
// How often were constructs from standard library used (like vector, array,
// map, list, unordered_map, set etc.). Were they used directly as type,
// or as part of another constructs? What behavior can we see when they are
// passed around? What sizes do they occur (#elements, constexpr)?
// Usage in templates and TMP?

// Construct a ULA by constructing a more constrained TIA.
UtilityLibAnalysis::UtilityLibAnalysis() : TemplateInstantiationAnalysis(
    InstKind::Class,
    hasAnyName(
        // Standard library utilities
        // vocabulary types
        "std::pair", "std::tuple",
        "std::bitset",
        // Dynamic memory
        "std::unique_ptr", "std::shared_ptr", "std::weak_ptr"
    ),
    // libc++:
    // pair \in <utility>, tuple \in <tuple>, bitset \in <bitset>
    // all smart pointers \in <memory>
    "utility$|tuple$|bitset$|memory$|"
    // libstdc++
    // pair \in <bits/stl_pair.h>, tuple \in <tuple>, bitset \in <bitset>
    // unique_ptr \in <bits/unique_ptr.h>
    // shared_ptr, weak_ptr \in <bits/shared_ptr.h>
    // "bits/utility\\.h$|bits/stl_iterator\\.h$|"
    // "bits/stl_pair\\.h$|bits/unique_ptr\\.h$|bits/shared_ptr\\.h$"){
    "/bits/.*$"){
    std::cout << "ULA ctor\n";
}

namespace {

// From ContainerLibAnalysis.cpp
// Map that for a stdlib type contains how many instantiation type args are
// intersting to us, e.g. for the instantiation stored in a .json below,
// "std::vector" : {
//     "location": "42"
//     "arguments": {
//         "non-type": [],
//         "type": [
//             "int",
//             "std::allocator<int>"
//         ],
//         "template": []
//     }
// }
// the result is 1, since we only care about the fact that std::vector was
// instantiated with int.
// -1 indicates that we want all arguments interest us.
// Used to analyze types contained in containers, utilities, smart pointer etc.
// TODO: what to do if a type uses both types and non-types
const StringMap<int> NumRelTypes_type = { // no constexpr support for map
    // ** Utilities **
    // pairs and tuples
    {"std::pair", 2}, {"std::tuple", -1},
    // bitset
    {"std::bitset", 0},
    // smart pointers
    {"std::unique_ptr", 1}, {"std::shared_ptr", 1}, {"std::weak_ptr", 1}
};

const StringMap<int> NumRelTypes_nonType = {
    // ** Utilities **
    // pairs and tuples
    {"std::pair", 0}, {"std::tuple", 0},
    // bitset
    {"std::bitset", 1},
    // smart pointers
    {"std::unique_ptr", 0}, {"std::shared_ptr", 0}, {"std::weak_ptr", 0}
};

const StringMap<StringMap<int>> NumRelTypes = {
    {"type", NumRelTypes_type},
    {"non-type", NumRelTypes_nonType},
    {"template", {}}
};

} // namespace


// Gathers data on how often each utility template was used.
void utilityPrevalence(const ordered_json& in, ordered_json& out){
    templatePrevalence(in, out);
}

// For each container template, gives statistics on how often each instantiation
// was used by a (member) variable.
void utilitytemplateTypeArgPrevalence(const ordered_json& in, ordered_json& out){
    templateTypeArgPrevalence(in, out, NumRelTypes);
}

void UtilityLibAnalysis::processFeatures(const ordered_json& j){
    if(j.contains(ImplicitClassKey)){
        ordered_json res1;
        ordered_json res2;
        utilityPrevalence(j.at(ImplicitClassKey), res1);
        utilitytemplateTypeArgPrevalence(j.at(ImplicitClassKey), res2);
        Statistics[UtilityPrevalenceKey] = res1;
        Statistics[UtilitiedTypesPrevalenceKey] = res2;
    }

    if(j.contains(ExplicitClassKey)){
        ordered_json res1;
        ordered_json res2;
        utilityPrevalence(j.at(ExplicitClassKey), res1);
        utilitytemplateTypeArgPrevalence(j.at(ExplicitClassKey), res2);
        Statistics[UtilityPrevalenceKey] = add(std::move(Statistics[UtilityPrevalenceKey]), res1);
        Statistics[UtilitiedTypesPrevalenceKey] = add(std::move(Statistics[UtilitiedTypesPrevalenceKey]), res2);
    }
}

//-----------------------------------------------------------------------------
