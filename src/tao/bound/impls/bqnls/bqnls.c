#include <../src/tao/bound/impls/bqnk/bqnk.h>

static PetscErrorCode TaoBQNLSComputeHessian(Tao tao)
{
  TAO_BNK        *bnk = (TAO_BNK *)tao->data;
  TAO_BQNK       *bqnk = (TAO_BQNK*)bnk->ctx;
  PetscErrorCode ierr;
  PetscReal      gnorm2, delta;
  
  PetscFunctionBegin;
  gnorm2 = bnk->gnorm*bnk->gnorm;
  delta = 2.0 * PetscMax(1.0, PetscAbsScalar(bnk->f)) / PetscMax(gnorm2, PetscPowReal(PETSC_MACHINE_EPSILON, 2.0/3.0));
  ierr = MatSymBrdnSetDelta(bqnk->B, delta);CHKERRQ(ierr);
  ierr = MatLMVMUpdate(bqnk->B, tao->solution, bnk->unprojected_gradient);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

static PetscErrorCode TaoBQNLSComputeStep(Tao tao, PetscBool shift, KSPConvergedReason *ksp_reason, PetscInt *step_type)
{
  TAO_BNK        *bnk = (TAO_BNK *)tao->data;
  TAO_BQNK       *bqnk = (TAO_BQNK*)bnk->ctx;
  PetscErrorCode ierr;
  PetscInt       nupdates;

  PetscFunctionBegin;
  ierr = MatSolve(bqnk->B, tao->gradient, tao->stepdirection);CHKERRQ(ierr);
  ierr = VecScale(tao->stepdirection, -1.0);CHKERRQ(ierr);
  ierr = TaoBNKBoundStep(tao, bnk->as_type, tao->stepdirection);CHKERRQ(ierr);
  *ksp_reason = KSP_CONVERGED_ATOL;
  ierr = MatLMVMGetUpdateCount(bqnk->B, &nupdates);CHKERRQ(ierr);
  if (nupdates == 0) {
    *step_type = BNK_SCALED_GRADIENT;
  } else {
    *step_type = BNK_BFGS;
  }
  PetscFunctionReturn(0);
}

static PetscErrorCode TaoSetFromOptions_BQNLS(PetscOptionItems *PetscOptionsObject,Tao tao)
{
  TAO_BNK        *bnk = (TAO_BNK *)tao->data;
  TAO_BQNK       *bqnk = (TAO_BQNK*)bnk->ctx;
  PetscErrorCode ierr;
  KSPType        ksp_type;
  PetscBool      is_spd;

  PetscFunctionBegin;
  ierr = PetscOptionsHead(PetscOptionsObject,"Quasi-Newton-Krylov method for bound constrained optimization");CHKERRQ(ierr);
  ierr = PetscOptionsEList("-tao_bqnls_as_type", "active set estimation method", "", BNK_AS, BNK_AS_TYPES, BNK_AS[bnk->as_type], &bnk->as_type, 0);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_bqnls_epsilon", "(developer) tolerance used when computing actual and predicted reduction", "", bnk->epsilon, &bnk->epsilon,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_bqnls_as_tol", "(developer) initial tolerance used when estimating actively bounded variables", "", bnk->as_tol, &bnk->as_tol,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_bqnls_as_step", "(developer) step length used when estimating actively bounded variables", "", bnk->as_step, &bnk->as_step,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsInt("-tao_bqnls_max_cg_its", "number of BNCG iterations to take for each Newton step", "", bnk->max_cg_its, &bnk->max_cg_its,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsTail();CHKERRQ(ierr);
  ierr = TaoSetFromOptions(bnk->bncg);CHKERRQ(ierr);
  ierr = TaoLineSearchSetFromOptions(tao->linesearch);CHKERRQ(ierr);
  ierr = KSPSetFromOptions(tao->ksp);CHKERRQ(ierr);
  ierr = KSPGetType(tao->ksp,&ksp_type);CHKERRQ(ierr);
  bnk->is_nash = bnk->is_gltr = bnk->is_stcg = PETSC_FALSE;
  ierr = MatSetFromOptions(bqnk->B);CHKERRQ(ierr);
  ierr = MatGetOption(bqnk->B, MAT_SPD, &is_spd);CHKERRQ(ierr);
  if (!is_spd) SETERRQ(PetscObjectComm((PetscObject)tao), PETSC_ERR_ARG_INCOMP, "LMVM matrix must be symmetric positive-definite");
  PetscFunctionReturn(0);
}

PETSC_EXTERN PetscErrorCode TaoCreate_BQNLS(Tao tao)
{
  TAO_BNK        *bnk;
  TAO_BQNK       *bqnk;
  PetscErrorCode ierr;
  
  PetscFunctionBegin;
  ierr = TaoCreate_BQNK(tao);CHKERRQ(ierr);
  ierr = KSPSetOptionsPrefix(tao->ksp, "unused");CHKERRQ(ierr);
  tao->ops->solve = TaoSolve_BNLS;
  tao->ops->setfromoptions = TaoSetFromOptions_BQNLS;
  
  bnk = (TAO_BNK*)tao->data;
  bnk->update_type = BNK_UPDATE_STEP;
  bnk->computehessian = TaoBQNLSComputeHessian;
  bnk->computestep = TaoBQNLSComputeStep;
  
  bqnk = (TAO_BQNK*)bnk->ctx;
  ierr = MatSetOptionsPrefix(bqnk->B, "tao_bqnls_");CHKERRQ(ierr);
  ierr = MatSetType(bqnk->B, MATLMVMBFGS);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}