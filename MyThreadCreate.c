#include<stdio.h>
#include<stdlib.h>
#include<ucontext.h>

#define STACK_SIZE 8192

#define TH_READY 1
#define TH_RUNNING 2
#define TH_BLOCKED 3

struct thread_ready_queue{
	struct thread_read_queue *next;
	int status;
	ucontext_t context;
};

thread_read_queue *head = NULL;
typedef void *MyThread;



MyThread MyThreadCreate (void (*start_funct)(void *), void *args){

	ucontext_t new_context, calling_context;
	char stack[STACK_SIZE];

	getcontext(new_context);


	new_context.uc_link				= &calling_context;
	new_context.uc_stack.ss_sp		= stack;
	new_context.uc_stack.ss_size	= sizeof(stack);

	makecontext(&new_context, (void *(void))start_funct, 1, args);
	getcontext(calling_context);
	
	stuct thread_ready_queue new_node, temp;

	temp = head;

	while(temp->next != NULL) temp = temp->next;

	new_node = malloc(sizeof(struct thread_ready_queue));
	new_node->next 		= NULL;
	new_node->context 	= &new_context;
	new_node->status 	= TH_READY;

	temp->next = new_node;

}


void MyThreadInit(void*(start_funct)(void*), void *args){

	ucontext_t new_context, calling_context;
	char stack[STACK_SIZE];

	getcontext(new_context);


	new_context.uc_link				= &calling_context;
	new_context.uc_stack.ss_sp		= stack;
	new_context.uc_stack.ss_size	= sizeof(stack);

	makecontext(&new_context, (void *(void))start_funct, 1, args);
	

	head = malloc(sizeof(struct thread_ready_queue));
	head->next 		= NULL;
	head->context 	= &new_context;
	head->status 	= TH_RUNNING;
	swapcontext (&calling_context, &new_context);

}

void MyThreadJoinAll(){

}


int main(){
	MyThread new_thread;

}
