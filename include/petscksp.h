/* $Id: ksp.h,v 1.17 1995/06/07 16:31:37 bsmith Exp bsmith $ */

#ifndef __KSP_PACKAGE
#define __KSP_PACKAGE
#include "petsc.h"
#include "vec.h"
#include "mat.h"
#include "pc.h"

#define KSP_COOKIE  PETSC_COOKIE+8

typedef struct _KSP*     KSP;

/*  Possible Krylov Space Methods */
typedef enum { KSPRICHARDSON, KSPCHEBYCHEV, KSPCG, KSPGMRES, 
               KSPTCQMR, KSPBCGS, KSPCGS, KSPTFQMR, KSPCR, KSPLSQR,
               KSPPREONLY } KSPMethod;

extern int KSPCreate(MPI_Comm,KSP *);
extern int KSPSetMethod(KSP,KSPMethod);
extern int KSPSetUp(KSP);
extern int KSPSolve(KSP,int *);
extern int KSPDestroy(KSP);

extern int KSPRegisterAll();
extern int KSPRegisterDestroy();
extern int KSPRegister(KSPMethod,char *,int (*)(KSP));

extern int KSPGetMethodName(KSPMethod,char **);
extern int KSPSetMaxIterations(KSP,int);
extern int KSPSetRightPreconditioner(KSP);
extern int KSPGetPreconditionerSide(KSP,int *);
extern int KSPGetMethodFromContext(KSP,KSPMethod *);
extern int KSPSetRelativeTolerance(KSP,double);
extern int KSPSetAbsoluteTolerance(KSP,double);
extern int KSPSetDivergenceTolerance(KSP,double);
extern int KSPGetTolerances(KSP,double*,double*,double*);
extern int KSPSetCalculateResidual(KSP,PetscTruth);
extern int KSPSetUsePreconditionedResidual(KSP);
extern int KSPSetInitialGuessNonzero(KSP);
extern int KSPSetCalculateEigenvalues(KSP);
extern int KSPSetRhs(KSP,Vec);
extern int KSPGetRhs(KSP,Vec *);
extern int KSPSetSolution(KSP,Vec);
extern int KSPGetSolution(KSP,Vec *);

extern int KSPSetBinv(KSP,PC);
extern int KSPGetBinv(KSP,PC*);

extern int KSPSetMonitor(KSP,int (*)(KSP,int,double, void*), void *);
extern int KSPGetMonitorContext(KSP,void **);
extern int KSPSetResidualHistory(KSP, double *,int);
extern int KSPSetConvergenceTest(KSP,int (*)(KSP,int,double, void*), void *);
extern int KSPGetConvergenceContext(KSP,void **);

extern int KSPBuildSolution(KSP, Vec,Vec *);
extern int KSPBuildResidual(KSP, Vec, Vec,Vec *);

extern int KSPRichardsonSetScale(KSP , double);
extern int KSPChebychevSetEigenvalues(KSP , double, double);
extern int KSPCGGetEigenvalues(KSP, int n,double*,double*);
extern int KSPGMRESSetRestart(KSP, int);
extern int KSPGMRESSetUseUnmodifiedGrammSchmidt(KSP);
extern int KSPSetFromOptions(KSP);

extern int KSPDefaultCGMonitor(KSP,int,double, void * );
extern int KSPDefaultCGConverged(KSP,int,double, void *);
extern int KSPDefaultMonitor(KSP,int,double, void *);
extern int KSPDefaultSMonitor(KSP,int,double, void *);
extern int KSPDefaultConverged(KSP,int,double, void *);

extern int KSPPrintHelp(KSP);

extern int KSPSetOptionsPrefix(KSP,char*);

extern int KSPView(KSP,Viewer);

#if defined(__DRAW_PACKAGE)
extern int KSPLGMonitorCreate(char*,char*,int,int,int,int,DrawLGCtx*);
extern int KSPLGMonitor(KSP,int,double,void*);
extern int KSPLGMonitorDestroy(DrawLGCtx);
#endif 

#endif


