#include<stdio.h>
#include<stdlib.h>
#include<ucontext.h>
//#include "mythread.h"

#define STACK_SIZE 8192

#define TH_READY 1
#define TH_RUNNING 2
#define TH_BLOCKED 3

struct thread_node{
	struct thread_node *next;
	ucontext_t context;
	struct thread_node *parent;
	int status;
};
struct thread_node *ready_queue_head = NULL;
struct thread_node *running_thread;
struct thread_node *temp;
ucontext_t unix_context;
typedef void *MyThread;


void fifo_scheduler(){

	struct thread_node *temp;
	while(ready_queue_head != NULL){
		temp = ready_queue_head;
		ready_queue_head = ready_queue_head->next;
		temp->next = NULL;
		running_thread = temp;
		running_thread->status = TH_RUNNING;
		swapcontext(&unix_context,&(running_thread->context));
		if(running_thread != NULL) printf("you have to exit the thread\n");

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


MyThread MyThreadCreate (void (*start_funct)(void *), void *args){

	struct thread_node *temp1;
	temp = malloc(sizeof(struct thread_node));
	getcontext(&(temp->context));
	temp->parent 					= running_thread;
	temp->context.uc_link			= &unix_context;
	temp->context.uc_stack.ss_sp	= malloc(STACK_SIZE);
	temp->context.uc_stack.ss_size	= STACK_SIZE;
	temp->next = NULL;
	temp->status = TH_READY;
	makecontext(&(temp->context), (void (*)(void))start_funct, 1, args);
	
	if(ready_queue_head == NULL) ready_queue_head = temp;

	else {
		temp1 = ready_queue_head;
		while(temp1->next != NULL) temp1 = temp1->next;
		temp1->next = temp;
	}

	return (MyThread)temp;

}

void MyThreadYield(void){

	struct thread_node *temp1;
	temp1 = ready_queue_head;
	while(temp1->next != NULL) temp1 = temp1->next;
	temp1->next = running_thread;
	running_thread = NULL;
	swapcontext(&(temp1->next->context),&unix_context);
}

void MyThreadExit(void){
	//ucontext_t to_destroy;
	//swapcontext(&to_destroy,unix_context);
	ucontext_t temp2 = running_thread->context;
	running_thread = NULL;
	swapcontext(&temp2,&unix_context);
}

void MyThreadJoin(MyThread thread){
	
}

void test_func3(void *x){
	printf("In funtion 3\n");
}
void test_func2(void *x){
	//MyThread thread1;
	int a=10;
	printf("In test funtion 2\n");
	
	MyThreadYield();
	printf("after the test funtion 2\n");
	MyThreadExit();
}

void test_func(void *x){
	
	MyThread thread1,thread2;
	int a=10;
	printf("In test funtion\n");
	thread1 = MyThreadCreate(test_func2,(void *)&a);
	thread2 = MyThreadCreate(test_func3,(void *)&a);
	printf("after thread creation\n");
	MyThreadYield();
	printf("after the test funtion\n");
	MyThreadExit();


}

int main(){
	//MyThread new_thread;
	int n = 10;
	MyThreadInit(test_func, (void *)&n);
	printf("Back in main\n");

}