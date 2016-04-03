#include <stdio.h>
#include "sudoku.h"


struct _cell
{
	int value;
};
typedef struct _cell Cell ;



struct _board
{
	Cell bd_array[SIZE][SIZE];
	int bd_size;
};
typedef struct _board Board;


struct _stack
{
	Board** st_array;
	int top;
};

typedef struct _stack Stack;



Stack Sudoku_Bd;



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
int **solveSudoku(int ** board){
	printf("Not Implemented\n");
	return board;
}