/*$Id: matstash.c,v 1.43 2000/04/26 21:21:56 bsmith Exp balay $*/

#include "src/mat/matimpl.h"

/*
       The input to the stash is ALWAYS in MatScalar precision, and the 
    internal storage and output is also in MatScalar.
*/
#define DEFAULT_STASH_SIZE   10000

/*
  MatStashCreate_Private - Creates a stash,currently used for all the parallel 
  matrix implementations. The stash is where elements of a matrix destined 
  to be stored on other processors are kept until matrix assembly is done.

  This is a simple minded stash. Simply adds entries to end of stash.

  Input Parameters:
  comm - communicator, required for scatters.
  bs   - stash block size. used when stashing blocks of values

  Output Parameters:
  stash    - the newly created stash
*/
#undef __FUNC__  
#define __FUNC__ /*<a name=""></a>*/"MatStashCreate_Private"
int MatStashCreate_Private(MPI_Comm comm,int bs,MatStash *stash)
{
  int        ierr,max,*opt,nopt;
  PetscTruth flg;

  PetscFunctionBegin;
  /* Require 2 tags,get the second using PetscCommGetNewTag() */
  stash->comm = comm;
  ierr = PetscCommGetNewTag(stash->comm,&stash->tag1);CHKERRQ(ierr);
  ierr = PetscCommGetNewTag(stash->comm,&stash->tag2);CHKERRQ(ierr);
  ierr = MPI_Comm_size(stash->comm,&stash->size);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(stash->comm,&stash->rank);CHKERRQ(ierr);

  nopt = stash->size;
  opt  = (int*)PetscMalloc(nopt*sizeof(int));CHKPTRQ(opt);
  ierr = OptionsGetIntArray(PETSC_NULL,"-vecstash_initial_size",opt,&nopt,&flg);CHKERRQ(ierr);
  if (flg) {
    if (nopt == 1)                max = opt[0];
    else if (nopt == stash->size) max = opt[stash->rank];
    else if (stash->rank < nopt)  max = opt[stash->rank];
    else                          max = 0; /* Use default */
    stash->umax = max;
  } else {
    stash->umax = 0;
  }
  ierr = PetscFree(opt);CHKERRQ(ierr);
  if (bs <= 0) bs = 1;

  stash->bs       = bs;
  stash->nmax     = 0;
  stash->oldnmax  = 0;
  stash->n        = 0;
  stash->reallocs = -1;
  stash->idx      = 0;
  stash->idy      = 0;
  stash->array    = 0;

  stash->send_waits  = 0;
  stash->recv_waits  = 0;
  stash->send_status = 0;
  stash->nsends      = 0;
  stash->nrecvs      = 0;
  stash->svalues     = 0;
  stash->rvalues     = 0;
  stash->rmax        = 0;
  stash->nprocs      = 0;
  stash->nprocessed  = 0;
  PetscFunctionReturn(0);
}

/* 
   MatStashDestroy_Private - Destroy the stash
*/
#undef __FUNC__  
#define __FUNC__ /*<a name=""></a>*/"MatStashDestroy_Private"
int MatStashDestroy_Private(MatStash *stash)
{
  int ierr;

  PetscFunctionBegin;
  ierr = PetscCommRestoreNewTag(stash->comm,&stash->tag2);CHKERRQ(ierr);
  ierr = PetscCommRestoreNewTag(stash->comm,&stash->tag1);CHKERRQ(ierr);
  if (stash->array) {
    ierr = PetscFree(stash->array);CHKERRQ(ierr);
    stash->array = 0;
  }
  PetscFunctionReturn(0);
}

/* 
   MatStashScatterEnd_Private - This is called as the fial stage of
   scatter. The final stages of messagepassing is done here, and
   all the memory used for messagepassing is cleanedu up. This
   routine also resets the stash, and deallocates the memory used
   for the stash. It also keeps track of the current memory usage
   so that the same value can be used the next time through.
*/
#undef __FUNC__  
#define __FUNC__ /*<a name=""></a>*/"MatStashScatterEnd_Private"
int MatStashScatterEnd_Private(MatStash *stash)
{ 
  int         nsends=stash->nsends,ierr,bs2,oldnmax;
  MPI_Status  *send_status;

  PetscFunctionBegin;
  /* wait on sends */
  if (nsends) {
    send_status = (MPI_Status *)PetscMalloc(2*nsends*sizeof(MPI_Status));CHKPTRQ(send_status);
    ierr        = MPI_Waitall(2*nsends,stash->send_waits,send_status);CHKERRQ(ierr);
    ierr        = PetscFree(send_status);CHKERRQ(ierr);
  }

  /* Now update nmaxold to be app 10% more than max n used, this way the
     wastage of space is reduced the next time this stash is used.
     Also update the oldmax, only if it increases */
  bs2      = stash->bs*stash->bs;
  oldnmax  = ((int)(stash->n * 1.1) + 5)*bs2;
  if (oldnmax > stash->oldnmax) stash->oldnmax = oldnmax;

  stash->nmax       = 0;
  stash->n          = 0;
  stash->reallocs   = -1;
  stash->rmax       = 0;
  stash->nprocessed = 0;

  if (stash->array) {
    ierr         = PetscFree(stash->array);CHKERRQ(ierr);
    stash->array = 0;
    stash->idx   = 0;
    stash->idy   = 0;
  }
  if (stash->send_waits) {
    ierr = PetscFree(stash->send_waits);CHKERRQ(ierr);
    stash->send_waits = 0;
  }
  if (stash->recv_waits) {
    ierr = PetscFree(stash->recv_waits);CHKERRQ(ierr);
    stash->recv_waits = 0;
  } 
  if (stash->svalues) {
    ierr = PetscFree(stash->svalues);CHKERRQ(ierr);
    stash->svalues = 0;
  }
  if (stash->rvalues) {
    ierr = PetscFree(stash->rvalues);CHKERRQ(ierr);
    stash->rvalues = 0;
  }
  if (stash->nprocs) {
    ierr = PetscFree(stash->nprocs);
    stash->nprocs = 0;
  }

  PetscFunctionReturn(0);
}

/* 
   MatStashGetInfo_Private - Gets the relavant statistics of the stash

   Input Parameters:
   stash    - the stash
   nstash   - the size of the stash. Indicates the number of values stored.
   reallocs - the number of additional mallocs incurred.
   
*/
#undef __FUNC__  
#define __FUNC__ /*<a name=""></a>*/"MatStashGetInfo_Private"
int MatStashGetInfo_Private(MatStash *stash,int *nstash,int *reallocs)
{
  int bs2 = stash->bs*stash->bs;

  PetscFunctionBegin;
  *nstash   = stash->n*bs2;
  if (stash->reallocs < 0) *reallocs = 0;
  else                     *reallocs = stash->reallocs;
  PetscFunctionReturn(0);
}


/* 
   MatStashSetInitialSize_Private - Sets the initial size of the stash

   Input Parameters:
   stash  - the stash
   max    - the value that is used as the max size of the stash. 
            this value is used while allocating memory.
*/
#undef __FUNC__  
#define __FUNC__ /*<a name=""></a>*/"MatStashSetInitialSize_Private"
int MatStashSetInitialSize_Private(MatStash *stash,int max)
{
  PetscFunctionBegin;
  stash->umax = max;
  PetscFunctionReturn(0);
}

/* MatStashExpand_Private - Expand the stash. This function is called
   when the space in the stash is not sufficient to add the new values
   being inserted into the stash.
   
   Input Parameters:
   stash - the stash
   incr  - the minimum increase requested
   
   Notes: 
   This routine doubles the currently used memory. 
 */
#undef __FUNC__  
#define __FUNC__ /*<a name=""></a>*/"MatStashExpand_Private"
static int MatStashExpand_Private(MatStash *stash,int incr)
{ 
  int       *n_idx,*n_idy,newnmax,bs2,ierr;
  MatScalar *n_array;

  PetscFunctionBegin;
  /* allocate a larger stash */
  bs2     = stash->bs*stash->bs; 
  if (!stash->oldnmax && !stash->nmax) { /* new stash */
    if (stash->umax)                  newnmax = stash->umax/bs2;             
    else                              newnmax = DEFAULT_STASH_SIZE/bs2;
  } else if (!stash->nmax) { /* resuing stash */ 
    if (stash->umax > stash->oldnmax) newnmax = stash->umax/bs2;
    else                              newnmax = stash->oldnmax/bs2;
  } else                              newnmax = stash->nmax*2;
  if (newnmax  < (stash->nmax + incr)) newnmax += 2*incr;

  n_array = (MatScalar*)PetscMalloc((newnmax)*(2*sizeof(int)+bs2*sizeof(MatScalar)));CHKPTRQ(n_array);
  n_idx   = (int*)(n_array + bs2*newnmax);
  n_idy   = (int*)(n_idx + newnmax);
  ierr = PetscMemcpy(n_array,stash->array,bs2*stash->nmax*sizeof(MatScalar));CHKERRQ(ierr);
  ierr = PetscMemcpy(n_idx,stash->idx,stash->nmax*sizeof(int));CHKERRQ(ierr);
  ierr = PetscMemcpy(n_idy,stash->idy,stash->nmax*sizeof(int));CHKERRQ(ierr);
  if (stash->array) {ierr = PetscFree(stash->array);CHKERRQ(ierr);}
  stash->array   = n_array; 
  stash->idx     = n_idx; 
  stash->idy     = n_idy;
  stash->nmax    = newnmax;
  stash->reallocs++;
  PetscFunctionReturn(0);
}
/*
  MatStashValuesRow_Private - inserts values into the stash. This function
  expects the values to be roworiented. Multiple columns belong to the same row
  can be inserted with a single call to this function.

  Input Parameters:
  stash  - the stash
  row    - the global row correspoiding to the values
  n      - the number of elements inserted. All elements belong to the above row.
  idxn   - the global column indices corresponding to each of the values.
  values - the values inserted
*/
#undef __FUNC__  
#define __FUNC__ /*<a name=""></a>*/"MatStashValuesRow_Private"
int MatStashValuesRow_Private(MatStash *stash,int row,int n,int *idxn,MatScalar *values)
{
  int    ierr,i; 

  PetscFunctionBegin;
  /* Check and see if we have sufficient memory */
  if ((stash->n + n) > stash->nmax) {
    ierr = MatStashExpand_Private(stash,n);CHKERRQ(ierr);
  }
  for (i=0; i<n; i++) {
    stash->idx[stash->n]   = row;
    stash->idy[stash->n]   = idxn[i];
    stash->array[stash->n] = values[i];
    stash->n++;
  }
  PetscFunctionReturn(0);
}
/*
  MatStashValuesCol_Private - inserts values into the stash. This function
  expects the values to be columnoriented. Multiple columns belong to the same row
  can be inserted with a single call to this function.

  Input Parameters:
  stash   - the stash
  row     - the global row correspoiding to the values
  n       - the number of elements inserted. All elements belong to the above row.
  idxn    - the global column indices corresponding to each of the values.
  values  - the values inserted
  stepval - the consecutive values are sepated by a distance of stepval.
            this happens because the input is columnoriented.
*/
#undef __FUNC__  
#define __FUNC__ /*<a name=""></a>*/"MatStashValuesCol_Private"
int MatStashValuesCol_Private(MatStash *stash,int row,int n,int *idxn,MatScalar *values,int stepval)
{
  int    ierr,i; 

  PetscFunctionBegin;
  /* Check and see if we have sufficient memory */
  if ((stash->n + n) > stash->nmax) {
    ierr = MatStashExpand_Private(stash,n);CHKERRQ(ierr);
  }
  for (i=0; i<n; i++) {
    stash->idx[stash->n]   = row;
    stash->idy[stash->n]   = idxn[i];
    stash->array[stash->n] = values[i*stepval];
    stash->n++;
  }
  PetscFunctionReturn(0);
}

/*
  MatStashValuesRowBlocked_Private - inserts blocks of values into the stash. 
  This function expects the values to be roworiented. Multiple columns belong 
  to the same block-row can be inserted with a single call to this function.
  This function extracts the sub-block of values based on the dimensions of
  the original input block, and the row,col values corresponding to the blocks.

  Input Parameters:
  stash  - the stash
  row    - the global block-row correspoiding to the values
  n      - the number of elements inserted. All elements belong to the above row.
  idxn   - the global block-column indices corresponding to each of the blocks of 
           values. Each block is of size bs*bs.
  values - the values inserted
  rmax   - the number of block-rows in the original block.
  cmax   - the number of block-columsn on the original block.
  idx    - the index of the current block-row in the original block.
*/
#undef __FUNC__  
#define __FUNC__ /*<a name=""></a>*/"MatStashValuesRowBlocked_Private"
int MatStashValuesRowBlocked_Private(MatStash *stash,int row,int n,int *idxn,MatScalar *values,int rmax,int cmax,int idx)
{
  int       ierr,i,j,k,bs2,bs=stash->bs; 
  MatScalar *vals,*array;

  PetscFunctionBegin;
  bs2 = bs*bs;
  if ((stash->n+n) > stash->nmax) {
    ierr = MatStashExpand_Private(stash,n);CHKERRQ(ierr);
  }
  for (i=0; i<n; i++) {
    stash->idx[stash->n]   = row;
    stash->idy[stash->n] = idxn[i];
    /* Now copy over the block of values. Store the values column oriented.
       This enables inserting multiple blocks belonging to a row with a single
       funtion call */
    array = stash->array + bs2*stash->n;
    vals  = values + idx*bs2*n + bs*i;
    for (j=0; j<bs; j++) {
      for (k=0; k<bs; k++) {array[k*bs] = vals[k];}
      array += 1;
      vals  += cmax*bs;
    }
    stash->n++;
  }
  PetscFunctionReturn(0);
}

/*
  MatStashValuesColBlocked_Private - inserts blocks of values into the stash. 
  This function expects the values to be roworiented. Multiple columns belong 
  to the same block-row can be inserted with a single call to this function.
  This function extracts the sub-block of values based on the dimensions of
  the original input block, and the row,col values corresponding to the blocks.

  Input Parameters:
  stash  - the stash
  row    - the global block-row correspoiding to the values
  n      - the number of elements inserted. All elements belong to the above row.
  idxn   - the global block-column indices corresponding to each of the blocks of 
           values. Each block is of size bs*bs.
  values - the values inserted
  rmax   - the number of block-rows in the original block.
  cmax   - the number of block-columsn on the original block.
  idx    - the index of the current block-row in the original block.
*/
#undef __FUNC__  
#define __FUNC__ /*<a name=""></a>*/"MatStashValuesColBlocked_Private"
int MatStashValuesColBlocked_Private(MatStash *stash,int row,int n,int *idxn,MatScalar *values,int rmax,int cmax,int idx)
{
  int       ierr,i,j,k,bs2,bs=stash->bs; 
  MatScalar *vals,*array;

  PetscFunctionBegin;
  bs2 = bs*bs;
  if ((stash->n+n) > stash->nmax) {
    ierr = MatStashExpand_Private(stash,n);CHKERRQ(ierr);
  }
  for (i=0; i<n; i++) {
    stash->idx[stash->n]   = row;
    stash->idy[stash->n] = idxn[i];
    /* Now copy over the block of values. Store the values column oriented.
     This enables inserting multiple blocks belonging to a row with a single
     funtion call */
    array = stash->array + bs2*stash->n;
    vals  = values + idx*bs + bs2*rmax*i;
    for (j=0; j<bs; j++) {
      for (k=0; k<bs; k++) {array[k] = vals[k];}
      array += bs;
      vals  += rmax*bs;
    }
    stash->n++;
  }
  PetscFunctionReturn(0);
}
/*
  MatStashScatterBegin_Private - Initiates the transfer of values to the
  correct owners. This function goes through the stash, and check the
  owners of each stashed value, and sends the values off to the owner
  processors.

  Input Parameters:
  stash  - the stash
  owners - an array of size 'no-of-procs' which gives the ownership range
           for each node.

  Notes: The 'owners' array in the cased of the blocked-stash has the 
  ranges specified blocked global indices, and for the regular stash in
  the proper global indices.
*/
#undef __FUNC__  
#define __FUNC__ /*<a name=""></a>*/"MatStashScatterBegin_Private"
int MatStashScatterBegin_Private(MatStash *stash,int *owners)
{ 
  int         *owner,*startv,*starti,tag1=stash->tag1,tag2=stash->tag2,bs2;
  int         rank=stash->rank,size=stash->size,*nprocs,*procs,nsends,nreceives;
  int         nmax,*work,count,ierr,*sindices,*rindices,i,j,idx;
  MatScalar   *rvalues,*svalues;
  MPI_Comm    comm = stash->comm;
  MPI_Request *send_waits,*recv_waits;

  PetscFunctionBegin;

  bs2 = stash->bs*stash->bs;
  /*  first count number of contributors to each processor */
  nprocs = (int*)PetscMalloc(2*size*sizeof(int));CHKPTRQ(nprocs);
  ierr   = PetscMemzero(nprocs,2*size*sizeof(int));CHKERRQ(ierr);
  procs  = nprocs + size;
  owner  = (int*)PetscMalloc((stash->n+1)*sizeof(int));CHKPTRQ(owner);

  for (i=0; i<stash->n; i++) {
    idx = stash->idx[i];
    for (j=0; j<size; j++) {
      if (idx >= owners[j] && idx < owners[j+1]) {
        nprocs[j]++; procs[j] = 1; owner[i] = j; break;
      }
    }
  }
  nsends = 0;  for (i=0; i<size; i++) { nsends += procs[i];} 
  
  /* inform other processors of number of messages and max length*/
  work      = (int *)PetscMalloc(2*size*sizeof(int));CHKPTRQ(work);
  ierr      = MPI_Allreduce(nprocs,work,2*size,MPI_INT,PetscMaxSum_Op,comm);CHKERRQ(ierr);
  nmax      = work[rank];
  nreceives = work[size+rank]; 
  ierr      = PetscFree(work);CHKERRQ(ierr);
  /* post receives: 
     since we don't know how long each individual message is we 
     allocate the largest needed buffer for each receive. Potentially 
     this is a lot of wasted space.
  */
  rvalues    = (MatScalar*)PetscMalloc((nreceives+1)*(nmax+1)*(bs2*sizeof(MatScalar)+2*sizeof(int)));CHKPTRQ(rvalues);
  rindices   = (int*)(rvalues + bs2*nreceives*nmax);
  recv_waits = (MPI_Request *)PetscMalloc((nreceives+1)*2*sizeof(MPI_Request));CHKPTRQ(recv_waits);
  for (i=0,count=0; i<nreceives; i++) {
    ierr = MPI_Irecv(rvalues+bs2*nmax*i,bs2*nmax,MPIU_MATSCALAR,MPI_ANY_SOURCE,tag1,comm,
                     recv_waits+count++);CHKERRQ(ierr);
    ierr = MPI_Irecv(rindices+2*nmax*i,2*nmax,MPI_INT,MPI_ANY_SOURCE,tag2,comm,recv_waits+count++);CHKERRQ(ierr);
  }

  /* do sends:
      1) starts[i] gives the starting index in svalues for stuff going to 
         the ith processor
  */
  svalues    = (MatScalar*)PetscMalloc((stash->n+1)*(bs2*sizeof(MatScalar)+2*sizeof(int)));CHKPTRQ(svalues);
  sindices   = (int*)(svalues + bs2*stash->n);
  send_waits = (MPI_Request*)PetscMalloc(2*(nsends+1)*sizeof(MPI_Request));CHKPTRQ(send_waits);
  startv     = (int*)PetscMalloc(2*size*sizeof(int));CHKPTRQ(startv);
  starti     = startv + size;
  /* use 2 sends the first with all_a, the next with all_i and all_j */
  startv[0]  = 0; starti[0] = 0;
  for (i=1; i<size; i++) { 
    startv[i] = startv[i-1] + nprocs[i-1];
    starti[i] = starti[i-1] + nprocs[i-1]*2;
  } 
  for (i=0; i<stash->n; i++) {
    j = owner[i];
    if (bs2 == 1) {
      svalues[startv[j]]              = stash->array[i];
    } else {
      int       k;
      MatScalar *buf1,*buf2;
      buf1 = svalues+bs2*startv[j];
      buf2 = stash->array+bs2*i;
      for (k=0; k<bs2; k++){ buf1[k] = buf2[k]; }
    }
    sindices[starti[j]]             = stash->idx[i];
    sindices[starti[j]+nprocs[j]]   = stash->idy[i];
    startv[j]++;
    starti[j]++;
  }
  startv[0] = 0;
  for (i=1; i<size; i++) { startv[i] = startv[i-1] + nprocs[i-1];} 
  for (i=0,count=0; i<size; i++) {
    if (procs[i]) {
      ierr = MPI_Isend(svalues+bs2*startv[i],bs2*nprocs[i],MPIU_MATSCALAR,i,tag1,comm,
                       send_waits+count++);CHKERRQ(ierr);
      ierr = MPI_Isend(sindices+2*startv[i],2*nprocs[i],MPI_INT,i,tag2,comm,
                       send_waits+count++);CHKERRQ(ierr);
    }
  }
  ierr = PetscFree(owner);CHKERRQ(ierr);
  ierr = PetscFree(startv);CHKERRQ(ierr);
  /* This memory is reused in scatter end  for a different purpose*/
  for (i=0; i<2*size; i++) nprocs[i] = -1;
  stash->nprocs      = nprocs;

  stash->svalues    = svalues;    stash->rvalues    = rvalues;
  stash->nsends     = nsends;     stash->nrecvs     = nreceives;
  stash->send_waits = send_waits; stash->recv_waits = recv_waits;
  stash->rmax       = nmax;
  PetscFunctionReturn(0);
}

/* 
   MatStashScatterGetMesg_Private - This function waits on the receives posted 
   in the function MatStashScatterBegin_Private() and returns one message at 
   a time to the calling function. If no messages are left, it indicates this
   by setting flg = 0, else it sets flg = 1.

   Input Parameters:
   stash - the stash

   Output Parameters:
   nvals - the number of entries in the current message.
   rows  - an array of row indices (or blocked indices) corresponding to the values
   cols  - an array of columnindices (or blocked indices) corresponding to the values
   vals  - the values
   flg   - 0 indicates no more message left, and the current call has no values associated.
           1 indicates that the current call successfully received a message, and the
             other output parameters nvals,rows,cols,vals are set appropriately.
*/
#undef __FUNC__  
#define __FUNC__ /*<a name=""></a>*/"MatStashScatterGetMesg_Private"
int MatStashScatterGetMesg_Private(MatStash *stash,int *nvals,int **rows,int** cols,MatScalar **vals,int *flg)
{
  int         i,ierr,size=stash->size,*flg_v,*flg_i,i1,i2,*rindices,match_found=0,bs2;
  MPI_Status  recv_status;

  PetscFunctionBegin;

  *flg = 0; /* When a message is discovered this is reset to 1 */
  /* Return if no more messages to process */
  if (stash->nprocessed == stash->nrecvs) { PetscFunctionReturn(0); } 

  flg_v = stash->nprocs;
  flg_i = flg_v + size;
  bs2   = stash->bs*stash->bs;
  /* If a matching pair of receieves are found, process them, and return the data to
     the calling function. Until then keep receiving messages */
  while (!match_found) {
    ierr = MPI_Waitany(2*stash->nrecvs,stash->recv_waits,&i,&recv_status);CHKERRQ(ierr);
    /* Now pack the received message into a structure which is useable by others */
    if (i % 2) { 
      ierr = MPI_Get_count(&recv_status,MPI_INT,nvals);CHKERRQ(ierr);
      flg_i[recv_status.MPI_SOURCE] = i/2; 
      *nvals = *nvals/2; /* This message has both row indices and col indices */
    } else { 
      ierr = MPI_Get_count(&recv_status,MPIU_MATSCALAR,nvals);CHKERRQ(ierr);
      flg_v[recv_status.MPI_SOURCE] = i/2; 
      *nvals = *nvals/bs2; 
    }
    
    /* Check if we have both the messages from this proc */
    i1 = flg_v[recv_status.MPI_SOURCE];
    i2 = flg_i[recv_status.MPI_SOURCE];
    if (i1 != -1 && i2 != -1) {
      rindices    = (int*)(stash->rvalues + bs2*stash->rmax*stash->nrecvs);
      *rows       = rindices + 2*i2*stash->rmax;
      *cols       = *rows + *nvals;
      *vals       = stash->rvalues + i1*bs2*stash->rmax;
      *flg        = 1;
      stash->nprocessed ++;
      match_found = 1;
    }
  }
  PetscFunctionReturn(0);
}
