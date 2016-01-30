#include<stdio.h>
#include<stdlib.h>
#include<ucontext.h>
#include "mythread.h"

#define STACK_SIZE 8192

#define NOT_WAITING 0
#define WAIT_ON_JOIN 1
#define WAIT_ON_JOIN_ALL 2

struct thread_node{
	struct thread_node *next;
	ucontext_t context;
	struct thread_node *parent;
	int wait_status;
	struct thread_node *waiting_on_thread;
};
struct thread_node *ready_queue_head = NULL, *blocked_queue_head = NULL;
struct thread_node *running_thread;
struct thread_node *temp;
ucontext_t unix_context;


void fifo_scheduler(){

	struct thread_node *temp;
	while(ready_queue_head != NULL){
		running_thread = ready_queue_head;
		ready_queue_head = ready_queue_head->next;
		running_thread->next = NULL;
		swapcontext(&unix_context,&(running_thread->context));
		if(running_thread != NULL) printf("you have to exit the thread\n");

	}
}

void MyThreadInit(void(*start_funct)(void*), void *args){


	ready_queue_head 					= malloc(sizeof(struct thread_node));
	ready_queue_head->next 				= NULL;
	ready_queue_head->parent  			= NULL;
	ready_queue_head->wait_status		= NOT_WAITING;
	ready_queue_head->waiting_on_thread = NULL;

	getcontext(&(ready_queue_head->context));
	ready_queue_head->context.uc_link			= &unix_context;
	ready_queue_head->context.uc_stack.ss_sp	= malloc(STACK_SIZE);
	ready_queue_head->context.uc_stack.ss_size	= STACK_SIZE;

	makecontext(&(ready_queue_head->context), (void (*)(void))start_funct, 1, args);
	fifo_scheduler();
}

MyThread MyThreadCreate (void (*start_funct)(void *), void *args){

	struct thread_node *temp1;
	temp 					= malloc(sizeof(struct thread_node));
	temp->next 				= NULL;
	temp->parent 			= running_thread;
	temp->wait_status		= NOT_WAITING;
	temp->waiting_on_thread	= NULL;

	getcontext(&(temp->context));
	temp->context.uc_link			= &unix_context;
	temp->context.uc_stack.ss_sp	= malloc(STACK_SIZE);
	temp->context.uc_stack.ss_size	= STACK_SIZE;
	
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
	while(temp1->next != NULL) 
		temp1 = temp1->next;
	
	temp1->next 	= running_thread;
	running_thread 	= NULL;
	swapcontext(&(temp1->next->context),&unix_context);
}

void MyThreadExit(void){
	//ucontext_t to_destroy;
	//swapcontext(&to_destroy,unix_context);
	struct thread_node *temp1,*bl_temp1,*bl_temp2;
	ucontext_t temp2 = running_thread->context;
	 if (running_thread->parent != NULL && (running_thread->parent->wait_status == WAIT_ON_JOIN) && ((running_thread->parent)->waiting_on_thread == running_thread)){
		(running_thread->parent)->wait_status = NOT_WAITING;
		bl_temp1 = blocked_queue_head;
		bl_temp2 = blocked_queue_head;
		while(bl_temp1->next != NULL && bl_temp1 != running_thread->parent){
			bl_temp2 = bl_temp1;
			bl_temp1 = bl_temp1->next;
		}
		if(bl_temp2 == bl_temp1) blocked_queue_head = NULL;
		else{
			bl_temp2->next = bl_temp1->next;
		}

		if(ready_queue_head == NULL) ready_queue_head = running_thread->parent;
		else {
			temp1 = ready_queue_head;
			while(temp1->next != NULL) temp1 = temp1->next;
			temp1->next = running_thread->parent;
		}
	}
	else if( running_thread->parent != NULL && running_thread->parent->wait_status == WAIT_ON_JOIN_ALL){
		temp1 = ready_queue_head;
		while(temp1 != NULL && temp1->parent != running_thread->parent ) temp1 = temp1->next;
		if(temp1 == NULL){
			temp1 = blocked_queue_head;
			while(temp1 != NULL && (temp1->parent != running_thread->parent )) temp1 = temp1->next;
			if(temp1 == NULL){
				if(ready_queue_head == NULL) ready_queue_head = running_thread->parent;
				else{
					temp1 = ready_queue_head;
					while(temp1->next != NULL) temp1 = temp1->next;	
					temp1->next = running_thread->parent;
					running_thread->parent->wait_status = NOT_WAITING;
				}
			}
		}
	}
	free(running_thread);
	running_thread = NULL;
	swapcontext(&temp2,&unix_context);
}

int MyThreadJoin(MyThread thread){

	struct thread_node *temp1, *temp2;
	temp = (struct thread_node *)thread;
	int n;

	temp1 = ready_queue_head;
	temp2 = blocked_queue_head;
	while(temp1 != NULL && temp1 != temp) temp1 = temp1->next;
	while(temp2 != NULL && temp2 != temp) temp2 = temp2->next;
	if(temp1 == NULL && temp2 == NULL){
		printf("Child already terminated\n");
	}

	else if(temp != NULL && temp->parent != running_thread){
		printf("Can only join your own child\n");
	}
	else{
		if(blocked_queue_head == NULL) blocked_queue_head = running_thread;
		else{
			temp1 = blocked_queue_head;
			while(temp1->next != NULL) temp1 = temp1->next;
			temp1->next = running_thread;
		}
		temp1 						= running_thread;
		temp1->wait_status 			= WAIT_ON_JOIN;
		temp1->waiting_on_thread 	= temp;
		temp1->next = NULL;
		running_thread = NULL;
		swapcontext(&(temp1->context),&unix_context);
		return 0;
	}
	return -1;
}

void MyThreadJoinAll(void){
	struct thread_node *temp1, *temp2;

	temp1 = ready_queue_head;
	while(temp1 != NULL && temp1->parent != running_thread) temp1 = temp1->next;
	if(temp1 == NULL){
		temp1 = blocked_queue_head;
		while(temp1 != NULL && temp1->parent != running_thread) temp1 = temp1->next;
	}
	if(temp1 != NULL){
		running_thread->wait_status = WAIT_ON_JOIN_ALL;
		if(blocked_queue_head == NULL) blocked_queue_head = running_thread;
		else {
			temp2 = blocked_queue_head;
			while(temp2->next != NULL) temp2 = temp2->next;
			temp2->next = running_thread;
			running_thread = NULL;
			swapcontext(&(temp2->next->context), &unix_context);
		}
	}
}
int mode = 0;

void t0(void * n)
{
  MyThread T;

  int n1 = (int)n; 
  printf("t0 start %d\n", n1);

  int n2 = n1 -1 ;
  if (n1 > 0) {
    printf("t0 create\n");
    T = MyThreadCreate(t0, (void *)n2);
    if (mode == 1)
      MyThreadYield();
    else if (mode == 2)
      MyThreadJoin(T);
  }
  printf("t0 end\n");
  MyThreadExit();
}

int main(int argc, char *argv[])
{
  int count; 
  
  if (argc < 2 || argc > 3)
    return -1;
  count = atoi(argv[1]);
  if (argc == 3)
    mode = atoi(argv[2]);
  MyThreadInit(t0, (void *)count);
}
