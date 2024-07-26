// RUN: %clangxx %s -o %t1.o
// RUN: %t1.o

#include <cassert>
#include <filesystem>
#include <iostream>
#include <string>
#include <regex>

std::string normalizeGlobalLocation(std::string&& GlobalLocation){
    std::string Result;
    std::string::size_type idx = GlobalLocation.find(" <Spelling=");
    if(idx != std::string::npos){
        std::string path = GlobalLocation.substr(0, idx);
        std::string spelling_path = GlobalLocation.substr(idx + strlen(" <Spelling="), GlobalLocation.find(">", idx) - idx - strlen(" <Spelling="));

        Result = std::filesystem::weakly_canonical(path).string() + " <Spelling=" + std::filesystem::weakly_canonical(spelling_path).string() + ">";
    } else {
        Result = std::filesystem::weakly_canonical(GlobalLocation).string();
    }
    return Result;
}

std::string normalizeGlobalLocation(const std::string& GlobalLocation){
    std::string Result;
    std::string::size_type idx = GlobalLocation.find(" <Spelling=");
    if(idx != std::string::npos){
        std::string path = GlobalLocation.substr(0, idx);
        std::string spelling_path = GlobalLocation.substr(idx + strlen(" <Spelling="), GlobalLocation.find(">", idx) - idx - strlen(" <Spelling="));
        std::cout << "Spelling path is: " << spelling_path << "\n";

        Result = std::filesystem::weakly_canonical(path).string() + " <Spelling=" + std::filesystem::weakly_canonical(spelling_path).string() + ">";
    } else {
        Result = std::filesystem::weakly_canonical(GlobalLocation).string();
    }
    return Result;
}

int main(){
    std::string path1 = "/a/../b";
    std::string path1_normalized = "/b";
    std::string path2 = "/a/../b <Spelling=/c/../d>";
    std::string path2_normalized = "/b <Spelling=/d>";

    std::cout << normalizeGlobalLocation(path2) << std::endl;

    assert(normalizeGlobalLocation(std::move(path1)) == path1_normalized);
    assert(normalizeGlobalLocation(std::move(path2)) == path2_normalized);

    return 0;
}