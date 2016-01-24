#include<stdio.h>
#include<stdlib.h>
#include<ucontext.h>

#define STACK_SIZE 8192

#define TH_READY 1
#define TH_RUNNING 2
#define TH_BLOCKED 3

struct thread{
	struct thread *next;
	ucontext_t *context;
	struct thread *parent;
};

struct thread *ready_queue_head = NULL;
struct thread *running_thread;
struct thread *temp;
ucontext_t current_context;
typedef void *MyThread;



MyThread MyThreadCreate (void (*start_funct)(void *), void *args){

	ucontext_t new_context;
	char stack[STACK_SIZE];

	getcontext(&new_context);

	new_context.uc_link				= &current_context;
	new_context.uc_stack.ss_sp		= stack;
	new_context.uc_stack.ss_size	= sizeof(stack);

	makecontext(&new_context, (void (*)(void))start_funct, 1, args);
	getcontext(&current_context);
	
	temp = ready_queue_head;

	while(temp->next != NULL) temp = temp->next;

	temp->next = malloc(sizeof(struct thread));
	temp->next->next 	= NULL;
	temp->next->context = &new_context;
	temp->next->parent	= running_thread;

	
}


void MyThreadInit(void(*start_funct)(void*), void *args){

	ucontext_t new_context;
	char stack[STACK_SIZE];

	getcontext(&new_context);


	new_context.uc_link				= &current_context;
	new_context.uc_stack.ss_sp		= stack;
	new_context.uc_stack.ss_size	= sizeof(stack);

	makecontext(&new_context, (void (*)(void))start_funct, 1, args);
	
	running_thread = malloc(sizeof(struct thread));
	running_thread->next 	= NULL;
	running_thread->context = &new_context;
	running_thread->parent  = NULL;

	running_thread = temp;
	
	swapcontext (&current_context, &new_context);

}
int i=0;
void test_func(void *x){
	
	printf("%d :in test funtion\n");
	MyThreadCreate()

}

int main(){
	//MyThread new_thread;
	int n = 10;
	MyThreadInit(test_func, (void *)&n);

}
