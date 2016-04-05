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
	int row_used[SIZE];
	int col_used[SIZE];
	int grid_used[SIZE];
};
typedef struct _board Board;

inline int grid_id(int i, int j){
	return (i/MINIGRIDSIZE)*MINIGRIDSIZE + j/MINIGRIDSIZE;
}
inline int used_vals(int i, int j,Board *board){
	return (board->grid_used[grid_id(i,j)] | board->row_used[i] | board->col_used[j]); 
}
void print_valid(int i, int j, Board *board){
	
	int used = used_vals(i,j,board);
	int k;
	FOR(k,SIZE){
		if(!(used & (1<<k))) printf("%d ",k+1);
	}
	printf("\n");
}

struct _stack
{
	Board** st_array;
	int top;
	long long pop_count;
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
		st->pop_count++;
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


void updateBoard(int i , int j , int new_val , Board* bd)
{
	//assert(bd->arr[i][j].value == 0);
	int old_val = bd->arr[i][j].value;
	if(new_val == old_val) return;
	if(new_val > 0 && old_val == 0) bd->fill_count++;
	else if(new_val == 0 && old_val > 0) bd->fill_count--;
	else assert(0);
	bd->arr[i][j].value = new_val;
	
	int gid = grid_id(i,j);
	// update the used vals of row,col,grid
	int mask;
	if(new_val > 0){
		mask = 1<<(new_val-1);
		bd->row_used[i] |= mask;
		bd->col_used[j] |= mask;
		bd->grid_used[gid] |= mask;
	}
	else{
		mask = 1<<(old_val-1);
		bd->row_used[i] &= ~mask;
		bd->col_used[j] &= ~mask;
		bd->grid_used[gid] &= ~mask;	
	}
}

void getBoard(int** bd, Board* conv_bd)
{
	conv_bd->fill_count=0;
	int i,j;
	FOR(i,SIZE)
	{
		conv_bd->row_used[i] = 0;
		conv_bd->col_used[i] = 0;
		conv_bd->grid_used[i] = 0;
	}
	FOR(i,SIZE)
	{
		FOR(j,SIZE)
		{	
			updateBoard(i,j,bd[i][j],conv_bd);
		}
	}
}

int eliminate(Board *board){
	int flag1 = 1;
	int i,j;

	FOR(i,SIZE)
	{
		FOR(j,SIZE)
		{
			if(board->arr[i][j].value != 0) continue;
			
			//getValidVals(i,j,valid_moves,board);
			int used = used_vals(i,j,board);
			
			int k,singleton,cnt=0;
			FOR(k,SIZE)
			{
				if(!(used & (1<<k)))
				{
					singleton=k+1;
					cnt++;
				}
			}
			if(cnt==1) 
			{
				updateBoard(i,j,singleton,board);
				flag1=0;
			}
		}
	}
	return (flag1 == 0);
}



int **solveSudoku(int ** input){

	allocStack(100000,&global_stack);
	Board* curr_board = (Board*)malloc(sizeof(Board));
	getBoard(input,curr_board);
	//printf("%d\n",curr_board->row_used[0]);	
	//assert(curr_board->row_used[0] & 1);
	Board *solution = NULL;
	//DFS starts here
	
	omp_lock_t solution_lock;
	omp_init_lock(&solution_lock);
	
	Push(curr_board , &global_stack);
	
	free(curr_board);
	
	curr_board = NULL;
	//int row_perm[SIZE], col_perm[SIZE];
	int idle_counter = 0;
	omp_lock_t idle_counter_lock;
	omp_init_lock(&idle_counter_lock);

	int prune_count = 0;

	#pragma omp parallel firstprivate(curr_board) reduction(+:prune_count)
	{
		int idle = 0;
			
		
		while(1)
		{

			if(solution){
				break;
			}
			
			curr_board = Pop(&global_stack);

			if(!curr_board){
				
				// no work found, if not idle before, increment the idle counter
				if(!idle){
					omp_set_lock(&idle_counter_lock);
					idle_counter++;
					omp_unset_lock(&idle_counter_lock);
					idle = 1;
				}
				omp_set_lock(&idle_counter_lock);
				if(idle_counter == omp_get_num_threads()){
					omp_unset_lock(&idle_counter_lock);
					break;
				}
				omp_unset_lock(&idle_counter_lock);
				continue;
			}
			else{
				// got work to do! if were idle, decrement the idle counter
				if(idle){
					idle = 0;
					omp_set_lock(&idle_counter_lock);
					idle_counter--;
					omp_unset_lock(&idle_counter_lock);
				}
			}	
			int i,j,flag = 0,flag1=0;
			//Heuristic - Elimination
			// till there is a cell that can be filled via elimination

			//Heuristic - Lone Ranger
			//till there is a value which is accepted by only one cell in a row , column or box.
			while(eliminate(curr_board));// || lone_ranger(curr_board,valid_mvs));
			
			if(curr_board->fill_count== SIZE*SIZE){
				omp_set_lock(&solution_lock);
				solution = curr_board;
				omp_unset_lock(&solution_lock);
				break;
			}

			// Main DFS, after no more simplification of the board is possible
			//memset(valid_mvs,0,sizeof(int)*SIZE);

			//Heuristic - Prune
			//Prunes all the branches that will be pruned in future as there are conflicts in them.
			int x = 0;//prune(curr_board , valid_mvs);
			if(x != 0) prune_count++;
			if(x==0)
			{ 
				FOR(i,SIZE)
				{
					FOR(j,SIZE)
					{
						if(curr_board->arr[i][j].value) continue;
						//printf("%d,%d\n",i,j);
						//print_valid(i,j,curr_board);
						//getValidVals(i,j,valid_mvs,curr_board);
						int k,used = used_vals(i,j,curr_board);
						FOR(k,SIZE)
						{
							if(!(used & (1<<k)))
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
			}
			free(curr_board);
			curr_board=NULL;

		}
	}

	//printf("Not Implemented\n");
	printf("Pruned : %d Popped: %lld\n",prune_count,global_stack.pop_count);
	
	if(solution) 
	{
		printf("SOLVED!!!\n");
		return getOutput(solution);
	}
	else
	{
		printf("NO SOLUTION EXISTS !!!\n");
		return input;
	}
}