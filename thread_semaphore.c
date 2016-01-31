#include<stdio.h>
#include<stdlib.h>
#include<ucontext.h>
#include "mythread.h"

#define STACK_SIZE 8192 //Stack Size

/* %% Below variables define whether the thread is in blocked queue and why the thread is in blocked queue %*/
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

typedef struct thread_node* Thread;
Thread ready_queue_head = NULL, blocked_queue_head = NULL;
Thread running_thread;
Thread temp;
Thread unix_context;

void add_to_ready_queue(Thread node){

	Thread temp1 = ready_queue_head;
	node->next = NULL;
	if(ready_queue_head == NULL) ready_queue_head = node;

	else {
		temp1 = ready_queue_head;
		while(temp1->next != NULL) temp1 = temp1->next;
		temp1->next = node;
	}
}

void add_to_block_queue(Thread node){
	node->next = NULL;
	if(blocked_queue_head == NULL) blocked_queue_head = node;
	else{
		node->next = blocked_queue_head;
		blocked_queue_head = node;
	}
}

void move_from_blocked_to_ready(Thread node){
	Thread bl_temp1;

	node->wait_status = NOT_WAITING;
	node->waiting_on_thread = NULL;

	bl_temp1 = blocked_queue_head;

	if(blocked_queue_head == node) blocked_queue_head = blocked_queue_head->next;
	else while(bl_temp1->next != NULL && bl_temp1->next != node) bl_temp1 = bl_temp1->next;

	bl_temp1->next = node->next;
	node->next = NULL;

	add_to_ready_queue(node);
}

void fifo_scheduler(){

	while(ready_queue_head != NULL){
		running_thread = ready_queue_head;
		ready_queue_head = ready_queue_head->next;
		running_thread->next = NULL;
		swapcontext(&unix_context->context,&(running_thread->context));
		if(running_thread != NULL)  return ;

	}
}

void MyThreadInit(void(*start_funct)(void*), void *args){


	ready_queue_head 					= malloc(sizeof(struct thread_node));
	unix_context						= malloc(sizeof(struct thread_node));
	ready_queue_head->next 				= NULL;
	unix_context->next					= NULL;
	ready_queue_head->parent  			= unix_context;
	unix_context->parent				= NULL;
	ready_queue_head->wait_status		= NOT_WAITING;
	unix_context->wait_status			= NOT_WAITING;
	ready_queue_head->waiting_on_thread = NULL;
	unix_context->waiting_on_thread		= NULL;
	getcontext(&(ready_queue_head->context));
	ready_queue_head->context.uc_link			= &unix_context->context;
	ready_queue_head->context.uc_stack.ss_sp	= malloc(STACK_SIZE);
	ready_queue_head->context.uc_stack.ss_size	= STACK_SIZE;

	makecontext(&(ready_queue_head->context), (void (*)(void))start_funct, 1, args);
	fifo_scheduler();
}

MyThread MyThreadCreate (void (*start_funct)(void *), void *args){

	Thread temp1;
	temp 					= malloc(sizeof(struct thread_node));
	temp->next 				= NULL;
	temp->parent 			= running_thread;
	temp->wait_status		= NOT_WAITING;
	temp->waiting_on_thread	= NULL;

	getcontext(&(temp->context));
	temp->context.uc_link			= &unix_context->context;
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

	Thread temp1;
	
	temp1 = ready_queue_head;
	while(temp1->next != NULL) 
		temp1 = temp1->next;
	
	temp1->next 	= running_thread;
	running_thread 	= NULL;
	swapcontext(&(temp1->next->context),&unix_context->context);
}

int is_no_child_alive(Thread node){
	Thread temp1,temp2;
	temp1 = ready_queue_head;
	temp2 = blocked_queue_head;
	while(temp1 != NULL && temp1->parent != node) temp1 = temp1->next;
	while(temp2 != NULL && temp2->parent != node) temp2 = temp2->next;
	if(temp1 == NULL && temp2 == NULL) return 1;
	return 0;
}
void MyThreadExit(void){

	ucontext_t temp2 = running_thread->context;
	 if (((running_thread->parent)->wait_status == WAIT_ON_JOIN) && ((running_thread->parent)->waiting_on_thread == running_thread)){
		move_from_blocked_to_ready(running_thread->parent);
	}
	else if( running_thread->parent->wait_status == WAIT_ON_JOIN_ALL && is_no_child_alive(running_thread->parent)){
		move_from_blocked_to_ready(running_thread->parent);
	}
	free(running_thread);
	running_thread = NULL;
	swapcontext(&temp2,&unix_context->context);

}

int MyThreadJoin(MyThread thread){

	Thread temp1,temp2;
	temp = (Thread )thread;

	temp1 = ready_queue_head;
	temp2 = blocked_queue_head;
	while(temp1 != NULL && temp1 != temp) temp1 = temp1->next;
	while(temp2 != NULL && temp2 != temp) temp2 = temp2->next;
	if(temp1 == NULL && temp2 == NULL){
		//printf("Child already terminated\n");
		return 0;
	}

	else if(temp != NULL && temp->parent != running_thread){
		//printf("Can only join your own child\n");
		return -1;
	}
	else{
		temp1 						= running_thread;
		temp1->wait_status 			= WAIT_ON_JOIN;
		temp1->waiting_on_thread 	= temp;
		add_to_block_queue(temp1);
		running_thread = NULL;
		swapcontext(&(temp1->context),&(unix_context->context));
		return 0;
	}
}

void MyThreadJoinAll(void){
	Thread temp1;

	if(!is_no_child_alive(running_thread)){
		temp1 = running_thread;
		temp1->wait_status = WAIT_ON_JOIN_ALL;
		add_to_block_queue(temp1);
		running_thread = NULL;
		swapcontext(&(temp1->context), &(unix_context)->context);
	}
}

struct thread_list{
	Thread thread;
	struct thread_list *next;
};
struct semaphore
{
	int initialValue;
	int semValue;
	struct thread_list *list;
};

MySemaphore MySemaphoreInit(int initialValue){
	struct semaphore *sem = malloc(sizeof(struct semaphore *));
	sem->initialValue = initialValue;
	sem->semValue = initialValue;
	sem->list = NULL;
	return (MySemaphore)sem;
}

void MySemaphoreSignal(MySemaphore sem){
	struct semaphore *temp = (struct semaphore *)sem;
	Thread temp1;
	struct thread_list *temp2;
	temp->semValue--;
	if(temp->semValue < 0){
		//add this to tail of Thread list
		if(temp->list == NULL) {
			temp->list->thread = running_thread;
			temp->list->next = NULL;
		}
		else{
			temp2 = temp->list;
			while(temp2->next != NULL) temp2 = temp2->next;
			(temp2->next)->thread = running_thread;
			(temp2->next)->next = NULL;
		}
		//block this thread	
		add_to_block_queue(running_thread);
		temp1 = running_thread;
		running_thread = NULL;
		swapcontext(&temp1->context, &unix_context->context);
	} 

}

void MySemaphoreWait(MySemaphore sem){
	struct semaphore *temp = (struct semaphore *)sem;
	Thread temp1;
	temp->semValue++;
	if(temp->semValue <= 0){
		//remove head of Thread of list
		temp1 = (temp->list)->thread;
		temp->list = (temp->list)->next;
		//move this thread to ready_queue
		move_from_blocked_to_ready(temp1);
	}
}

int MySemaphoreDestroy(MySemaphore sem){
	struct semaphore *temp = (struct semaphore *)sem;
	if(temp->initialValue != temp->semValue){
		printf("Some threads are using the semaphore\n");
		return -1;
	}
	free(temp);
	return 0;

}