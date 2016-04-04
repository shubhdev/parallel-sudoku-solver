#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <omp.h>
#include "sudoku.h"



#define FOR(i,N) for(i=0;i<N;i++)

struct _cell
{
	int value;
};
typedef struct _cell Cell ;



struct _board
{
	Cell arr[SIZE][SIZE];
	int fill_count;
};
typedef struct _board Board;


struct _stack
{
	Board** st_array;
	int top;
	omp_lock_t lock;
};

typedef struct _stack Stack;



Stack global_stack;



Board* Pop(Stack *st)
{
	Board *res = NULL;
	omp_set_lock(&st->lock);
	if(st->top >= 0)
	{	
		res = st->st_array[st->top--];
	}
	omp_unset_lock(&st->lock);
	return res;
}


void Push(Board* new_bd , Stack *st)
{	
	assert(new_bd !=NULL);
	Board* nbd= (Board*)malloc(sizeof(Board));
	*nbd = *new_bd;
	omp_set_lock(&st->lock);

	st->st_array[st->top+1] = nbd;
	st->top += 1;
	omp_unset_lock(&st->lock);

}



void getBoard(int** bd, Board* conv_bd)
{
	conv_bd->fill_count=0;
	int i,j;
	FOR(i,SIZE)
	{
		FOR(j,SIZE)
		{
			conv_bd->arr[i][j].value = bd[i][j];
			if(bd[i][j]) conv_bd->fill_count++;
		}
	}

}


int** getOutput(Board* bd)
{
	int i,j;
	int** conv_bd = (int**)malloc(SIZE*sizeof(int*));
	FOR(i,SIZE) conv_bd[i]= (int*)malloc(SIZE*sizeof(int));
	
	FOR(i,SIZE)
	{
		FOR(j,SIZE)
		{
			conv_bd[i][j] = bd->arr[i][j].value ;
		}
	}
	return conv_bd;
}


void allocStack(int MAX_SIZE, Stack* st)
{
	st->top = -1;
	st->st_array = (Board**)malloc(MAX_SIZE*sizeof(Board*));
	omp_init_lock(&st->lock);
}


void getValidVals(int x , int y , int* valid_vals,Board* bd)
{
	assert(bd->arr[x][y].value==0);
	memset(valid_vals,0,SIZE*sizeof(int));
	int i;
	//#pragma omp parallel for 

	FOR(i,SIZE)
	{
		if(bd->arr[x][i].value) valid_vals[bd->arr[x][i].value-1] =1; 
		if(bd->arr[i][y].value) valid_vals[bd->arr[i][y].value-1] =1;
		int x1 =(x/MINIGRIDSIZE)*MINIGRIDSIZE + i/MINIGRIDSIZE ;
		int y1 = (y/MINIGRIDSIZE)*MINIGRIDSIZE + i%MINIGRIDSIZE ;
		if(bd->arr[x1][y1].value) valid_vals[bd->arr[x1][y1].value -1] = 1;			
	}
	
}

void updateBoard(int i , int j , int val , Board* bd)
{
	//assert(bd->arr[i][j].value == 0);
	if(val>0 && bd->arr[i][j].value == 0) bd->fill_count++;
	else if(val==0 && bd->arr[i][j].value > 0) bd->fill_count--;
	bd->arr[i][j].value = val;
}


int **solveSudoku(int ** input){

	allocStack(100000,&global_stack);
	Board* curr_board = (Board*)malloc(sizeof(Board));
	getBoard(input,curr_board);
	Board *solution = NULL;
	//DFS starts here
	omp_lock_t solve_lock;
	omp_init_lock(&solve_lock);
	Push(curr_board , &global_stack);
	printf("%d\n",global_stack.top);
	free(curr_board);
	curr_board=NULL;
	#pragma omp parallel firstprivate(curr_board)
	{
		#pragma omp single
		printf("Num threads : %d\nStarting parallel....\n",omp_get_num_threads());

		while(1)
		{
			// printf("running...\n");
			omp_set_lock(&solve_lock);
			if(solution){
				omp_unset_lock(&solve_lock);
				break;
			}
			omp_unset_lock(&solve_lock);
			
			curr_board = Pop(&global_stack);

			if(!curr_board){
				continue;
			}
			if(curr_board->fill_count== SIZE*SIZE){
				printf("SOLVED!!!\n");
				omp_set_lock(&solve_lock);
				solution = curr_board;
				omp_unset_lock(&solve_lock);
				break;
			}
			int i,j,flag = 0;
			FOR(i,SIZE)
			{
				FOR(j,SIZE)
				{
					if(curr_board->arr[i][j].value) continue;
					int valid_mvs[SIZE];
					getValidVals(i,j,valid_mvs,curr_board);
					int k;
					FOR(k,SIZE)
					{
						if(!valid_mvs[k])
						{
							updateBoard(i,j,k+1,curr_board);
							Push(curr_board,&global_stack);
							updateBoard(i,j,0,curr_board);
						}
					}
					flag = 1;
					break;
				}
				if(flag) break;
			}

			free(curr_board);
			curr_board=NULL;

		}
	}

	//printf("Not Implemented\n");
	if(solution) return getOutput(solution);
	else return input;
}