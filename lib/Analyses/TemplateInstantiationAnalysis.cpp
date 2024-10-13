#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <utility>
#include <type_traits>
#include <optional>

#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/ADT/StringRef.h"
#include "clang/AST/TemplateBase.h"

#include <nlohmann/json.hpp>

#include "cxx-langstat/Analysis.h"
#include "cxx-langstat/Analyses/TemplateInstantiationAnalysis.h"
#include "cxx-langstat/Utils.h"

using namespace clang;
using namespace clang::ast_matchers;
using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;
using StringRef = llvm::StringRef;

//-----------------------------------------------------------------------------
// Compute statistics on arguments used to instantiate templates,
// no matter whether instantiation are explicit or implicit.
// Should be divided into 3 categories:
// - classes,
// - functions (including member methods), &
// - variables (including class static member variables, (not
//   class field, those cannot be templated if they're not static))
//
// Template instantiations counted should stem from either explicit instantiations
// written by programmers or from implicit ones through 'natural usage'.
//
// Remember that template parameters can be non-types, types or templates.
// Goal: for each instantiation report:
// - classes:
//   * report every explicit instantiation
//   * report every "implicit instantiation", e.g. report ALL instances of
//     std::vector<int>, notably when that occurs multiple times
// - functions:
//   * report every call of f<t> for some instantiating arguments.
// - variables: Report for each var<t> the instantiation, no matter how often
//   that happened.

TemplateInstantiationAnalysis::TemplateInstantiationAnalysis() :
    TemplateInstantiationAnalysis(InstKind::Any, anything(), ".*") {
}

TemplateInstantiationAnalysis::TemplateInstantiationAnalysis(InstKind IK,
    internal::Matcher<clang::NamedDecl> Names, std::string HeaderRegex) :
    IK(IK), Names(Names), HeaderRegex(HeaderRegex) {
        std::cout << "TIA ctor\n";
}

// test code for instantiations:
// - Test code to see if able to find "subinstantiations"
// classTemplateSpecializationDecl(
//     Names,
//     isTemplateInstantiation(),
//     unless(has(cxxRecordDecl())))
// .bind("ImplicitCTSD"),
// - If variable is inside of a template, then it has to be the
// case that the template is being instantiated
// This doesn't work transitively
// anyOf(unless(hasAncestor(functionTemplateDecl())), isInstantiated()),
// - A variable of CTSD type can be matched by this matcher. For
// some reason if it is, it will match twice. Either you uncomment
// the line down below and disallow those variable instantiations
// to be matched here, or you filter out the duplicates below.
// See VarTemplateInstClass.cpp for insights.
// unless(isTemplateInstantiation())

void TemplateInstantiationAnalysis::extractFeatures() {
    // Result of the class insts matcher will give back a pointer to the
    // ClassTemplateSpecializationDecl (CTSD).
    if(IK == InstKind::Class || IK == InstKind::Any){
        // Want variable that has type of some class instantiation,
        // class name is restricted to come from 'Names'
        
        auto CTSD = classTemplateSpecializationDecl(hasSpecializedTemplate(
            classTemplateDecl(
                has(cxxRecordDecl(isDefinition())),
                // isDefinition(),
                Names,
                isExpansionInFileMatching(HeaderRegex)
            )
        )).bind("all-CTSD");
        
        auto type = hasUnqualifiedDesugaredType(recordType(hasDeclaration(CTSD)));
        auto type_matcher = hasType(recursiveTypeMatcher(type));

        auto TemplatedClassUsesMatcher = decl(anyOf(
            // -- Implicit --
            // Implicit uses:
            // Variable declarations (which include function parameter variables
            // & static member variables)
            varDecl(
                isExpansionInHomeDirectory(),
                unless(isExpansionInSystemHeader()),
                type_matcher,
                unless(hasParent(decl(isImplicit()))) // implicitly generated constructors for templated classes
            ).bind("DeclaratorDeclThatUseTemplatedClass"),

            // Field declarations (non static variable member)
            fieldDecl(
                isExpansionInHomeDirectory(),
                unless(isExpansionInSystemHeader()),
                type_matcher
            ).bind("DeclaratorDeclThatUseTemplatedClass"),

            // Function return types
            functionDecl(
                returns(recursiveTypeMatcher(type)),
                unless(isImplicit()),
                unless(isExpansionInSystemHeader()),
                isExpansionInHomeDirectory()
            ).bind("DeclaratorDeclThatUseTemplatedClass"),


            // -- Explicit --
            // Explicit instantiations that are not explicit specializations,
            // which is ensured by isTemplateInstantiation() according to
            // matcher reference
            classTemplateSpecializationDecl(
                Names,
                unless(isExpansionInSystemHeader()),
                isExpansionInHomeDirectory(),
                // should not be stored where classtemplate is stored,
                // because there the implicit instantiations are usually put
                unless(hasParent(classTemplateDecl())),
                isTemplateInstantiation()
            ).bind("explicit-CTSD")
        ));

        // detecting expressions and new-expressions
        auto TemplatedClassUsesMatcher_expr = expr(
            anyOf(cxxTemporaryObjectExpr(), cxxNewExpr()),
            type_matcher,
            unless(isExpansionInSystemHeader()),
            isExpansionInHomeDirectory()
        ).bind("CTSD-expr");
        
        auto TemplatedClassResults = Extractor.extract2(*Context, TemplatedClassUsesMatcher);

        ClassExplicitInsts = getASTNodes<ClassTemplateSpecializationDecl>(TemplatedClassResults, "explicit-CTSD");
        TemplatedClassUses = getASTNodes<DeclaratorDecl>(TemplatedClassResults, "DeclaratorDeclThatUseTemplatedClass");
        TemplatedClassUses_expr = getASTNodes<Expr>(Extractor.extract2(*Context, TemplatedClassUsesMatcher_expr), "CTSD-expr");

        // only really needed to find location of where class was implicitly instantiated
        // using variable of member variable/field
        // Variables = getASTNodes<DeclaratorDecl>(TemplatedClassResults,
        //     "DeclaratorDeclThatUseTemplatedClass");

        // No doubt, this is a botch. If some class type was instantiated through
        // a variable templates instantiation, which will be matched twice, the loop
        // below will filter out those variable declarations and the belonging
        // implicit class inst with it in O(n^2). A better solution would be to
        // bundle variables with the class template specialization that is their type
        // and sort (O(nlogn)), but for sake of ease this is (still) omitted.
        // That they're matched twice is due to an bug in RecursiveASTVisitor:
        // https://lists.llvm.org/pipermail/cfe-dev/2021-February/067595.html
        std::unordered_map<std::string, int> VarTemplateInstantiationLocs;
        for (unsigned i = 0; i < TemplatedClassUses.size(); i++){
            auto& it = TemplatedClassUses[i];
            if (auto* VTSD = dyn_cast<VarTemplateSpecializationDecl>(it.Node)){
                const clang::SourceLocation& loc = VTSD->getPointOfInstantiation();

                if (isSourceLocationInHomeDirectory(loc, Context->getSourceManager())){
                    if (VarTemplateInstantiationLocs.count(loc.printToString(Context->getSourceManager()))){
                        TemplatedClassUses.erase(TemplatedClassUses.begin() + i);
                        i--; // unsigned is defined behaviour for underflow, so this is ok. this is to offset the i++ in the for loop
                    }
                    else {
                        VarTemplateInstantiationLocs[loc.printToString(Context->getSourceManager())] = 1;
                    }
                }
            }
        }
    }

    //
    if(IK == InstKind::Function || IK == InstKind::Any){
        // NEW APPROACH: why should we only capture instantiations made by calls?
        // Any valid template instantiation, explicit or implicit, produces a DeclRefExpr node in the AST
        // Should be caught by the following matcher.
        auto ImplFuncInstMatcher = declRefExpr(
            to(functionDecl(
                Names,
                anyOf(
                    allOf(
                        isTemplateInstantiation(), // implicit instantiation
                        isExpansionInFileMatching(HeaderRegex)
                    ),
                    allOf(
                        isExplicitTemplateSpecialization(), //explicit specialization show up as in user file
                        anyOf(
                            isExpansionInFileMatching(HeaderRegex),
                            allOf(unless(isExpansionInSystemHeader()), isExpansionInMainFile())
                        )
                    )
                )
                // isTemplateInstantiation(),
                // isExpansionInFileMatching(HeaderRegex)
            ).bind("ImplicitFuncInsts")), 
            isExpansionInHomeDirectory(), 
            unless(isExpansionInSystemHeader())
        ).bind("callers");

        auto ImplFuncResults = Extractor.extract2(*Context, ImplFuncInstMatcher);
        ImplFuncInsts = getASTNodes<FunctionDecl>(ImplFuncResults, "ImplicitFuncInsts");
        Callers = getASTNodes<DeclRefExpr>(ImplFuncResults, "callers");

        // Explicit function template instantiations rely on matching functionDecls
        // that are a template instantiation, getting the location of that instantiation,
        // and making sure that it is in a user source file.
        auto ExplFuncInstMatcher = functionDecl(
            Names,
            isTemplateInstantiation()
            // not using the one below because a specialization is not an instantiation
            // anyOf(
            //     isTemplateInstantiation(),
            //     isExplicitTemplateSpecialization()
            // )
            // isExpansionInFileMatching(HeaderRegex)
        ).bind("ExplicitFuncInsts");
        
        auto ExplFuncInst_temp = getASTNodes<FunctionDecl>(Extractor.extract2(*Context, ExplFuncInstMatcher), "ExplicitFuncInsts");

        // std::cout << "ExplFuncInst_temp.size() = " << ExplFuncInst_temp.size() << std::endl;

        for (auto& it: ExplFuncInst_temp){
            if (it.Node->getTemplateSpecializationInfo() && it.Node->getTemplateSpecializationInfo()->isExplicitInstantiationOrSpecialization()){
                const clang::FunctionTemplateDecl* func_template_decl = it.Node->getPrimaryTemplate();

                if (!func_template_decl){
                    // std::cout << "Skipping " << it.Node->getNameAsString() << " because it has no primary template" << std::endl;
                    continue;
                }
                if (!isSourceLocationInFileMatching(func_template_decl->getLocation(), Context->getSourceManager(), HeaderRegex)){
                    // std::cout << "Skipping " << it.Node->getNameAsString() << " because the func template decl it refers to is not in the specified header regex" << std::endl;
                    // std::cout << "it is in location: " << Context->getFullLoc(func_template_decl->getLocation()).printToString(Context->getSourceManager()) << std::endl;
                    continue;
                }
                
                clang::SourceLocation source_location = it.Node->getTemplateSpecializationInfo()->getPointOfInstantiation();
                if (isSourceLocationInHomeDirectory(source_location, Context->getSourceManager())){
                    ExplFuncInsts.push_back(it);
                    Instantiators.emplace_back(Context->getFullLoc(source_location).getLineNumber(), Context->getFullLoc(source_location).printToString(Context->getSourceManager()));
                }
                else {
                    // std::cout << "Skipping " << it.Node->getNameAsString() << " because the point of instantiation is not in the home directory" << std::endl;
                }
            }
            else {
                // std::cout << "Skipping " << it.Node->getNameAsString() << " because couldn't get TST info or is not explicit instantiation" << std::endl;
            }
        }

        // std::cout << "ExplFuncInsts.size(): " << ExplFuncInsts.size() << std::endl;
    }

    // Same behavior as with classTemplates: gives pointer to a
    // varSpecializationDecl. However, the location reported is that of the
    // varDecl itself... no matter if explicit or implicit instantiation.
    if(IK == InstKind::Variable || IK == InstKind::Any){
        internal::VariadicDynCastAllOfMatcher<Decl, VarTemplateSpecializationDecl>
            varTemplateSpecializationDecl;
        auto VarInstMatcher = varTemplateSpecializationDecl(
            isExpansionInHomeDirectory(),
            unless(isExpansionInSystemHeader()),
            // isExpansionInMainFile(),
            isTemplateInstantiation())
        .bind("VarInsts");
        auto VarResults = Extractor.extract2(*Context, VarInstMatcher);
        VarInsts = getASTNodes<VarTemplateSpecializationDecl>(VarResults,
            "VarInsts");
        if(VarInsts.size())
            removeDuplicateMatches(VarInsts);
    }
}

// Overloaded function to get template arguments depending whether it's a class
// function, or variable.
const TemplateArgumentList*
getTemplateArgs(const Match<ClassTemplateSpecializationDecl>& Match){
    return &(Match.Node->getTemplateInstantiationArgs());
}
const TemplateArgumentList*
getTemplateArgs(const Match<VarTemplateSpecializationDecl>& Match){
    return &(Match.Node->getTemplateInstantiationArgs());
}
const TemplateArgumentList*
getTemplateArgs(const Match<DeclaratorDecl>& Match){
    clang::QualType qualified_type;

    if (const clang::FunctionDecl *FD = dyn_cast<clang::FunctionDecl>(Match.Node)){
        qualified_type = FD->getReturnType();
    }
    else {
        qualified_type = Match.Node->getType();
    }
    
    qualified_type = removePointersAndReference(qualified_type);

    if (const TemplateSpecializationType *TST = qualified_type->getAs<clang::TemplateSpecializationType>()){
        if (const ClassTemplateSpecializationDecl *CTSD = dyn_cast<ClassTemplateSpecializationDecl>(TST->getAsRecordDecl())) {
            return &(CTSD->getTemplateInstantiationArgs());
        }
    }
    // assert(false);
    return nullptr;
}
const TemplateArgumentList*
getTemplateArgs(const Match<Expr>& Match){
    clang::QualType qualified_type = removePointersAndReference(Match.Node->getType());
    if (const TemplateSpecializationType *TST = qualified_type->getAs<clang::TemplateSpecializationType>()){
        if (const ClassTemplateSpecializationDecl *CTSD = dyn_cast<ClassTemplateSpecializationDecl>(TST->getAsRecordDecl())) {
            return &(CTSD->getTemplateInstantiationArgs());
        }
    }
    // assert(false);
    return nullptr;
}
// Is this memory-safe?
// Probably yes, assuming the template arguments being stored on the heap,
// being freed only later by the clang library.
const TemplateArgumentList*
getTemplateArgs(const Match<FunctionDecl>& Match){
    // If Match.Node is a non-templated method of a class template
    // we don't care about its instantiation. Then only the class instantiation
    // encompassing it is really interesting, which is output at a different
    // points in code (and time).
    if(auto m = llvm::dyn_cast<CXXMethodDecl>(Match.Node)){
        if(m->getInstantiatedFromMemberFunction())
            return nullptr;
    }
    auto TALPtr = Match.Node->getTemplateSpecializationArgs();
    if(!TALPtr){
        std::cout << "Template argument list ptr is nullptr,"
        << " function declaration " << getMatchDeclName(Match) << " at line " << Match.Location
        << " was not a template specialization" << '\n';
        // should we exit here, or just continue and ignore this failure?
        // did not occur often in my testing so far.
        // happened once when running TIA on Driver.cpp
    }
    return TALPtr;
}

// Given a mapping from template argumend kind to actual arguments and a given,
// previously unseen argument, check what kind the argument has and add it
// to the mapping.
void updateArgsAndKinds(const TemplateArgument& TArg,
    std::multimap<std::string, std::string>& TArgs, const clang::ASTContext& Context) {
        LangOptions LO;
        PrintingPolicy PP(LO);
        PP.PrintCanonicalTypes = true;
        PP.SuppressTagKeyword = true;
        PP.SuppressScope = false;
        PP.SuppressUnwrittenScope = true;
        PP.FullyQualifiedName = true;
        PP.Bool = true;

        std::string Result;
        llvm::raw_string_ostream stream(Result);
        // std::cout << TArg.isPackExpansion() << std::endl;
        // std::cout << TArg.containsUnexpandedParameterPack() << std::endl;

        switch (TArg.getKind()){
            // ** Type **
            case TemplateArgument::Type:
                TArgs.emplace("type", TArg.getAsType().getAsString(PP));
                break;

            // ** Non-type **
            // Declaration
            case TemplateArgument::Declaration:
                cast<clang::NamedDecl>(TArg.getAsDecl())->printQualifiedName(stream, PP);
                TArgs.emplace("non-type", Result);
                break;
            // nullptr
            // FIXME: report type of nullptr, e.g. nullptr to class XY etc.
            case TemplateArgument::NullPtr:
                TArgs.emplace("non-type", "nullptr");
                break;
            // Integers/Integrals
            case TemplateArgument::Integral:
                // just dump that stuff, don't want to deal with llvm::APSInt
                TArg.dump(stream);
                TArgs.emplace("non-type", Result);
                break;
            case TemplateArgument::StructuralValue:
                TArg.dump(stream);
                TArgs.emplace("non-type", Result);
                break;
            // ** Template **
            // Template
            case TemplateArgument::Template:
                TArg.getAsTemplate().print(stream, PP);
                TArgs.emplace("template", Result);
                break;
            // TemplateExpansion
            // FIXME: what to do in this case
            case TemplateArgument::TemplateExpansion:
                // std::cout << "TemplateExpansion\n";
                TArg.dump(stream);
                TArgs.emplace("template", Result);
                break;
            // Pack
            case TemplateArgument::Pack:
                for(auto it=TArg.pack_begin(); it!=TArg.pack_end(); it++)
                    updateArgsAndKinds(*it, TArgs, Context);
                break;

            // ** Miscellaneous **
            // FIXME: what to do in these cases?
            // Expression
            case TemplateArgument::Expression:
                // std::cout << "expr\n";
                TArg.dump(stream);
                TArgs.emplace("non-type", Result);
                break;
            // Null
            case TemplateArgument::Null:
                // std::cout << "null\n";
                TArg.dump(stream);
                TArgs.emplace("non-type", Result);
                break;
            default:
                TArg.dump(stream);
                TArgs.emplace("non-type", Result);
                break;
        }
}

// Note about the reported locations:
// For explicit instantiations, a 'single' CTSD match in the AST is returned
// which contains info about the correct location.
// For implicit instantiations (i.e. 'natural' usage e.g. through the use
// of variables, fields), the location of the CTSD is also reported. However,
// since that is a subtree of the tree representing the ClassTemplateDecl, we
// have to do some extra work to get the location of where the instantiation
// in the code actually occurred, that is, the line where the programmer wrote
// the variable or the field.
template<>
std::pair<unsigned, std::shared_ptr<std::string> > 
    TemplateInstantiationAnalysis::getInstantiationLocation(
    const Match<ClassTemplateSpecializationDecl>& Match, bool isImplicit){
    if(isImplicit){
        auto location = Context->getFullLoc(Variables[VariablesCounter].Node->getInnerLocStart())
            .getLineNumber();
        auto global_location = std::make_shared<std::string>(Context->getFullLoc(
            Variables[VariablesCounter].Node->getInnerLocStart()).printToString(Context->getSourceManager()));
        VariablesCounter++;
        return {location, global_location};
        // return Context->getFullLoc(Variables[VariablesCounter++].Node->getInnerLocStart())
        //     .getLineNumber();
        // return std::make_shared<std::string>(std::move(Context->getFullLoc(Variables[VariablesCounter++].Node->getInnerLocStart())
            // .printToString(Context->getSourceManager())));
        // can't I just do Variables[i-1].Location to get loc of var/field?
        // std::cout << Variables[i-1].Location << std::endl;
        // return std::to_string(static_cast<int>(Variables[i-1].Location));
    } else{
        auto location = Context->getFullLoc(Match.Node->getTemplateKeywordLoc())
            .getLineNumber();
        auto global_location = std::make_shared<std::string>(Context->getFullLoc(
            Match.Node->getTemplateKeywordLoc()).printToString(Context->getSourceManager()));
        return {location, global_location};
        // return Context->getFullLoc(Match.Node->getTemplateKeywordLoc())
        //     .getLineNumber();
        // return std::make_shared<std::string>(std::move(Context->getFullLoc(Match.Node->getTemplateKeywordLoc())
            // .printToString(Context->getSourceManager())));
        // when giving location of explicit inst, can just give match.Location,
        // since CTSD holds right location already since not subtree of CTD
        // return Match.Location;
    }
}

template<>
std::pair<unsigned, std::shared_ptr<std::string> > 
    TemplateInstantiationAnalysis::getInstantiationLocation(
    const Match<clang::Expr>& Match, bool){

    auto location = Context->getFullLoc(Match.Node->getExprLoc())
        .getLineNumber();
    auto global_location = std::make_shared<std::string>(Context->getFullLoc(
        Match.Node->getExprLoc()).printToString(Context->getSourceManager()));
    return {location, global_location};
}

template<>
std::pair<unsigned, std::shared_ptr<std::string> >
    TemplateInstantiationAnalysis::getInstantiationLocation(
    const Match<FunctionDecl>&, bool isImplicit){
        if(isImplicit){
            auto location = Callers[CallersCounter].Location;
            auto global_location = Callers[CallersCounter].GlobalLocation;
            CallersCounter++;
            // return Callers[CallersCounter++].Location;
            return {location, global_location};
        } else {
            auto location = Instantiators[InstantiatorsCounter].Location;
            auto global_location = Instantiators[InstantiatorsCounter].GlobalLocation;
            InstantiatorsCounter++;
            return {location, global_location};
        }
}

template<>
std::pair<unsigned, std::shared_ptr<std::string> >
    TemplateInstantiationAnalysis::getInstantiationLocation(
    const Match<DeclaratorDecl>& Match, bool){
        auto location = Context->getFullLoc(Match.Node->getLocation())
            .getLineNumber();
        auto global_location = std::make_shared<std::string>(Match.Node->getLocation().printToString(Context->getSourceManager()));
        return {location, global_location};
        // return Context->getFullLoc(Match.Node->getPointOfInstantiation())
        //     .getLineNumber();
        // return std::make_shared<std::string>(std::move(Context->getFullLoc(Match.Node->getPointOfInstantiation())
        //     .printToString(Context->getSourceManager())));
}

template<typename T>
std::pair<unsigned, std::shared_ptr<std::string> >
    TemplateInstantiationAnalysis::getInstantiationLocation(
    const Match<T>& Match, bool){
        auto location = Context->getFullLoc(Match.Node->getPointOfInstantiation())
            .getLineNumber();
        auto global_location = std::make_shared<std::string>(std::move(Context->getFullLoc(
            Match.Node->getPointOfInstantiation()).printToString(Context->getSourceManager())));
        return {location, global_location};
        // return Context->getFullLoc(Match.Node->getPointOfInstantiation())
        //     .getLineNumber();
        // return std::make_shared<std::string>(std::move(Context->getFullLoc(Match.Node->getPointOfInstantiation())
        //     .printToString(Context->getSourceManager())));
}

// Given a vector of matches, create a JSON object storing all instantiations.
template<typename T>
void TemplateInstantiationAnalysis::gatherInstantiationData(Matches<T>& Insts,
    const std::string& InstKind, bool AreImplicit){
    const std::array<std::string, 3> ArgKinds = {"non-type", "type", "template"};
    ordered_json instances;
    for(auto match : Insts){
        LangOptions LO;
        PrintingPolicy PP(LO);
        PP.PrintCanonicalTypes = true;
        PP.SuppressTagKeyword = true;
        PP.SuppressScope = false;
        PP.SuppressUnwrittenScope = true;
        PP.FullyQualifiedName = true;
        PP.Bool = true;
        PP.SuppressTemplateArgsInCXXConstructors = true;

        std::string DeclName;

        if constexpr (std::is_same_v<T, FunctionDecl>){
            // we are interested in the name of the function (object)
            DeclName = match.getIdentifierName();
        }
        else {
            // we are interested in the name of the return type
            DeclName = match.getCleanTypeName(PP);
        }

        std::multimap<std::string, std::string> TArgs;
        const TemplateArgumentList* TALPtr(getTemplateArgs(match));
        // Only report instantiation if it had any arguments it was instantiated
        // with.
        if(TALPtr){
            ordered_json instance;
            ordered_json arguments;

            auto numTArgs = TALPtr->size();
            for(unsigned idx=0; idx<numTArgs; idx++){
                auto TArg = TALPtr->get(idx);
                updateArgsAndKinds(TArg, TArgs, *Context);
            }

            const auto [location, global_location] = getInstantiationLocation(match, AreImplicit);

            instance["Location"] = location;
            instance["GlobalLocation"] = normalizeGlobalLocation(*global_location);

            for(auto key : ArgKinds){
                auto range = TArgs.equal_range(key);
                std::vector<std::string> v;
                for (auto it = range.first; it != range.second; it++)
                    v.emplace_back(it->second);
                arguments[key] = v;
            }
            instance["arguments"] = arguments;
            // Use emplace instead of '=' because can be mult. insts for a template
            instances[DeclName].emplace_back(instance);
        } else {
            // std::cout << DeclName << " had no inst args" << std::endl;
            // FIXME: find more elegant solution
            // No TAL -> skip a function call in reporting -> increase counter
            // to get correct call for each object in FuncInst
            if(InstKind == ImplFuncKey)
                CallersCounter++;
        }
    }
    if (Features.contains(InstKind)){
        // combine the vectors that are found at the same key
        for (auto& [key, value]: instances.items()){
            if (Features[InstKind].contains(key)){
                Features[InstKind][key].insert(Features[InstKind][key].end(), value.begin(), value.end());
            } else {
                Features[InstKind][key] = value;
            }
        }
    } else {
        Features[InstKind] = instances;
    }
}

void TemplateInstantiationAnalysis::analyzeFeatures(){
    extractFeatures();
    if(IK == InstKind::Class || IK == InstKind::Any){
        gatherInstantiationData(ClassExplicitInsts, ExplicitClassKey, false);
        gatherInstantiationData(TemplatedClassUses, ImplicitClassKey, true);
        gatherInstantiationData(TemplatedClassUses_expr, ImplicitClassKey, true);
    }
    if(IK == InstKind::Function || IK == InstKind::Any){
        gatherInstantiationData(ImplFuncInsts, ImplFuncKey, true);
        gatherInstantiationData(ExplFuncInsts, ExplFuncKey, false);
    }
    if(IK == InstKind::Variable || IK == InstKind::Any)
        gatherInstantiationData(VarInsts, VarKey, false);
    
    for (const auto& key: TemplateInstantiationAnalysis::getFeatureKeys()){
        if (!Features.contains(key))
            continue;
        for (const auto& [feature_name, instances]: Features[key].items()){
            std::sort(instances.begin(), instances.end(), 
                [](const nlohmann::json& a, const nlohmann::json& b){
                    return a["Location"] < b["Location"];
                });
        }
    }
}

void TemplateInstantiationAnalysis::processFeatures(const nlohmann::ordered_json& j){
    if(j.contains(ExplicitClassKey)) {
        ordered_json res;
        templatePrevalence(j.at(ExplicitClassKey), res);
        Statistics[ShorthandName + " " + ExplicitClassKey] = res;
    }
    if(j.contains(ImplicitClassKey)) {
        ordered_json res;
        templatePrevalence(j.at(ImplicitClassKey), res);
        Statistics[ShorthandName + " " + ImplicitClassKey] = res;
    }
    if(j.contains(ImplFuncKey)) {
        ordered_json res;
        templatePrevalence(j.at(ImplFuncKey), res);
        Statistics[ShorthandName + " " + ImplFuncKey] = res;
    }
    if(j.contains(ExplFuncKey)) {
        ordered_json res;
        templatePrevalence(j.at(ExplFuncKey), res);
        Statistics[ShorthandName + " " + ExplFuncKey] = res;
    }
}

int getNumRelevantTypes(StringRef Type, const StringMap<int>& SM){
    if(auto [l,r] = Type.split("::__1"); !r.empty())
        return SM.at((l+r).str());
    return SM.at(Type.str());
}

std::string getRelevantTypesAsString(StringRef Type, json Types,
    const StringMap<int>& SM){
        int n = getNumRelevantTypes(Type, SM);
        if(n == -1)
            n = Types.end() - Types.begin();

        std::string t = "";
        // for(nlohmann::json::iterator i = Types.begin(); i < std::min(Types.begin() + n, Types.end()); i++){

        int idx = 0;
        for (auto it = Types.begin(); it != Types.end() && idx < n; ++it, ++idx){
            // std::cout << "Types.begin() = " << Types.begin() << " Types.end() = " << Types.end() << std::endl;
            t = t + (*it).get<std::string>() + ", ";
        }

        if (!t.empty()){
            return llvm::StringRef(t).drop_back(2).str(); // dropping the ", "
        }
        else {
            return "";
        }
}

void templatePrevalence(const ordered_json& in, ordered_json& out){
    std::map<std::string, unsigned> m;
    for(const auto& [Type, Insts] : in.items()){
        m.try_emplace(Type, Insts.size());
    }
    out = m;
}

void templateTypeArgPrevalence(const ordered_json& in, ordered_json& out,
    const StringMap<StringMap<int>>& SM){
        StringMap<StringMap<unsigned>> m;
        for(const auto& [Type, Insts] : in.items()){
            for(const auto& Inst : Insts){
                m.try_emplace(Type, StringMap<unsigned>());

                
                json ContainedTypes = Inst["arguments"]["type"];
                assert(ContainedTypes.is_array());

                auto TypeString = getRelevantTypesAsString(Type, ContainedTypes, SM.at("type"));

                ContainedTypes = Inst["arguments"]["non-type"];
                assert(ContainedTypes.is_array());

                auto TypeString2 = getRelevantTypesAsString(Type, ContainedTypes, SM.at("non-type"));

                if (!TypeString2.empty()){
                    if (TypeString.empty()){
                        std::swap(TypeString, TypeString2);
                    }
                    else {
                        TypeString += ", " + TypeString2;
                    }
                }

                if(!TypeString.empty()){
                    m.at(Type).try_emplace(TypeString, 0);
                    m.at(Type).at(TypeString)++;
                }
            }
        }
        out = m;
}

//-----------------------------------------------------------------------------
