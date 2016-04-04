#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "sudoku.h"



#define FOR(i,N) for(i=0;i<N;i++)

struct _cell
{
	int value;
};
typedef struct _cell Cell ;



struct _board
{
	Cell bd_array[SIZE][SIZE];
};
typedef struct _board Board;


struct _stack
{
	Board** st_array;
	int top;
};

typedef struct _stack Stack;



Stack global_stack;



Board* Pop(Stack st)
{
	if(st.top >= 0)
	{
		return st.st_array[st.top--];
	}
	else return NULL;
}


void Push(Board* new_bd , Stack st)
{
	assert(new_bd !=NULL);
	Board* nbd= (Board*)malloc(sizeof(Board));
	*nbd = *new_bd;

	st.st_array[st.top+1] = nbd;
	st.top++;
}



void getBoard(int** bd, Board* conv_bd)
{

	int i,j;
	FOR(i,SIZE)
	{
		FOR(j,SIZE)
		{
			conv_bd->bd_array[i][j].value = bd[i][j];
		}
	}

}


void getOutput(Board* bd , int** conv_bd)
{
	int i,j;
	FOR(i,SIZE)
	{
		FOR(j,SIZE)
		{
			conv_bd[i][j] = bd->bd_array[i][j].value ;
		}
	}

}


void allocStack(int MAX_SIZE, Stack* st)
{
	st->st_array = (Board**)malloc(MAX_SIZE*sizeof(Board*));
}


void getValidVals(int x , int y , int* valid_vals,Board* bd)
{
	assert(bd->bd_array[x][y].value==0);
	int i;
	#pragma omp parallel for 

	FOR(i,SIZE)
	{
		if(bd->bd_array[x][i].value) valid_vals[bd->bd_array[x][i].value-1] =1; 
		if(bd->bd_array[i][y].value) valid_vals[bd->bd_array[i][y].value-1] =1;
		int x1 =((x+1)/MINIGRIDSIZE)*MINIGRIDSIZE + (i)/MINIGRIDSIZE ;
		int y1 = ((y+1)/MINIGRIDSIZE)*MINIGRIDSIZE + (i)%MINIGRIDSIZE ;
		if(bd->bd_array[x1][y1].value) valid_vals[bd->bd_array[x1][y1].value -1] = 1;			
	}
	
}



int **solveSudoku(int ** board){

	allocStack(100000,&global_stack);
	printf("Not Implemented\n");
	return board;
}