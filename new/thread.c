#include<stdio.h>
#include<stdlib.h>
#include<ucontext.h>

#define STACK_SIZE 8192

struct thread_node{
	struct thread_node *next;
	ucontext_t context;
	struct thread_node *parent;
};
struct thread_node *ready_queue_head = NULL;
ucontext_t unix_context;

void fifo_scheduler(){

	struct thread_node *temp;
	while(ready_queue_head != NULL){
		temp = ready_queue_head;
		ready_queue_head = ready_queue_head->next;
		temp->next = NULL;
		swapcontext(&unix_context,&(temp->context));
	}
}

void MyThreadInit(void(*start_funct)(void*), void *args){


	ready_queue_head = malloc(sizeof(struct thread_node));
	ready_queue_head->next 	= NULL;
	ready_queue_head->parent  = NULL;
	getcontext(&(ready_queue_head->context));


	ready_queue_head->context.uc_link			= &unix_context;
	ready_queue_head->context.uc_stack.ss_sp	= malloc(STACK_SIZE);
	ready_queue_head->context.uc_stack.ss_size	= STACK_SIZE;

	makecontext(&(ready_queue_head->context), (void (*)(void))start_funct, 1, args);
	fifo_scheduler();
}

void test_func(void *x){
	
	//MyThread thread1;
	int a=10;
	printf("In test funtion\n");
	//thread1 = MyThreadCreate(test_func2,(void *)&a);
	//MyThreadJoin(thread1);
	//printf("after the test funtion\n");

}

int main(){
	//MyThread new_thread;
	int n = 10;
	MyThreadInit(test_func, (void *)&n);

}