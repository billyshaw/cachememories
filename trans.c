/*  A transpose function in trans.c designed to cause as few cache misses as possible */

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
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
	int block_row, block_col;
	int i = 0, j = 0;
	int diag = 0;
	int temp = 0;

	/*
	We use the blocking technique to improve temporal locality of inner loops by defining a sub-matrix of the matrix A 
	with some size b to be a square block. The outer-loops iterate across these block structures, with the two inner loops		 
	iterating through each block.
	*/

	// Case: -M 32 -N 32
	if (M == 32) {
		for (block_col = 0; block_col < N; block_col += 8) {
			for (block_row = 0; block_row < N; block_row += 8) {
				for (i = block_row; i < block_row + 8; i++) {
					for (j = block_col; j < block_col + 8; j++) {						
						if (i != j) {
							B[j][i] = A[i][j];
						} else {
							// Diagonal element
							// Use a temp variable so B[i][j] is not accessed, thus reducing misses
							temp = A[i][j];
							diag = i;
						}
					}

					// Put diagonal elements
					if (block_row == block_col) {							
						B[diag][diag] = temp;
					}
				}	
			}
		}
	}

	// Case: -M 64 -N 64
	if (M == 64) {
		for (block_col = 0; block_col < N; block_col += 4) {
			for (block_row = 0; block_row < N; block_row += 4) {
				for (i = block_row; i < block_row+4; i ++) {
					for (j = block_col; j < block_col+4; j ++) {						
						if (i != j) {
							B[j][i] = A[i][j];
						} else {
							// Diagonal element
							// Use temp variable so that B[i][j] is not accessed, thus reducing misses
							temp = A[i][j];
							diag = i;
						}
					}

					// Put diagonal elements
					if (block_row == block_col) {							
						B[diag][diag] = temp;
					}
				}	
			}
		}
	}
 
	// Case: -M 61 -N 67
	if (M == 61 && N == 67) {
		for (block_col = 0; block_col < M; block_col += 16) {
			for (block_row = 0; block_row < N; block_row += 16) {	
				// Non-square matrix, thus there will be cases when block increment exceed size of matrix A	
				// Thus, need to check i < N and j < M
				for (i = block_row; (i < block_row + 16) && (i < N); i ++) {
					for (j = block_col; (j < block_col + 16) && (j < M); j ++) {
						if (i != j) {
							B[j][i] = A[i][j];
						} else {
							temp = A[i][j];
							diag = i;
						}
					}

					// Put diagonal elements
					if (block_row == block_col) {
						B[diag][diag] = temp;
					}
				}
	 		}
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

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}
