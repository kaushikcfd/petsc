/*$Id: dscatter.c,v 1.35 2001/01/15 21:43:37 bsmith Exp balay $*/
/*
       Contains the data structure for drawing scatter plots
    graphs in a window with an axis. This is intended for scatter
    plots that change dynamically.
*/

#include "petsc.h"         /*I "petsc.h" I*/

struct _p_DrawSP {
  PETSCHEADER(int) 
  int         (*destroy)(PetscDrawSP);
  int         (*view)(PetscDrawSP,PetscViewer);
  int         len,loc;
  PetscDraw        win;
  PetscDrawAxis    axis;
  PetscReal   xmin,xmax,ymin,ymax,*x,*y;
  int         nopts,dim;
};

#define CHUNCKSIZE 100

#undef __FUNC__  
#define __FUNC__ "PetscDrawSPCreate" 
/*@C
    PetscDrawSPCreate - Creates a scatter plot data structure.

    Collective over PetscDraw

    Input Parameters:
+   win - the window where the graph will be made.
-   dim - the number of sets of points which will be drawn

    Output Parameters:
.   drawsp - the scatter plot context

   Level: intermediate

   Concepts: scatter plot^creating

.seealso:  PetscDrawSPDestroy()
@*/
int PetscDrawSPCreate(PetscDraw draw,int dim,PetscDrawSP *drawsp)
{
  int         ierr;
  PetscTruth  isnull;
  PetscObject obj = (PetscObject)draw;
  PetscDrawSP      sp;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(draw,PETSC_DRAW_COOKIE);
  PetscValidPointer(drawsp);
  ierr = PetscTypeCompare(obj,PETSC_DRAW_NULL,&isnull);CHKERRQ(ierr);
  if (isnull) {
    ierr = PetscDrawOpenNull(obj->comm,(PetscDraw*)drawsp);CHKERRQ(ierr);
    PetscFunctionReturn(0);
  }
  PetscHeaderCreate(sp,_p_DrawSP,int,DRAWSP_COOKIE,0,"PetscDrawSP",obj->comm,PetscDrawSPDestroy,0);
  sp->view    = 0;
  sp->destroy = 0;
  sp->nopts   = 0;
  sp->win     = draw;
  sp->dim     = dim;
  sp->xmin    = 1.e20;
  sp->ymin    = 1.e20;
  sp->xmax    = -1.e20;
  sp->ymax    = -1.e20;
  ierr = PetscMalloc(2*dim*CHUNCKSIZE*sizeof(PetscReal),&sp->x);CHKERRQ(ierr);
  PetscLogObjectMemory(sp,2*dim*CHUNCKSIZE*sizeof(PetscReal));
  sp->y       = sp->x + dim*CHUNCKSIZE;
  sp->len     = dim*CHUNCKSIZE;
  sp->loc     = 0;
  ierr = PetscDrawAxisCreate(draw,&sp->axis);CHKERRQ(ierr);
  PetscLogObjectParent(sp,sp->axis);
  *drawsp = sp;
  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ "PetscDrawSPSetDimension" 
/*@
   PetscDrawSPSetDimension - Change the number of sets of points  that are to be drawn.

   Not Collective (ignored on all processors except processor 0 of PetscDrawSP)

   Input Parameter:
+  sp - the line graph context.
-  dim - the number of curves.

   Level: intermediate

   Concepts: scatter plot^setting number of data types

@*/
int PetscDrawSPSetDimension(PetscDrawSP sp,int dim)
{
  int ierr;

  PetscFunctionBegin;
  if (sp && sp->cookie == PETSC_DRAW_COOKIE) PetscFunctionReturn(0);
  PetscValidHeaderSpecific(sp,DRAWSP_COOKIE);
  if (sp->dim == dim) PetscFunctionReturn(0);

  ierr = PetscFree(sp->x);CHKERRQ(ierr);
  sp->dim     = dim;
  ierr = PetscMalloc(2*dim*CHUNCKSIZE*sizeof(PetscReal),&sp->x);CHKERRQ(ierr);
  PetscLogObjectMemory(sp,2*dim*CHUNCKSIZE*sizeof(PetscReal));
  sp->y       = sp->x + dim*CHUNCKSIZE;
  sp->len     = dim*CHUNCKSIZE;
  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ "PetscDrawSPReset" 
/*@
   PetscDrawSPReset - Clears line graph to allow for reuse with new data.

   Not Collective (ignored on all processors except processor 0 of PetscDrawSP)

   Input Parameter:
.  sp - the line graph context.

   Level: intermediate

  Concepts: scatter plot^resetting

@*/
int PetscDrawSPReset(PetscDrawSP sp)
{
  PetscFunctionBegin;
  if (sp && sp->cookie == PETSC_DRAW_COOKIE) PetscFunctionReturn(0);
  PetscValidHeaderSpecific(sp,DRAWSP_COOKIE);
  sp->xmin  = 1.e20;
  sp->ymin  = 1.e20;
  sp->xmax  = -1.e20;
  sp->ymax  = -1.e20;
  sp->loc   = 0;
  sp->nopts = 0;
  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ "PetscDrawSPDestroy" 
/*@C
   PetscDrawSPDestroy - Frees all space taken up by scatter plot data structure.

   Collective over PetscDrawSP

   Input Parameter:
.  sp - the line graph context

   Level: intermediate

.seealso:  PetscDrawSPCreate()
@*/
int PetscDrawSPDestroy(PetscDrawSP sp)
{
  int ierr;

  PetscFunctionBegin;
  PetscValidHeader(sp);

  if (--sp->refct > 0) PetscFunctionReturn(0);
  if (sp->cookie == PETSC_DRAW_COOKIE){
    ierr = PetscDrawDestroy((PetscDraw) sp);CHKERRQ(ierr);
    PetscFunctionReturn(0);
  }
  ierr = PetscDrawAxisDestroy(sp->axis);CHKERRQ(ierr);
  ierr = PetscFree(sp->x);CHKERRQ(ierr);
  PetscLogObjectDestroy(sp);
  PetscHeaderDestroy(sp);
  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ "PetscDrawSPAddPoint" 
/*@
   PetscDrawSPAddPoint - Adds another point to each of the scatter plots.

   Not Collective (ignored on all processors except processor 0 of PetscDrawSP)

   Input Parameters:
+  sp - the scatter plot data structure
-  x, y - the points to two vectors containing the new x and y 
          point for each curve.

   Level: intermediate

   Concepts: scatter plot^adding points

.seealso: PetscDrawSPAddPoints()
@*/
int PetscDrawSPAddPoint(PetscDrawSP sp,PetscReal *x,PetscReal *y)
{
  int i,ierr;

  PetscFunctionBegin;
  if (sp && sp->cookie == PETSC_DRAW_COOKIE) PetscFunctionReturn(0);

  PetscValidHeaderSpecific(sp,DRAWSP_COOKIE);
  if (sp->loc+sp->dim >= sp->len) { /* allocate more space */
    PetscReal *tmpx,*tmpy;
    ierr = PetscMalloc((2*sp->len+2*sp->dim*CHUNCKSIZE)*sizeof(PetscReal),&tmpx);CHKERRQ(ierr);
    PetscLogObjectMemory(sp,2*sp->dim*CHUNCKSIZE*sizeof(PetscReal));
    tmpy = tmpx + sp->len + sp->dim*CHUNCKSIZE;
    ierr = PetscMemcpy(tmpx,sp->x,sp->len*sizeof(PetscReal));CHKERRQ(ierr);
    ierr = PetscMemcpy(tmpy,sp->y,sp->len*sizeof(PetscReal));CHKERRQ(ierr);
    ierr = PetscFree(sp->x);CHKERRQ(ierr);
    sp->x = tmpx; sp->y = tmpy;
    sp->len += sp->dim*CHUNCKSIZE;
  }
  for (i=0; i<sp->dim; i++) {
    if (x[i] > sp->xmax) sp->xmax = x[i]; 
    if (x[i] < sp->xmin) sp->xmin = x[i];
    if (y[i] > sp->ymax) sp->ymax = y[i]; 
    if (y[i] < sp->ymin) sp->ymin = y[i];

    sp->x[sp->loc]   = x[i];
    sp->y[sp->loc++] = y[i];
  }
  sp->nopts++;
  PetscFunctionReturn(0);
}


#undef __FUNC__  
#define __FUNC__ "PetscDrawSPAddPoints" 
/*@C
   PetscDrawSPAddPoints - Adds several points to each of the scatter plots.

   Not Collective (ignored on all processors except processor 0 of PetscDrawSP)

   Input Parameters:
+  sp - the LineGraph data structure
.  xx,yy - points to two arrays of pointers that point to arrays 
           containing the new x and y points for each curve.
-  n - number of points being added

   Level: intermediate

   Concepts: scatter plot^adding points

.seealso: PetscDrawSPAddPoint()
@*/
int PetscDrawSPAddPoints(PetscDrawSP sp,int n,PetscReal **xx,PetscReal **yy)
{
  int       i,j,k,ierr;
  PetscReal *x,*y;

  PetscFunctionBegin;
  if (sp && sp->cookie == PETSC_DRAW_COOKIE) PetscFunctionReturn(0);
  PetscValidHeaderSpecific(sp,DRAWSP_COOKIE);
  if (sp->loc+n*sp->dim >= sp->len) { /* allocate more space */
    PetscReal *tmpx,*tmpy;
    int    chunk = CHUNCKSIZE;
    if (n > chunk) chunk = n;
    ierr = PetscMalloc((2*sp->len+2*sp->dim*chunk)*sizeof(PetscReal),&tmpx);CHKERRQ(ierr);
    PetscLogObjectMemory(sp,2*sp->dim*CHUNCKSIZE*sizeof(PetscReal));
    tmpy = tmpx + sp->len + sp->dim*chunk;
    ierr = PetscMemcpy(tmpx,sp->x,sp->len*sizeof(PetscReal));CHKERRQ(ierr);
    ierr = PetscMemcpy(tmpy,sp->y,sp->len*sizeof(PetscReal));CHKERRQ(ierr);
    ierr = PetscFree(sp->x);CHKERRQ(ierr);
    sp->x   = tmpx; sp->y = tmpy;
    sp->len += sp->dim*CHUNCKSIZE;
  }
  for (j=0; j<sp->dim; j++) {
    x = xx[j]; y = yy[j];
    k = sp->loc + j;
    for (i=0; i<n; i++) {
      if (x[i] > sp->xmax) sp->xmax = x[i]; 
      if (x[i] < sp->xmin) sp->xmin = x[i];
      if (y[i] > sp->ymax) sp->ymax = y[i]; 
      if (y[i] < sp->ymin) sp->ymin = y[i];

      sp->x[k]   = x[i];
      sp->y[k] = y[i];
      k += sp->dim;
    }
  }
  sp->loc   += n*sp->dim;
  sp->nopts += n;
  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ "PetscDrawSPDraw" 
/*@
   PetscDrawSPDraw - Redraws a scatter plot.

   Not Collective (ignored on all processors except processor 0 of PetscDrawSP)

   Input Parameter:
.  sp - the line graph context

   Level: intermediate

@*/
int PetscDrawSPDraw(PetscDrawSP sp)
{
  PetscReal xmin=sp->xmin,xmax=sp->xmax,ymin=sp->ymin,ymax=sp->ymax;
  int       ierr,i,j,dim = sp->dim,nopts = sp->nopts,rank;
  PetscDraw      draw = sp->win;

  PetscFunctionBegin;
  if (sp && sp->cookie == PETSC_DRAW_COOKIE) PetscFunctionReturn(0);
  PetscValidHeaderSpecific(sp,DRAWSP_COOKIE);

  if (nopts < 1) PetscFunctionReturn(0);
  if (xmin > xmax || ymin > ymax) PetscFunctionReturn(0);
  ierr = PetscDrawClear(draw);CHKERRQ(ierr);
  ierr = PetscDrawAxisSetLimits(sp->axis,xmin,xmax,ymin,ymax);CHKERRQ(ierr);
  ierr = PetscDrawAxisDraw(sp->axis);CHKERRQ(ierr);
  
  ierr = MPI_Comm_rank(sp->comm,&rank);CHKERRQ(ierr);
  if (rank)   PetscFunctionReturn(0);
  for (i=0; i<dim; i++) {
    for (j=0; j<nopts; j++) {
      ierr = PetscDrawString(draw,sp->x[j*dim+i],sp->y[j*dim+i],PETSC_DRAW_RED,"x");CHKERRQ(ierr);
    }
  }
  ierr = PetscDrawFlush(sp->win);CHKERRQ(ierr);
  ierr = PetscDrawPause(sp->win);CHKERRQ(ierr);
  PetscFunctionReturn(0);
} 
 
#undef __FUNC__  
#define __FUNC__ "PetscDrawSPSetLimits" 
/*@
   PetscDrawSPSetLimits - Sets the axis limits for a line graph. If more
   points are added after this call, the limits will be adjusted to
   include those additional points.

   Not Collective (ignored on all processors except processor 0 of PetscDrawSP)

   Input Parameters:
+  xsp - the line graph context
-  x_min,x_max,y_min,y_max - the limits

   Level: intermediate

   Concepts: scatter plot^setting axis

@*/
int PetscDrawSPSetLimits(PetscDrawSP sp,PetscReal x_min,PetscReal x_max,PetscReal y_min,PetscReal y_max) 
{
  PetscFunctionBegin;
  if (sp && sp->cookie == PETSC_DRAW_COOKIE) PetscFunctionReturn(0);
  PetscValidHeaderSpecific(sp,DRAWSP_COOKIE);
  sp->xmin = x_min; 
  sp->xmax = x_max; 
  sp->ymin = y_min; 
  sp->ymax = y_max;
  PetscFunctionReturn(0);
}
 
#undef __FUNC__  
#define __FUNC__ "PetscDrawSPGetAxis" 
/*@C
   PetscDrawSPGetAxis - Gets the axis context associated with a line graph.
   This is useful if one wants to change some axis property, such as
   labels, color, etc. The axis context should not be destroyed by the
   application code.

   Not Collective (except PetscDrawAxis can only be used on processor 0 of PetscDrawSP)

   Input Parameter:
.  sp - the line graph context

   Output Parameter:
.  axis - the axis context

   Level: intermediate

@*/
int PetscDrawSPGetAxis(PetscDrawSP sp,PetscDrawAxis *axis)
{
  PetscFunctionBegin;
  if (sp && sp->cookie == PETSC_DRAW_COOKIE) {
    *axis = 0;
    PetscFunctionReturn(0);
  }
  PetscValidHeaderSpecific(sp,DRAWSP_COOKIE);
  *axis = sp->axis;
  PetscFunctionReturn(0);
}

#undef __FUNC__  
#define __FUNC__ "PetscDrawSPGetDraw" 
/*@C
   PetscDrawSPGetDraw - Gets the draw context associated with a line graph.

   Not Collective, PetscDraw is parallel if PetscDrawSP is parallel

   Input Parameter:
.  sp - the line graph context

   Output Parameter:
.  draw - the draw context

   Level: intermediate

@*/
int PetscDrawSPGetDraw(PetscDrawSP sp,PetscDraw *draw)
{
  PetscFunctionBegin;
  PetscValidHeader(sp);
  if (sp && sp->cookie == PETSC_DRAW_COOKIE) {
    *draw = (PetscDraw)sp;
  } else {
    *draw = sp->win;
  }
  PetscFunctionReturn(0);
}
