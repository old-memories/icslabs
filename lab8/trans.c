/*
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N]) //32X32 and 61X61 use almost the same strategy
                                                              // but 64X64 is different
{
    int i,j;
    int tmp,index;
    int ii,jj;
    if(M==32)
    {   //4X4, one block is 8X8
        for(ii = 0;ii < N ;ii+=8){
            for(jj =0 ;jj < M; jj+=8){
                for(i=ii ; i<ii+8;i++){
                    for(j=jj;j<jj+8;j++){
                        if(i!=j){                  //if diag we store it 
                            B[j][i] = A[i][j];
                        }else{
                            tmp = A[i][j];                  
                            index = i;
                        }
                    }
                    if(ii == jj){           
                        B[index][index] = tmp;
                    }
                }
            }
        }
    }
    else if(M==64)
    { 
		int a1,a2,a3,a4,a5,a6,a7,a8;
		for(ii=0;ii<N/8;ii++)
		{
			for(jj=0;jj<M/8;jj++)
			{    // first  8 as a group
			for (i = 0; i < 4; i++) {
           		a1 = A[i+ii*8][jj*8];
				a2=A[i+ii*8][1+jj*8];
				a3=A[i+ii*8][2+jj*8];
				a4=A[i+ii*8][3+jj*8];
				a5=A[i+ii*8][4+jj*8];
				a6=A[i+ii*8][5+jj*8];
				a7=A[i+ii*8][6+jj*8];
				a8=A[i+ii*8][7+jj*8];
				B[0+jj*8][i+ii*8]=a1;
				B[1+jj*8][i+ii*8]=a2;
				B[2+jj*8][i+ii*8]=a3;
				B[3+jj*8][i+ii*8]=a4;
				B[0+jj*8][i+4+ii*8]=a5;
				B[1+jj*8][i+4+ii*8]=a6;
				B[2+jj*8][i+4+ii*8]=a7;
				B[3+jj*8][i+4+ii*8]=a8;
    			}
			for(i=0;i<4;i++)
			{   //4 as a group
				a5=A[4+ii*8][i+jj*8];
				a6=A[5+ii*8][i+jj*8];
				a7=A[6+ii*8][i+jj*8];
				a8=A[7+ii*8][i+jj*8];
				a1=B[i+jj*8][4+ii*8];
				a2=B[i+jj*8][5+ii*8];
				a3=B[i+jj*8][6+ii*8];
				a4=B[i+jj*8][7+ii*8];
				B[i+jj*8][4+ii*8]=a5;
				B[i+jj*8][5+ii*8]=a6;
				B[i+jj*8][6+ii*8]=a7;
				B[i+jj*8][7+ii*8]=a8;
				B[i+4+jj*8][ii*8]=a1;
				B[i+4+jj*8][1+ii*8]=a2;
				B[i+4+jj*8][2+ii*8]=a3;
				B[i+4+jj*8][3+ii*8]=a4;
				
			}
			for(i=0;i<4;i++)
			{   // 4 as a group
				a1=A[i+4+ii*8][4+jj*8];
				a2=A[i+4+ii*8][5+jj*8];
				a3=A[i+4+ii*8][6+jj*8];
				a4=A[i+4+ii*8][7+jj*8];
				B[4+jj*8][i+4+ii*8]=a1;
				B[5+jj*8][i+4+ii*8]=a2;
				B[6+jj*8][i+4+ii*8]=a3;
				B[7+jj*8][i+4+ii*8]=a4;
			}
			}
					    
		}
    }else{
            //4X4,one block 16X16
        for(ii = 0;ii < N ;ii+=16){
            for(jj =0 ;jj < M; jj+=16){
                for(i=ii ; i<ii+16 && (i<N);i++){
                    for(j=jj;j<jj+16 &&(j<M);j++){
                        if(i!=j){                    //if diag we store it 
                            B[j][i] = A[i][j];
                        }else{
                            tmp = A[i][j];                 
                            index = i;
                        }
                    }
                    if(ii == jj){             
                        B[index][index] = tmp;
                    }
                }
            }
        }

    }
}

    /*
     * You can define additional transpose functions below. We've defined
     * a simple one below to help you get started.
     */

    /*
     * trans - A simple baseline transpose function, not optimized for the cache.
     */
    char trans_desc[] = "Simple row-wise scan transpose";
    void trans(int M, int N, int A[N][M], int B[M][N])
    {
        int i, j, tmp;

        for (i = 0; i < N; i++)
        {
            for (j = 0; j < M; j++)
            {
                tmp = A[i][j];
                B[j][i] = tmp;
            }
        }

    }

    /*
     * registerFunctions - This function registers your transpose
     *     functions with the driver.  At runtime, the driver will
     *     evaluate each of the registered functions and summarize their
     *     performance. This is a handy way to experiment with different
     *     transpose strategies.
     */
    void registerFunctions()
    {
        /* Register your solution function */
        registerTransFunction(transpose_submit, transpose_submit_desc);

        /* Register any additional transpose functions */
        registerTransFunction(trans, trans_desc);

    }

    /*
     * is_transpose - This helper function checks if B is the transpose of
     *     A. You can check the correctness of your transpose by calling
     *     it before returning from the transpose function.
     */
    int is_transpose(int M, int N, int A[N][M], int B[M][N])
    {
        int i, j;

        for (i = 0; i < N; i++)
        {
            for (j = 0; j < M; ++j)
            {
                if (A[i][j] != B[j][i])
                {
                    return 0;
                }
            }
        }
        return 1;
    }


