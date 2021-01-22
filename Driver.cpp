// helpful: https://github.com/peter-can-talk/cppnow-2017/blob/master/code/mccabe/mccabe.cpp

// clang includes
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Path.h"
// standard includes
#include <iostream> // should be removed
#include <fstream> // file stream
// JSON library
#include <nlohmann/json.hpp>
//
#include "cxx-langstat/AnalysisRegistry.h"
#include "cxx-langstat/Utils.h"
#include "cxx-langstat/Options.h"
#include "../Driver.h"

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;
using StringRef = llvm::StringRef;
using ASTContext = clang::ASTContext;
using ordered_json = nlohmann::ordered_json;

//-----------------------------------------------------------------------------
// Consumes the AST, i.e. does computations on it
class Consumer : public ASTConsumer {
public:
    Consumer(StringRef InFile, AnalysisRegistry* Registry) :
        InFile(InFile),
        Registry(Registry){
    }
    // Called when AST for TU is ready/has been parsed
    void HandleTranslationUnit(ASTContext& Context){
        std::cout << "Handling the translation unit" << std::endl;
        ordered_json AllAnalysesFeatures;
        Stage Stage = Registry->Options.Stage;
        int AnalysisIndex=0;
        for(const auto& an : Registry->Analyses){ // ref to unique_ptr bad?
            auto AnalysisAbbreviation = Registry
                ->Options.EnabledAnalyses.Items[AnalysisIndex].Name.str();
            //
            if(Stage != emit_statistics){
                // Analyze clang AST and extract features
                an->run(InFile, Context);
                AllAnalysesFeatures[AnalysisAbbreviation]=an->getFeatures();
            }
            // process features from json (not from disk)
            if(Stage == none){
                an->processFeatures(AllAnalysesFeatures[AnalysisAbbreviation]);
                /// FIXME: actually write statistics to disk
            }
            AnalysisIndex++;
        }
        // Write to file if -emit-features is active
        if(Stage == emit_features){
            auto OutputFile = Registry->Options.OutputFiles[FileIndex];
            std::ofstream o(OutputFile);
            o << AllAnalysesFeatures.dump(4) << '\n';
            FileIndex++;
        }
        std::cout << "all features:" << AllAnalysesFeatures.dump(4) << std::endl;
    }
public:
    StringRef InFile;
    AnalysisRegistry* Registry;
    static unsigned FileIndex;
};
unsigned Consumer::FileIndex=0;

// Responsible for steering when what is executed
class Action : public ASTFrontendAction {
public:
    Action(AnalysisRegistry* Registry) : Registry(Registry){
        std::cout << "Creating AST Action" << std::endl;
    }
    // Called at start of processing a single input
    bool BeginSourceFileAction(CompilerInstance& CI) {
        std::cout
        << "Starting to process " << getCurrentFile().str()
        << ". AST=" << isCurrentFileAST() << ".\n";
        return true;
    }
    // Called after frontend is initialized, but before per-file processing
    virtual std::unique_ptr<ASTConsumer> CreateASTConsumer(
        CompilerInstance &CI, StringRef InFile){
            std::cout << "Creating AST Consumer" << std::endl;
            return std::make_unique<Consumer>(getCurrentFile(), Registry);
    }
    //
    void EndSourceFileAction(){
        std::cout
        << "Finished processing " << getCurrentFile().str() << ".\n";
    }
    AnalysisRegistry* Registry;
};

// Responsible for building Actions
class Factory : public clang::tooling::FrontendActionFactory {
public:
    // ctor
    Factory(AnalysisRegistry* Reg) : Registry(Reg){
    }
    //
    std::unique_ptr<FrontendAction> create() override {
        return std::make_unique<Action>(Registry);
    }
    AnalysisRegistry* Registry;
};

//-----------------------------------------------------------------------------

// // new means to give analyses, still inactive
// llvm::cl::list<AnalysisType> AnalysesList (
//     llvm::cl::desc("available analyses"),
//     llvm::cl::values(
//         clEnumVal(cca, "cyclo")
//     ),
//     llvm::cl::cat(CXXLangstatCategory));
// llvm::cl::alias CCAAlias("cyclo", llvm::cl::desc(" a"), llvm::cl::aliasopt(AnalysesList));

//-----------------------------------------------------------------------------

int CXXLangstatMain(std::vector<std::string> InputFilesOption, std::vector<std::string> OutputFilesOption,
    Stage PipelineStage, std::string AnalysesOption, std::string BuildPath, std::unique_ptr<CompilationDatabase> db){

    // std::unique_ptr<CompilationDatabase> db = nullptr;

    const std::vector<std::string>& spl = InputFilesOption;
    std::vector<std::string>& OutputFiles = OutputFilesOption;

    if(PipelineStage == emit_features){
        // No output files specified -> store at working dir
        // If specified, check that #input files = #output files
        if(OutputFiles.size()==0){
            for(const auto& InputFile : spl){
                StringRef filename = llvm::sys::path::filename(InputFile);
                filename.consume_back(llvm::sys::path::extension(filename)); // use replace_extension
                OutputFiles.emplace_back("./" + filename.str() + ".features.json");
            }
        } else if(OutputFiles.size()>0 && OutputFiles.size()!=spl.size()){
            std::cout << "#Source files != #Output files, quitting..\n";
            exit(1);
        }
    // When -emit-features option is not used, zero or one output file is ok.
    } else {
        if(OutputFiles.size()==0){
            OutputFiles.emplace_back("./stats.json");
        }
        if(OutputFiles.size()>1){
            std::cout << "Can only specify multiple output files with -emit-features, quitting..\n";
            exit(1);
        }
    }

    // Create custom options object for registry
    CXXLangstatOptions Opts(PipelineStage, OutputFiles, AnalysesOption);
    AnalysisRegistry* Registry = new AnalysisRegistry(Opts);

    if(PipelineStage != emit_statistics){
        // https://clang.llvm.org/doxygen/CommonOptionsParser_8cpp_source.html @ 109
        // Read in database found in dir specified by -p or a parent path
        std::string ErrorMessage;
        if(!BuildPath.empty()){
            std::cout << "READING BUILD PATH\n";
            std::cout << "BUILD PATH: " << BuildPath << std::endl;
            db = CompilationDatabase::autoDetectFromDirectory(BuildPath, ErrorMessage);
            if(!ErrorMessage.empty()){
                std::cout << ErrorMessage << std::endl;
                exit(1);
            }
            std::cout << "FOUND COMPILE COMMANDS:" << std::endl;
            for(auto cc : db->getAllCompileCommands()){
                for(auto s : cc.CommandLine){
                    std::cout << s << " ";
                }
                std::cout << std::endl;
            }
        }
        if(db){
            ClangTool Tool(*db, spl);
            // Tool is run for every file specified in source path list
            Tool.run(std::make_unique<Factory>(Registry).get());
        } else {
            std::cout << "The tool couldn't run due to missing compilation database "
                "Please try one of the following:\n"
                " When analyzing .ast files, append \"--\", to indicate no CDB is necessary\n"
                " When analyzing .cpp files, append \"-- <compile flags>\" or \"-p <build path>\"\n";
        }
    }

    // Process features stored on disk to statistics
    else if(PipelineStage == emit_statistics){
        std::cout << "do because stage 2" << std::endl;
        ordered_json AllFilesAllStatistics;
        for(auto File : spl){
            ordered_json j;
            std::ifstream i(File);
            i >> j;
            auto AnalysisIndex = 0;
            ordered_json OneFileAllStatistics;
            for(const auto& an : Registry->Analyses){ // ref to unique_ptr bad?
                auto AnalysisAbbreviation = Registry
                    ->Options.EnabledAnalyses.Items[AnalysisIndex].Name.str();
                an->processFeatures(j[AnalysisAbbreviation]);
                for(const auto& [statdesc, stats] : an->getStatistics().items()){
                    OneFileAllStatistics[statdesc] = stats;
                }
                AnalysisIndex++;
            }
            AllFilesAllStatistics[File] = OneFileAllStatistics;
        }
        std::ofstream o(Registry->Options.OutputFiles[0]);
        o << AllFilesAllStatistics.dump(4) << std::endl;
        std::cout << "all stats\n" << AllFilesAllStatistics.dump(4) << std::endl;
    }

    // Not really important here, but good practice
    delete Registry;
    std::cout << "Reached end of program" << std::endl;
    return 0;
}
