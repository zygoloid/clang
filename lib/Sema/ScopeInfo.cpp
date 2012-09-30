//===--- ScopeInfo.cpp - Information about a semantic context -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements FunctionScopeInfo and its subclasses, which contain
// information about a single function, block, lambda, or method body.
//
//===----------------------------------------------------------------------===//

#include "clang/Sema/ScopeInfo.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclObjC.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/ExprObjC.h"

using namespace clang;
using namespace sema;

void FunctionScopeInfo::Clear() {
  HasBranchProtectedScope = false;
  HasBranchIntoScope = false;
  HasIndirectGoto = false;

  SwitchStack.clear();
  Returns.clear();
  ErrorTrap.reset();
  PossiblyUnreachableDiags.clear();
  WeakObjectUses.clear();
}

static const NamedDecl *getBestPropertyDecl(const ObjCPropertyRefExpr *PropE) {
  if (PropE->isExplicitProperty())
    return PropE->getExplicitProperty();

  return PropE->getImplicitPropertyGetter();
}

static bool isSelfExpr(const Expr *E) {
  E = E->IgnoreParenImpCasts();

  const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(E);
  if (!DRE)
    return false;

  const ImplicitParamDecl *Param = dyn_cast<ImplicitParamDecl>(DRE->getDecl());
  if (!Param)
    return false;

  const ObjCMethodDecl *M = dyn_cast<ObjCMethodDecl>(Param->getDeclContext());
  if (!M)
    return false;

  return M->getSelfDecl() == Param;
}

FunctionScopeInfo::WeakObjectProfileTy::BaseInfoTy
FunctionScopeInfo::WeakObjectProfileTy::getBaseInfo(const Expr *E) {
  E = E->IgnoreParenCasts();

  const NamedDecl *D = 0;
  bool IsExact = false;

  switch (E->getStmtClass()) {
  case Stmt::DeclRefExprClass:
    D = cast<DeclRefExpr>(E)->getDecl();
    IsExact = isa<VarDecl>(D);
    break;
  case Stmt::MemberExprClass: {
    const MemberExpr *ME = cast<MemberExpr>(E);
    D = ME->getMemberDecl();
    IsExact = isa<CXXThisExpr>(ME->getBase()->IgnoreParenImpCasts());
    break;
  }
  case Stmt::ObjCIvarRefExprClass: {
    const ObjCIvarRefExpr *IE = cast<ObjCIvarRefExpr>(E);
    D = IE->getDecl();
    IsExact = isSelfExpr(IE->getBase());
    break;
  }
  case Stmt::PseudoObjectExprClass: {
    const PseudoObjectExpr *POE = cast<PseudoObjectExpr>(E);
    const ObjCPropertyRefExpr *BaseProp =
      dyn_cast<ObjCPropertyRefExpr>(POE->getSyntacticForm());
    if (BaseProp) {
      D = getBestPropertyDecl(BaseProp);

      const Expr *DoubleBase = BaseProp->getBase();
      if (const OpaqueValueExpr *OVE = dyn_cast<OpaqueValueExpr>(DoubleBase))
        DoubleBase = OVE->getSourceExpr();

      IsExact = isSelfExpr(DoubleBase);
    }
    break;
  }
  default:
    break;
  }

  return BaseInfoTy(D, IsExact);
}


FunctionScopeInfo::WeakObjectProfileTy::WeakObjectProfileTy(
                                          const ObjCPropertyRefExpr *PropE)
    : Base(0, true), Property(getBestPropertyDecl(PropE)) {

  if (PropE->isObjectReceiver()) {
    const OpaqueValueExpr *OVE = cast<OpaqueValueExpr>(PropE->getBase());
    const Expr *E = OVE->getSourceExpr();
    Base = getBaseInfo(E);
  } else if (PropE->isClassReceiver()) {
    Base.setPointer(PropE->getClassReceiver());
  } else {
    assert(PropE->isSuperReceiver());
  }
}

FunctionScopeInfo::WeakObjectProfileTy::WeakObjectProfileTy(
                                                      const DeclRefExpr *DRE)
  : Base(0, true), Property(DRE->getDecl()) {
  assert(isa<VarDecl>(Property));
}

FunctionScopeInfo::WeakObjectProfileTy::WeakObjectProfileTy(
                                                  const ObjCIvarRefExpr *IvarE)
  : Base(getBaseInfo(IvarE->getBase())), Property(IvarE->getDecl()) {
}

void FunctionScopeInfo::markSafeWeakUse(const Expr *E) {
  E = E->IgnoreParenImpCasts();

  if (const PseudoObjectExpr *POE = dyn_cast<PseudoObjectExpr>(E)) {
    markSafeWeakUse(POE->getSyntacticForm());
    return;
  }

  if (const ConditionalOperator *Cond = dyn_cast<ConditionalOperator>(E)) {
    markSafeWeakUse(Cond->getTrueExpr());
    markSafeWeakUse(Cond->getFalseExpr());
    return;
  }

  if (const BinaryConditionalOperator *Cond =
        dyn_cast<BinaryConditionalOperator>(E)) {
    markSafeWeakUse(Cond->getCommon());
    markSafeWeakUse(Cond->getFalseExpr());
    return;
  }

  // Has this weak object been seen before?
  FunctionScopeInfo::WeakObjectUseMap::iterator Uses;
  if (const ObjCPropertyRefExpr *RefExpr = dyn_cast<ObjCPropertyRefExpr>(E))
    Uses = WeakObjectUses.find(FunctionScopeInfo::WeakObjectProfileTy(RefExpr));
  else if (const ObjCIvarRefExpr *IvarE = dyn_cast<ObjCIvarRefExpr>(E))
    Uses = WeakObjectUses.find(FunctionScopeInfo::WeakObjectProfileTy(IvarE));
  else if (const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(E))
    Uses = WeakObjectUses.find(FunctionScopeInfo::WeakObjectProfileTy(DRE));
  else
    return;

  if (Uses == WeakObjectUses.end())
    return;

  // Has there been a read from the object using this Expr?
  FunctionScopeInfo::WeakUseVector::reverse_iterator ThisUse =
    std::find(Uses->second.rbegin(), Uses->second.rend(), WeakUseTy(E, true));
  if (ThisUse == Uses->second.rend())
    return;

  ThisUse->markSafe();
}

FunctionScopeInfo::~FunctionScopeInfo() { }
BlockScopeInfo::~BlockScopeInfo() { }
LambdaScopeInfo::~LambdaScopeInfo() { }
