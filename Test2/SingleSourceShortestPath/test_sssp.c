//------------------------------------------------------------------------------
// test_sssp: test for LAGraph
//------------------------------------------------------------------------------

// LAGraph, (c) 2021 by The LAGraph Contributors, All Rights Reserved.
// SPDX-License-Identifier: BSD-2-Clause

//------------------------------------------------------------------------------

// Contributed by Jinhao Chen, Scott Kolodziej and Tim Davis, Texas A&M
// University

// Usage:
// test_sssp matrix.mtx sourcenodes.mtx delta
// test_sssp matrix.grb sourcenodes.mtx delta

#include "LAGraph_Test.h"

// #define NTHREAD_LIST 1
// #define NTHREAD_LIST 2
// #define THREAD_LIST 0

#define NTHREAD_LIST 1
#define THREAD_LIST 0

#define LAGRAPH_FREE_ALL            \
{                                   \
    LAGraph_Delete (&G, NULL) ;     \
    GrB_free (&SourceNodes) ;       \
    GrB_free (&pathlen) ;           \
}

int main (int argc, char **argv)
{

    printf ("%s v%d.%d.%d [%s]\n",
        GxB_IMPLEMENTATION_NAME,
        GxB_IMPLEMENTATION_MAJOR,
        GxB_IMPLEMENTATION_MINOR,
        GxB_IMPLEMENTATION_SUB,
        GxB_IMPLEMENTATION_DATE) ;

    char msg [LAGRAPH_MSG_LEN] ;

    LAGraph_Graph G = NULL ;
    GrB_Matrix SourceNodes = NULL ;
    GrB_Vector pathlen = NULL ;

    // start GraphBLAS and LAGraph
    LAGraph_TRY (LAGraph_Init (msg)) ;
    GrB_TRY (GxB_set (GxB_BURBLE, false)) ;

    double tic [2] ;

    //--------------------------------------------------------------------------
    // determine # of threads to use (TODO: make this a Test utility
    //--------------------------------------------------------------------------

    int nt = NTHREAD_LIST ;
    int Nthreads [20] = { 0, THREAD_LIST } ;
    int nthreads_max ;
    LAGraph_TRY (LAGraph_GetNumThreads (&nthreads_max, NULL)) ;
    if (Nthreads [1] == 0)
    {
        // create thread list automatically
        Nthreads [1] = nthreads_max ;
        for (int t = 2 ; t <= nt ; t++)
        {
            Nthreads [t] = Nthreads [t-1] / 2 ;
            if (Nthreads [t] == 0) nt = t-1 ;
        }
    }
    printf ("threads to test: ") ;
    for (int t = 1 ; t <= nt ; t++)
    {
        int nthreads = Nthreads [t] ;
        if (nthreads > nthreads_max) continue ;
        printf (" %d", nthreads) ;
    }
    printf ("\n") ;

    //--------------------------------------------------------------------------
    // read in the graph
    //--------------------------------------------------------------------------

    char *matrix_name = (argc > 1) ? argv [1] : "stdin" ; 
    LAGraph_TRY (LAGraph_Test_ReadProblem (&G, &SourceNodes,
        false, false, false, GrB_INT32, false, argc, argv, msg)) ;
    GrB_Index n, nvals ;
    GrB_TRY (GrB_Matrix_nrows (&n, G->A)) ;
    GrB_TRY (GrB_Matrix_nvals (&nvals, G->A)) ;

    //--------------------------------------------------------------------------
    // get delta
    //--------------------------------------------------------------------------

    int32_t delta ;
    if (argc > 3)
    {
        // usage:  ./test_sssp matrix sourcenodes delta
        delta = atoi (argv [3]) ;
    }
    else
    {
        // usage:  ./test_sssp matrix sourcenodes
        // or:     ./test_sssp < matrix
        delta = 2 ;
    }
    printf ("delta: %d\n", delta) ;

    //--------------------------------------------------------------------------
    // begin tests
    //--------------------------------------------------------------------------

    // get the number of source nodes
    GrB_Index nsource ;
    GrB_TRY (GrB_Matrix_nrows (&nsource, SourceNodes)) ;

    int ntrials = (int) nsource ;

    for (int tt = 1 ; tt <= nt ; tt++)
    {
        int nthreads = Nthreads [tt] ;
        if (nthreads > nthreads_max) continue ;
        LAGraph_TRY (LAGraph_SetNumThreads (nthreads, msg)) ;
        double total_time = 0 ;

        for (int trial = 0 ; trial < ntrials ; trial++)
        {

            //------------------------------------------------------------------
            // get the source node for this trial
            //------------------------------------------------------------------

            // src = SourceNodes [trial]
            GrB_Index src = -1 ;
            GrB_TRY (GrB_Matrix_extractElement (&src, SourceNodes, trial, 0)) ;
            src-- ;     // convert from 1-based to 0-based
            double ttrial ; 

            //------------------------------------------------------------------
            // sssp
            //------------------------------------------------------------------

            GrB_free (&pathlen) ;
            LAGraph_TRY (LAGraph_Tic (tic, msg)) ;
            LAGraph_TRY (LAGraph_SingleSourceShortestPath (&pathlen,
                G, src, delta, true, msg)) ;
            LAGraph_TRY (LAGraph_Toc (&ttrial, tic, msg)) ;

            printf ("sssp15:  threads: %2d trial: %2d source %9lu "
                "time: %10.4f sec\n", nthreads, trial, src, ttrial) ;
            total_time += ttrial ;
            // GxB_print (pathlen, 2) ;
        }

        //----------------------------------------------------------------------
        // report results
        //----------------------------------------------------------------------

        printf ("\n") ;
        double e = (double) nvals ;
        total_time = total_time / ntrials ;
        printf ("%2d: SSSP    time: %14.6f sec  rate: %8.2f (delta %d)\n",
            nthreads, total_time, 1e-6 * e / total_time, delta);
        fprintf (stderr, "Avg: SSSP         %3d: %10.3f sec: %s\n",
             nthreads, total_time, matrix_name) ;
    }

    //--------------------------------------------------------------------------
    // free all workspace and finish
    //--------------------------------------------------------------------------

    LAGRAPH_FREE_ALL ;
    LAGraph_TRY (LAGraph_Finalize (msg)) ;
    return (0) ;
}

