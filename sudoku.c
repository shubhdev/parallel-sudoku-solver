#include <stdio.h>
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
	st.st_array[st.top+1] = new_bd;
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


int **solveSudoku(int ** board){

	
	printf("Not Implemented\n");
	return board;
}