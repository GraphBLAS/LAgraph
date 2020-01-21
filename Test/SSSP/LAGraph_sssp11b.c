//------------------------------------------------------------------------------
// LAGraph_sssp11: Single source shortest path with delta stepping, same as
// LAGraph_sssp1 but with timing
//------------------------------------------------------------------------------

/*
    LAGraph:  graph algorithms based on GraphBLAS

    Copyright 2019 LAGraph Contributors.

    (see Contributors.txt for a full list of Contributors; see
    ContributionInstructions.txt for information on how you can Contribute to
    this project).

    All Rights Reserved.

    NO WARRANTY. THIS MATERIAL IS FURNISHED ON AN "AS-IS" BASIS. THE LAGRAPH
    CONTRIBUTORS MAKE NO WARRANTIES OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
    AS TO ANY MATTER INCLUDING, BUT NOT LIMITED TO, WARRANTY OF FITNESS FOR
    PURPOSE OR MERCHANTABILITY, EXCLUSIVITY, OR RESULTS OBTAINED FROM USE OF
    THE MATERIAL. THE CONTRIBUTORS DO NOT MAKE ANY WARRANTY OF ANY KIND WITH
    RESPECT TO FREEDOM FROM PATENT, TRADEMARK, OR COPYRIGHT INFRINGEMENT.

    Released under a BSD license, please see the LICENSE file distributed with
    this Software or contact permission@sei.cmu.edu for full terms.

    Created, in part, with funding and support from the United States
    Government.  (see Acknowledgments.txt file).

    This program includes and/or can make use of certain third party source
    code, object code, documentation and other files ("Third Party Software").
    See LICENSE file for more details.

*/

//------------------------------------------------------------------------------
// LAGraph_sssp: Single source shortest path with delta stepping.
// Contributed by Jinhao Chen, Scott Kolodziej and Tim Davis, Texas A&M
// University.
// Adapted from GraphBLAS Template Library (GBTL) by Scott McMillian and Tze
// Meng Low.
//------------------------------------------------------------------------------
// U. Sridhar, M. Blanco, R. Mayuranath, D. G. Spampinato, T. M. Low, and
// S. McMillan, “Delta-Stepping SSSP: From Vertices and Edges to GraphBLAS
// Implementations,” in 2019 IEEE International Parallel and Distributed
// Processing Symposium Workshops (IPDPSW), 2019, pp. 241–250.
// https://ieeexplore.ieee.org/document/8778222/references
// https://arxiv.org/abs/1911.06895
//------------------------------------------------------------------------------
// LAGraph_sssp computes the shortest path lengths from the specified source
// vertex to all other vertices in the graph.
//------------------------------------------------------------------------------

#include "sssp_test.h"

#define LAGRAPH_FREE_ALL                    \
{                                           \
    GrB_free(&AL);                          \
    GrB_free(&AH);                          \
    GrB_free(&t);                           \
    GrB_free(&tmasked);                     \
    GrB_free(&tReq);                        \
    GrB_free(&tless);                       \
    GrB_free(&s);                           \
}

// TODO assert the input matrix has type GrB_INT32
// TODO try different delta (for the GAP matrices)

GrB_Info LAGraph_sssp11b        // single source shortest paths
(
    GrB_Vector *path_length,   // path_length(i) is the length of the shortest
                               // path from the source vertex to vertex i
    GrB_Matrix A,              // input graph, treated as if boolean in
                               // semiring (INT32)
    GrB_Index source,          // source vertex from which to compute
                               // shortest paths
    int32_t delta,             // delta value for delta stepping
    bool AIsAllPositive        // A boolean indicating whether the entries of
                               // matrix A are all positive
)
{
    GrB_Info info;
    int32_t print_lvl = 0; // change to 2 to show the calculation step results

    (*path_length) = NULL;
    GrB_Index nrows, ncols, n = 0; // graph info
    GrB_Index tmasked_nvals = 0;

    GxB_Scalar lBound = NULL; // the threshold for GxB_select
    GxB_Scalar uBound = NULL; // the threshold for GxB_select

    GrB_Matrix AL = NULL;     // graph containing the light weight edges
    GrB_Matrix AH = NULL;     // graph containing the heavy weight edges

    // GrB_INT32 vectors
    GrB_Vector t = NULL;    // tentative shortest path length
    GrB_Vector tmasked = NULL;
    GrB_Vector tReq = NULL;

    // GrB_BOOL vectors
    GrB_Vector tless = NULL;
    GrB_Vector s = NULL;

    double total_time1 = 0;
    double total_time2 = 0;
    double total_time3 = 0;
    double total_time4 = 0;
    double total_time5 = 0;
    double total_time6 = 0;
    double total_time7 = 0;
    double total_time9 = 0;
    double total_time10= 0;
    double tic1[2], tic[2];
    LAGraph_tic(tic1);
    LAGraph_tic(tic);

    if (A == NULL || path_length == NULL)
    {
        // required argument is missing
        LAGRAPH_ERROR ("required arguments are NULL", GrB_NULL_POINTER) ;
    }

    // Get dimensions
    LAGRAPH_OK (GrB_Matrix_nrows (&nrows, A)) ;
    LAGRAPH_OK (GrB_Matrix_ncols (&ncols, A)) ;

    if (nrows != ncols)
    {
        // A must be square
        LAGRAPH_ERROR ("A must be square", GrB_INVALID_VALUE) ;
    }

    n = nrows;

    if (source >= n || source < 0)
    {
        LAGRAPH_ERROR ("invalid value for source vertex", GrB_INVALID_VALUE) ;
    }

    LAGRAPH_OK (GxB_Scalar_new(&lBound, GrB_INT32));
    LAGRAPH_OK (GxB_Scalar_new(&uBound, GrB_INT32));
    LAGRAPH_OK (GxB_Scalar_setElement_INT32(lBound, delta));

    // Create the workspace vectors
    LAGr_Vector_new(&t, GrB_INT32, n);
    LAGr_Vector_new(&tmasked, GrB_INT32, n);
    LAGr_Vector_new(&tReq, GrB_INT32, n);

    LAGr_Vector_new(&tless, GrB_BOOL, n);
    LAGr_Vector_new(&s, GrB_BOOL, n);

    // t = infinity, t[src] = 0
    LAGr_Vector_setElement(t, 0, source);

    // AL = A .* (A <= delta) with lBound = delta
    LAGr_Matrix_new(&AL, GrB_INT32, n, n);
    LAGRAPH_OK (GxB_select(AL, GrB_NULL, GrB_NULL, GxB_LE_THUNK, A, lBound,
        GrB_NULL));
    // GxB_print(AL, print_lvl);

    // AH = A .* (A > delta) with lBound = delta
    LAGr_Matrix_new(&AH, GrB_INT32, n, n);
    LAGRAPH_OK (GxB_select(AH, GrB_NULL, GrB_NULL, GxB_GT_THUNK, A, lBound,
        GrB_NULL));
    GrB_Index nvals = 0;
    LAGr_Matrix_nvals(&nvals, A);
    printf("A has nvals = %"PRIu64"\n", nvals);
    LAGr_Matrix_nvals(&nvals, AL);
    printf("AL has nvals = %"PRIu64"\n", nvals);
    LAGr_Matrix_nvals(&nvals, AH);
    printf("AH has nvals = %"PRIu64"\n", nvals);
    // GxB_print(AH, print_lvl);

    int32_t i = 0;

    // Instead of using tmasked >= i*delta = 0 to find out how many left to be
    // optimized, tmasked can be directly set the same as t since there is only
    // one entry that satisfies the condition
    // Furthermore, set s[src] = true to get the correct result from
    // GxB_PAIR_BOOL in the first loop.
    LAGRAPH_OK (GxB_Scalar_setElement_INT32(lBound, i * delta));
    LAGr_Vector_setElement(tmasked, 0, source);
    LAGr_Vector_setElement(s, true, source);
    // GxB_print(tmasked, print_lvl);

    LAGr_Vector_nvals(&tmasked_nvals, tmasked);

    if (print_lvl > 0)
    {
        fprintf (stderr, "outter tmasked has %ld nnz\n",tmasked_nvals);
    }
    double t1 = LAGraph_toc(tic);
    //printf("pre-handling time %12.6g sec\n", t1);

    //--------------------------------------------------------------------------
    // while (t >= i*delta) not empty
    //--------------------------------------------------------------------------

    while (tmasked_nvals > 0)
    {
        // tmasked = select (tmasked < (i+1)*delta)
        LAGraph_tic (tic);
        LAGRAPH_OK (GxB_Scalar_setElement_INT32(uBound, (i+1) * delta));
        LAGRAPH_OK (GxB_select(tmasked, GrB_NULL, GrB_NULL, GxB_LT_THUNK,
            tmasked, uBound, GrB_NULL));
        t1 = LAGraph_toc(tic);
        total_time1 += t1;

        LAGr_Vector_nvals(&tmasked_nvals, tmasked);

        if (print_lvl > 0)
        {
            fprintf (stderr, "inner tmasked has %ld nnz\n",tmasked_nvals);
        }

        //----------------------------------------------------------------------
        // continue while the current bucket B[i] is not empty
        //----------------------------------------------------------------------

        while (tmasked_nvals > 0)
        {
            // tReq = AL' (min.+) tmasked
            LAGraph_tic (tic);
            LAGr_vxm(tReq, GrB_NULL, GrB_NULL, GxB_MIN_PLUS_INT32,
                tmasked, AL, GrB_NULL);
            t1 = LAGraph_toc(tic);
            total_time2 += t1;
            // GxB_print(tReq, print_lvl);

            // Even though GrB_assign is faster than eWiseAdd here, the total
            // time of getting s with assign and tmasked = (s.*t) later is
            // longer than using eWiseAdd here
            // Time taken here and time to get tmasked =(s.*t) is commented
            // below for kron matrix
            // s = (s | pattern of tmasked)
            LAGraph_tic (tic);

            //printf("----------------------------------------------------\n");
            //GxB_print(s, 2);
            //GxB_print(tmasked, 2);
            // best:0.35+1.17=1.52sec
            LAGr_eWiseAdd (s, NULL, NULL, GxB_PAIR_BOOL, s, tmasked, NULL) ;
            // 2nd best:0.50+1.22=1.72sec
            //LAGr_apply(s, GrB_NULL, GrB_LOR, GxB_ONE_BOOL, tmasked, GrB_NULL);
            // worse:0.14+2.27=2.41sec
            //LAGr_assign (s, NULL, GxB_PAIR_BOOL, tmasked, GrB_ALL, n, NULL) ;
            //GxB_print(s, 2);

            t1 = LAGraph_toc(tic);
            total_time3 += t1;

            // if nnz(tReq) == 0, no need to continue the rest of this loop
            LAGr_Vector_nvals(&tmasked_nvals, tReq);
            if (tmasked_nvals == 0) { break; }

            // tless<tReq> = tReq .< t
            //printf("----------------------------------------------------\n");
            //GxB_print(tReq, 2);
            //GxB_print(t, 2);
            LAGraph_tic (tic);
            LAGr_eWiseAdd(tless, tReq, GrB_NULL, GrB_LT_INT32, tReq,
                t, LAGraph_desc_ooor);
            t1 = LAGraph_toc(tic);
            total_time4 += t1;
            //GxB_print(tless, 2);

            // tmasked<tless> = select (i*delta <= tReq < (i+1)*delta)
            // since all entries of the 5 GAP graphs are known to be positive,
            // and the entries of tmasked are at least i*delta,
            // tReq = tmasked min.+ AL must be >= i*delta.
            // Therefore, there is no need to perform GxB_select with
            // GxB_GE_THUNK to find tmasked >= i*delta from tReq 
            LAGraph_tic (tic);
            LAGRAPH_OK (GxB_select(tmasked, tless, GrB_NULL, GxB_LT_THUNK,
                tReq,    uBound, LAGraph_desc_ooor));
            // For general graph with negative weight, the following needs
            // to be done
            if (!AIsAllPositive)
            {
                LAGRAPH_OK (GxB_select(tmasked, tless, GrB_NULL, GxB_GE_THUNK, 
                    tmasked, lBound, GrB_NULL));
            }
            t1 = LAGraph_toc(tic);
            total_time5 += t1;
            // GxB_print(tmasked, print_lvl);

            // t<tless> = min(t, tReq)
            // GrB_apply is faster than GrB_eWiseAdd or GrB_assign here
            LAGraph_tic (tic);
            LAGr_apply(t, tless, NULL, GrB_IDENTITY_INT32, tReq, NULL);
            //LAGr_assign (t, tless, NULL, tReq, GrB_ALL, n, NULL) ;
            t1 = LAGraph_toc(tic);
            total_time6 += t1;

            LAGr_Vector_nvals(&tmasked_nvals, tmasked);

            if (print_lvl > 0)
            {
                fprintf (stderr, "inner tmasked has %ld nnz\n",tmasked_nvals);
            }
        }

        // tmasked = (t .* s)
        // GrB_apply is faster than GrB_assign here
        LAGraph_tic (tic);
        LAGr_apply(tmasked, s, NULL, GrB_IDENTITY_INT32, t, LAGraph_desc_ooor);
        //LAGr_assign (tmasked, s, NULL, t, GrB_ALL, n, LAGraph_desc_ooor) ;
        t1 = LAGraph_toc(tic);
        total_time10 += t1;

        // tReq = AH'(t .* s)
        LAGraph_tic (tic);
        LAGr_vxm(tReq, GrB_NULL, GrB_NULL, GxB_MIN_PLUS_INT32,
            tmasked, AH, GrB_NULL);
        t1 = LAGraph_toc(tic);
        total_time2 += t1;

        // t = min(t, tReq)
        //printf("----------------------------------------------------\n");
        //GxB_print(t, 2);
        LAGraph_tic (tic);
        LAGr_eWiseAdd(tless, tReq, GrB_NULL, GrB_LT_INT32, tReq,
            t, LAGraph_desc_ooor);
        LAGr_apply(t, tless, GrB_NULL, GrB_IDENTITY_INT32, tReq, GrB_NULL);
        
        //LAGr_eWiseAdd(t, tReq, NULL, GrB_MIN_INT32, t, tReq, NULL);
        //LAGr_eWiseAdd(t, NULL, NULL, GrB_MIN_INT32, t, tReq, NULL);
        t1 = LAGraph_toc(tic);
        total_time9 += t1;
        //GxB_print(t, 2);

        //----------------------------------------------------------------------
        // prepare for the next loop, and find out how many left to be computed
        //----------------------------------------------------------------------

        ++i;

        // tmasked = select (t >= i*delta)
        LAGraph_tic (tic);
        LAGRAPH_OK (GxB_Scalar_setElement_INT32(lBound, i * delta));
        LAGRAPH_OK (GxB_select(tmasked, GrB_NULL, GrB_NULL, GxB_GE_THUNK, t,
            lBound, GrB_NULL));
        t1 = LAGraph_toc(tic);
        total_time7 += t1;
        // GxB_print(tmasked, print_lvl);

        LAGr_Vector_nvals(&tmasked_nvals, tmasked);
        LAGr_Vector_clear(s);// clear s for the next loop

        if (print_lvl > 0)
        {
            fprintf (stderr, "outter tmasked has %ld nnz\n",tmasked_nvals);
        }
    }

    double total_time = LAGraph_toc(tic1);
    printf("total time %12.6g sec\n", total_time);
    printf("select LT time %12.6g sec, ratio %12.6g\n", total_time1,
        total_time1/total_time);
    printf("vxm time %12.6g sec, ratio %12.6g\n", total_time2,
        total_time2/total_time);
    printf("update s time %12.6g sec, ratio %12.6g\n", total_time3,
        total_time3/total_time);
    printf("find tless time %12.6g sec, ratio %12.6g\n", total_time4,
        total_time4/total_time);
    printf("update tmasked time %12.6g sec, ratio %12.6g\n", total_time5,
        total_time5/total_time);
    printf("update t time %12.6g sec, ratio %12.6g\n", total_time6,
        total_time6/total_time);
    printf("select GE time %12.6g sec, ratio %12.6g\n", total_time7,
        total_time7/total_time);
    printf("update t time2 %12.6g sec, ratio %12.6g\n", total_time9,
        total_time9/total_time);
    printf("get tmasked = t .* s time2 %12.6g sec, ratio %12.6g\n",
        total_time10, total_time10/total_time);

    // result = t
    *path_length = t;
    t = NULL;

    LAGRAPH_FREE_ALL;
    return GrB_SUCCESS;
}
