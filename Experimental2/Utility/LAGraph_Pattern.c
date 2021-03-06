//------------------------------------------------------------------------------
// LAGraph_Pattern: return the pattern of a matrix (spones(A) in MATLAB)
//------------------------------------------------------------------------------

// LAGraph, (c) 2021 by The LAGraph Contributors, All Rights Reserved.
// SPDX-License-Identifier: BSD-2-Clause
// Contributed by Tim Davis and Scott Kolodziej, Texas A&M University.

//------------------------------------------------------------------------------

// LAGraph_pattern: return the pattern of a matrix as a boolean matrix.

#include "LG_internal.h"

#define LAGRAPH_FREE_ALL GrB_free (C) ;

int LAGraph_Pattern     // return 0 if successful, -1 if failure
(
    GrB_Matrix *C,      // a boolean matrix with the pattern of A
    GrB_Matrix A,
    char *msg
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    LG_CLEAR_MSG ;
    GrB_Index nrows, ncols ;
    LG_CHECK (C == NULL, -1, "&C is NULL") ;
    LG_CHECK (A == NULL, -1, "A is NULL") ;
    (*C) = NULL ;

    //--------------------------------------------------------------------------
    // get the size of A
    //--------------------------------------------------------------------------

    GrB_TRY (GrB_Matrix_nrows (&nrows, A)) ;
    GrB_TRY (GrB_Matrix_ncols (&ncols, A)) ;

    //--------------------------------------------------------------------------
    // C<A,s> = true
    //--------------------------------------------------------------------------

    GrB_TRY (GrB_Matrix_new (C, GrB_BOOL, nrows, ncols)) ;
    GrB_TRY (GrB_assign (*C, A, NULL, (bool) true,
        GrB_ALL, nrows, GrB_ALL, nrows, GrB_DESC_S)) ;

    return (0) ;
}

