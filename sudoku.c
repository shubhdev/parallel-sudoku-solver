#include <bits/stdc++.h>
#include "sudoku.h"
#include "main.c"




struct cell
{
	int value
};
typedef struct cell Cell ;



struct board
{
	Cell*** bd_array;
	int bd_size;
};
typedef struct board Board;


struct stack
{
	Board** st_array;
	int top;
};

typedef struct stack Stack;



Stack* Sudoku_Bd;



Stack* Pop(Stack* st)
{
	if(top>=0)
	{
		return st->st_array[st->top];
	}
	else return NULL;
}


void Push(Board* new_bd , Stack* st)
{
	st->st_array[st->top+1] = new_bd;
	st->top++;
}