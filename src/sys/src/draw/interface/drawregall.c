/*$Id: drawregall.c,v 1.18 2001/01/15 21:43:22 bsmith Exp balay $*/
/*
       Provides the calling sequences for all the basic PetscDraw routines.
*/
#include "src/sys/src/draw/drawimpl.h"  /*I "petscdraw.h" I*/

EXTERN_C_BEGIN
EXTERN int PetscDrawCreate_X(PetscDraw);
EXTERN int PetscDrawCreate_PS(PetscDraw);
EXTERN int PetscDrawCreate_Null(PetscDraw);
EXTERN int PetscDrawCreate_Win32(PetscDraw);
EXTERN_C_END
  
#undef __FUNC__  
#define __FUNC__ "PetscDrawRegisterAll" 
/*@C
  PetscDrawRegisterAll - Registers all of the graphics methods in the PetscDraw package.

  Not Collective

  Level: developer

.seealso:  PetscDrawRegisterDestroy()
@*/
int PetscDrawRegisterAll(char *path)
{
  int ierr;

  PetscFunctionBegin;
  
#if defined(PETSC_HAVE_X11)
  ierr = PetscDrawRegisterDynamic(PETSC_DRAW_X,     path,"PetscDrawCreate_X",     PetscDrawCreate_X);CHKERRQ(ierr);
#elif defined (PARCH_win32)
  ierr = PetscDrawRegisterDynamic(PETSC_DRAW_WIN32, path,"PetscDrawCreate_Win32", PetscDrawCreate_Win32);CHKERRQ(ierr);
#endif
  ierr = PetscDrawRegisterDynamic(PETSC_DRAW_NULL,  path,"PetscDrawCreate_Null",  PetscDrawCreate_Null);CHKERRQ(ierr);
  ierr = PetscDrawRegisterDynamic(PETSC_DRAW_PS,    path,"PetscDrawCreate_PS",    PetscDrawCreate_PS);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

