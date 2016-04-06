#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <omp.h>
#include "sudoku.h"


extern int thread_count;
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
Board *copyBoard(Board *bd){
	assert(bd);
	Board* new_bd = (Board*)malloc(sizeof(Board));
	*new_bd = *bd;
	return new_bd;
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
int lone_ranger(Board *board){
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
			
			int used = used_vals(i,j,board);
			int k;
			FOR(k,SIZE)
			{
				if(!(used & (1<<k)))
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
			// printf("Calling update %d,%d %d->%d\n",i,last_array[l],board->arr[i][last_array[l]].value,l+1);

			// if(board->arr[i][last_array[l]].value){
			// 	//printf("Calling update %d,%d %d->%d\n",i,last_array[l],board->arr[i][last_array[l]].value,l+1);
			// 	exit(1);
			//}
			//printf("updating...\n");
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
			
			int used = used_vals(j,i,board);
			int k;
			FOR(k,SIZE)
			{
				if(!(used & (1<<k)))
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
			int used = used_vals(i1,j1,board);
			int k;
			FOR(k,SIZE)
			{
				if(!(used & (1<<k)))
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
int prune(Board *board){

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
			int used = used_vals(i,j,board);
			int k, cnt = 0;
			FOR(k,SIZE)
			{
				if(!(used & (1 << k)))
				{
					count_array[k]++;
					last_array[k] = j;
					cnt++;
				}
			}
			if(cnt == 0) return 1;
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
			
			int used = used_vals(j,i,board);
			int k;
			FOR(k,SIZE)
			{
				if(!(used & (1 << k)))
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
			int used = used_vals(i1,j1,board);
			int k;
			FOR(k,SIZE)
			{
				if(!(used & (1 << k)))
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
int getWork(Board *init_bd, Board** work_queue,int *_start){
	int start = 0, end = 0;
	work_queue[start] = init_bd;
	end++;
	int work_queue_size = 1;
	while(work_queue_size > 0 && work_queue_size < thread_count){
    	Board *curr_board = work_queue[start];
    	if(curr_board->fill_count == SIZE*SIZE) 
      		break;
      	work_queue[start] = 0;
      	work_queue_size--;
    	int i,j;
    	int flag = 0;
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
						Board *bd = copyBoard(curr_board);
						work_queue[end] = bd;
						end = (end+1)%work_queue_size;
						updateBoard(i,j,0,curr_board);
					}
				}
				flag = 1;
				break;
			}
			if(flag) break;
		}
      free(curr_board);
   }
   *_start = start;
   return work_queue_size;
}
Board *solveSerial(Stack *work_stack, Board *init_bd){
	return 0;
}

int **solveSudoku(int ** input){

	// TODO set the desited thread_count here
	Board* init_board = (Board*)malloc(sizeof(Board));
	getBoard(input,init_board);
	
	Board *solution = NULL;
	int solved = 0;
	while(eliminate(init_board) || lone_ranger(init_board));
	// fill in the work queue
	
	printf("After initial transformation, %d empty\n",init_board->fill_count);
	Board ** work_queue = (Board**)malloc(sizeof(Board*)*10000);
	int start;
	int work_queue_size = getWork(init_board,work_queue,&start);
	printf("Work queue size: %d\n",work_queue_size);

	Stack *work_stack = (Stack*)malloc(sizeof(Stack)*thread_count);
	int i;
	FOR(i,thread_count){
		allocStack(100000,work_stack+i);
	}

	#pragma omp parallel for schedule(dynamic)
	for(i = 0 ; i < work_queue_size ; i++){
		if(solved) continue;
		printf("tid : %d,  i: %d\n",omp_get_thread_num(),i);
		Board *bd = solveSerial(&work_stack[omp_get_thread_num()],work_queue[((start+i)%work_queue_size)]);
		if(bd){
			#pragma omp critical
			solution = bd;
			solved = 1;
		}
	}
			
		
		
	

	//printf("Not Implemented\n");
	//printf("Pruned : %d Popped: %lld\n",prune_count,global_stack.pop_count);
	
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
