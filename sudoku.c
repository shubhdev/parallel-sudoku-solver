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

int eliminate(Board *board, int *valid_moves){
	int flag1 = 1;
	int i,j;

	FOR(i,SIZE)
	{
		FOR(j,SIZE)
		{
			if(board->arr[i][j].value != 0) continue;
			
			getValidVals(i,j,valid_moves,board);
			int k,singleton,cnt=0;
			FOR(k,SIZE)
			{
				if(!valid_moves[k])
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



int lone_ranger(Board *board, int* valid_moves){
	int flag1 = 1;
	int i,j;
	int count_array[SIZE];
	int last_array[SIZE];

	//traversal in i
	FOR(i,SIZE)
	{
		memset(count_array,0,SIZE*sizeof(int));
		memset(last_array,0,SIZE*sizeof(int));
		
		FOR(j,SIZE)
		{
			if(board->arr[i][j].value != 0) continue;
			
			getValidVals(i,j,valid_moves,board);
			int k;
			FOR(k,SIZE)
			{
				if(!valid_moves[k])
				{
					count_array[k]++;
					last_array[k] = j;
				}
			}
		}
		int l;
		FOR(l,SIZE)
		if(count_array[l]==1) 
		{
			updateBoard(i,last_array[l],l+1,board);
			flag1=0;
		}
	}
	//traversal in j
	FOR(i,SIZE)
	{
		memset(count_array,0,SIZE*sizeof(int));
		memset(last_array,0,SIZE*sizeof(int));
		
		FOR(j,SIZE)
		{
			if(board->arr[j][i].value != 0) continue;
			
			getValidVals(j,i,valid_moves,board);
			int k;
			FOR(k,SIZE)
			{
				if(!valid_moves[k])
				{
					count_array[k]++;
					last_array[k] = j;
				}
			}
		}
		int l;
		FOR(l,SIZE)
		{
			if(count_array[l]==1) 
			{
				updateBoard(last_array[l],i,l+1,board);
				flag1=0;
			}
		}
	}

	//traversal inside box
	FOR(i,SIZE)
	{
		memset(count_array,0,SIZE*sizeof(int));
		memset(last_array,0,SIZE*sizeof(int));
		
		FOR(j,SIZE)
		{
			int i1 = i/MINIGRIDSIZE*MINIGRIDSIZE + j/MINIGRIDSIZE;
			int j1 = i%MINIGRIDSIZE*MINIGRIDSIZE + j%MINIGRIDSIZE;
			if(board->arr[i1][j1].value != 0) continue;		
			getValidVals(i1,j1,valid_moves,board);
			int k;
			FOR(k,SIZE)
			{
				if(!valid_moves[k])
				{
					count_array[k]++;
					last_array[k] = j;
				}
			}
		}
		int l;
		FOR(l,SIZE)
		{
			if(count_array[l]==1) 
			{

				int i2 = i/MINIGRIDSIZE*MINIGRIDSIZE + last_array[l]/MINIGRIDSIZE;
				int j2 = i%MINIGRIDSIZE*MINIGRIDSIZE + last_array[l]%MINIGRIDSIZE;
				updateBoard(i2,j2,l+1,board);
				flag1=0;
			}
		}
	}
	return (flag1 == 0);
}


//Heuristic Prune - Checks on the basis of all the possibilites whether this board will be pruned later
int prune(Board *board, int* valid_moves){

	int flag1 = 1;
	int i,j;
	int count_array[SIZE];
	int last_array[SIZE];

	//traversal in i
	FOR(i,SIZE)
	{
		memset(count_array,0,SIZE*sizeof(int));
		memset(last_array,0,SIZE*sizeof(int));
		FOR(j,SIZE)
		{
			if(board->arr[i][j].value != 0) 
			{
				count_array[board->arr[i][j].value-1]++;
				continue;
			}
			getValidVals(i,j,valid_moves,board);
			int k;
			FOR(k,SIZE)
			{
				if(!valid_moves[k])
				{
					count_array[k]++;
					last_array[k] = j;
				}
			}
		}
		int l;
		FOR(l,SIZE)
		{
			if(count_array[l]==0) 
			{
				return 1;
			}
		}
	}

	//traversal in j
	FOR(i,SIZE)
	{
		memset(count_array,0,SIZE*sizeof(int));
		memset(last_array,0,SIZE*sizeof(int));
		
		FOR(j,SIZE)
		{
			if(board->arr[j][i].value != 0) 
				{
					count_array[board->arr[j][i].value-1]++;
					continue;
				}
			
			getValidVals(j,i,valid_moves,board);
			int k;
			FOR(k,SIZE)
			{
				if(!valid_moves[k])
				{
					count_array[k]++;
					last_array[k] = j;
				}
			}
		}
		int l;
		FOR(l,SIZE)
		{
			if(count_array[l]==0) 
			{
				return 1;
			}
		}
	}

	//traversal inside box
	FOR(i,SIZE)
	{
		memset(count_array,0,SIZE*sizeof(int));
		memset(last_array,0,SIZE*sizeof(int));
		FOR(j,SIZE)
		{
			int i1 = i/MINIGRIDSIZE*MINIGRIDSIZE + j/MINIGRIDSIZE;
			int j1 = i%MINIGRIDSIZE*MINIGRIDSIZE + j%MINIGRIDSIZE;
			if(board->arr[i1][j1].value != 0) 
			{
				count_array[board->arr[i1][j1].value-1]++;
				continue;
			}
			getValidVals(i1,j1,valid_moves,board);
			int k;
			FOR(k,SIZE)
			{
				if(!valid_moves[k])
				{
					count_array[k]++;
					last_array[k] = j;
				}
			}
		}
		int l;
		FOR(l,SIZE)
		{
			if(count_array[l]==0) 
			{
				return 1;
			}
		}
	}
	return 0;
}



int **solveSudoku(int ** input){

	allocStack(100000,&global_stack);
	Board* curr_board = (Board*)malloc(sizeof(Board));
	getBoard(input,curr_board);
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
		#pragma omp single
		printf("Num threads : %d\nStarting parallel....\n",omp_get_num_threads());
		
		int valid_mvs[SIZE];
		
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
			while(eliminate(curr_board,valid_mvs) || lone_ranger(curr_board,valid_mvs));
			
			if(curr_board->fill_count== SIZE*SIZE){
				omp_set_lock(&solution_lock);
				solution = curr_board;
				omp_unset_lock(&solution_lock);
				break;
			}

			// Main DFS, after no more simplification of the board is possible
			memset(valid_mvs,0,sizeof(int)*SIZE);

			//Heuristic - Prune
			//Prunes all the branches that will be pruned in future as there are conflicts in them.
			int x = prune(curr_board , valid_mvs);
			if(x != 0) prune_count++;
			if(x==0)
			{ 
				FOR(i,SIZE)
				{
					FOR(j,SIZE)
					{
						if(curr_board->arr[i][j].value) continue;
						
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