#include <iostream>
#include <vector>

#include "cxx-langstat/Analyses/AlgorithmLibraryAnalysis.h"
#include "cxx-langstat/Utils.h"
#include "cxx-langstat/Stats.h"

using namespace clang;
using namespace clang::ast_matchers;
using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;

//-----------------------------------------------------------------------------
// Construct a ALA by constructing a more constrained TIA.
AlgorithmLibraryAnalysis::AlgorithmLibraryAnalysis() : TemplateInstantiationAnalysis(
    InstKind::Function,
    hasAnyName(
        // https://en.cppreference.com/w/cpp/algorithm
        // Non-modifying sequence ooperations:
        // - Batch operations
        "std::for_each", "std::for_each_n",
        // - Search operations
        "std::all_of", "std::any_of", "std::none_of",
        "std::find", "std::find_if", "std::find_if_not",
        "std::find_end", "std::find_first_of", "std::adjacent_find",
        "std::count", "std::count_if",
        "std::mismatch", "std::equal",
        "std::search", "std::search_n",

        // Modifying sequence operations:
        // - Copy operations
        "std::copy", "std::copy_if", "std::copy_n", "std::copy_backward",
        "std::move", "std::move_backward",
        // - Swap operations
        "std::swap", "std::swap_ranges", "std::iter_swap",
        // - Transformation operations
        "std::transform", "std::replace", "std::replace_if",
        "std::replace_copy", "std::replace_copy_if",
        // - Generation operations
        "std::fill", "std::fill_n", "std::generate", "std::generate_n",
        // - Removing operations
        "std::remove", "std::remove_if",
        "std::remove_copy", "std::remove_copy_if",
        "std::unique", "std::unique_copy",
        // - Order-changing operations
        "std::reverse", "std::reverse_copy",
        "std::rotate", "std::rotate_copy",
        "std::shift_left", "std::shift_right",
        "std::random_shuffle", "std::shuffle",
        // - Sampling operations
        "std::sample",

        // Sorting and related operations:
        // - Partitioning operations
        "std::is_partitioned", "std::partition", "std::partition_copy",
        "std::stable_partition", "std::partition_point",
        // - Sorting operations
        "std::sort", "std::stable_sort",
        "std::partial_sort", "std::partial_sort_copy",
        "std::is_sorted", "std::is_sorted_until",
        "std::nth_element",
        // - Binary search operations (on partitioned ranges)
        "std::lower_bound", "std::upper_bound",
        "std::equal_range", "std::binary_search",
        // - Set operations (on sorted ranges)
        "std::includes", "std::set_union", "std::set_intersection",
        "std::set_difference", "std::set_symmetric_difference",
        // - Merge operations (on sorted ranges)
        "std::merge", "std::inplace_merge",
        // - Heap operations
        "std::push_heap", "std::pop_heap", "std::make_heap",
        "std::sort_heap", "std::is_heap", "std::is_heap_until",
        
        // Minimum/maximum operations
        "std::max", "std::max_element", "std::min", "std::min_element",
        "std::minmax", "std::minmax_element", "std::clamp",
        // - Lexicographical comparison operations
        "std::lexicographical_compare", "std::lexicographical_compare_three_way",
        // - Permutation operations
        "std::next_permutation", "std::prev_permutation", "std::is_permutation"

        // This list of algorithms doesn't contain algorithms on ranges yet
    ),
    // libc++
    "algorithm$|"
    // libstdc++
    // "bits/stl_algo\\.h$|"
    // "bits/stl_algobase\\.h$|"
    // "bits/stl_heap\\.h$|"
    // "bits/ranges_algo\\.h$|"
    // "bits/move\\.h$" // swap is actually found here
    "/bits/.*$" // just include everything, to not worry about auxiliary stdlib system headers
    ){
    std::cout << "ALA ctor\n";
}

// Gathers data on how often each algorithm template was used.
void algorithmPrevalence(const ordered_json& in, ordered_json& out){
    templatePrevalence(in, out);
}

void AlgorithmLibraryAnalysis::analyzeFeatures(){
    TemplateInstantiationAnalysis::analyzeFeatures();
    if (Features.contains(ImplFuncKey) && Features[ImplFuncKey].contains("std::move")){
        for (size_t idx = 0; idx < Features[ImplFuncKey]["std::move"].size(); ){
            auto& instance = Features[ImplFuncKey]["std::move"][idx];
            if (instance["arguments"]["type"].size() != 2 && instance["arguments"]["type"].size() != 3){
                // remove the element from the array
                Features[ImplFuncKey]["std::move"].erase(idx);
            }
            else {
                ++idx;
            }
        }
    }
    if (Features.contains(ExplFuncKey) && Features[ExplFuncKey].contains("std::move")){
        for (size_t idx = 0; idx < Features[ExplFuncKey]["std::move"].size(); ){
            auto& instance = Features[ExplFuncKey]["std::move"][idx];
            if (instance["arguments"]["type"].size() != 2 && instance["arguments"]["type"].size() != 3){
                // remove the element from the array
                Features[ExplFuncKey]["std::move"].erase(idx);
            }
            else {
                ++idx;
            }
        }
    }
}

void AlgorithmLibraryAnalysis::processFeatures(const ordered_json& j){
    ordered_json res_temp;
    ordered_json res;

    if(j.contains(ImplFuncKey)){
        algorithmPrevalence(j.at(ImplFuncKey), res_temp);
    }
    res = add(std::move(res), res_temp);
    if (j.contains(ExplFuncKey)){
        algorithmPrevalence(j.at(ExplFuncKey), res_temp);
    }
    res = add(std::move(res), res_temp);

    Statistics[AlgorithmPrevalenceKey] = res;
}

//-----------------------------------------------------------------------------
