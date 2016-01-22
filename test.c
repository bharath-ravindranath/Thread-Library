#include<stdio.h>
#include<stdlib.h>
#include<ucontext.h>

ucontext_t parent, child;

void function_to_test_swapcontext(int i){
	
	printf("In Function setcontext\n");
}

int main(){
	getcontext(&child);
	char stack[8192];
	child.uc_link		= &parent;
	child.uc_stack.ss_sp	= stack;
	child.uc_stack.ss_size	= sizeof(stack);
	
	makecontext(&child, (void(*)(void))function_to_test_swapcontext, 1, 100);
	
	
	printf("Calling the funtion using setcontext\n");
	swapcontext(&parent, &child);

	printf("Back in main\n");

	return 0;

	
}
