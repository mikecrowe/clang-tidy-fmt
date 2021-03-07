// Declares clang::SyntaxOnlyAction.
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::tooling;
using namespace llvm;

using namespace clang;
using namespace clang::ast_matchers;

//    |-CXXOperatorCallExpr 0x55dda8d48be8 <line:24:5, col:20> 'void' '()'
//    | |-ImplicitCastExpr 0x55dda8d48bb8 <col:10, col:20> 'void (*)(const char *)' <FunctionToPointerDecay>
//    | | `-DeclRefExpr 0x55dda8d48b68 <col:10, col:20> 'void (const char *)' lvalue CXXMethod 0x55dda8d48a68 'operator()' 'void (const char *)'
//    | |-DeclRefExpr 0x55dda8d48848 <col:5> 'NullTrace' lvalue Var 0x55dda8d1ad88 'TRACE' 'NullTrace'
//    | `-ImplicitCastExpr 0x55dda8d48bd0 <col:11> 'const char *' <ArrayToPointerDecay>
//    |   `-StringLiteral 0x55dda8d488e8 <col:11> 'const char [7]' lvalue "Hello\n"

const auto TraceClassExpr = expr(hasType(cxxRecordDecl(hasName("::NullTrace"))));

StatementMatcher TraceMatcher =
    traverse(TK_AsIs,
             cxxOperatorCallExpr(hasOverloadedOperatorName("()"),
                                 hasArgument(0, TraceClassExpr)
                 ).bind("trace"));

StatementMatcher LoopMatcher =
    forStmt(hasLoopInit(declStmt(
                hasSingleDecl(varDecl(hasInitializer(integerLiteral(equals(0))))
                                  .bind("initVarName")))),
            hasIncrement(unaryOperator(
                hasOperatorName("++"),
                hasUnaryOperand(declRefExpr(
                    to(varDecl(hasType(isInteger())).bind("incVarName")))))),
            hasCondition(binaryOperator(
                hasOperatorName("<"),
                hasLHS(ignoringParenImpCasts(declRefExpr(
                    to(varDecl(hasType(isInteger())).bind("condVarName"))))),
                hasRHS(expr(hasType(isInteger())))))).bind("forLoop");

class TracePrinter : public MatchFinder::MatchCallback {
public :
    virtual void run(const MatchFinder::MatchResult &Result) override;
};

#if 0
static bool areSameVariable(const ValueDecl *First, const ValueDecl *Second) {
  return First && Second &&
         First->getCanonicalDecl() == Second->getCanonicalDecl();
}

static bool areSameExpr(ASTContext *Context, const Expr *First,
                        const Expr *Second) {
  if (!First || !Second)
    return false;
  llvm::FoldingSetNodeID FirstID, SecondID;
  First->Profile(FirstID, *Context, true);
  Second->Profile(SecondID, *Context, true);
  return FirstID == SecondID;
}
#endif

void TracePrinter::run(const MatchFinder::MatchResult &Result) {
    llvm::outs() << "Hello\n";
#if 0
  ASTContext *Context = Result.Context;
  const ForStmt *FS = Result.Nodes.getNodeAs<ForStmt>("forLoop");
  // We do not want to convert header files!
  if (!FS || !Context->getSourceManager().isWrittenInMainFile(FS->getForLoc()))
    return;
  const VarDecl *IncVar = Result.Nodes.getNodeAs<VarDecl>("incVarName");
  const VarDecl *CondVar = Result.Nodes.getNodeAs<VarDecl>("condVarName");
  const VarDecl *InitVar = Result.Nodes.getNodeAs<VarDecl>("initVarName");

  if (!areSameVariable(IncVar, CondVar) || !areSameVariable(IncVar, InitVar))
    return;
  llvm::outs() << "Potential array-based loop discovered.\n";
#endif
}

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory MyToolCategory("my-tool options");

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static cl::extrahelp MoreHelp("\nMore help text...\n");

int main(int argc, const char **argv) {
  auto ExpectedParser = CommonOptionsParser::create(argc, argv, MyToolCategory);
  if (!ExpectedParser) {
    // Fail gracefully for unsupported options.
    llvm::errs() << ExpectedParser.takeError();
    return 1;
  }
  CommonOptionsParser& OptionsParser = ExpectedParser.get();
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());

  TracePrinter Printer;
  MatchFinder Finder;
  Finder.addMatcher(TraceMatcher, &Printer);

  return Tool.run(newFrontendActionFactory(&Finder).get());
}
