#include<stdio.h>
#include<stdlib.h>
#include<ucontext.h>
#include "mythread.h"

#define STACK_SIZE 8192

#define NOT_WAITING 0
#define WAIT_ON_JOIN 1
#define WAIT_ON_JOIN_ALL 2

#define TH_ALIVE 1
#define TH_FINISHED 2

struct thread_node{
	struct thread_node *next;
	ucontext_t context;
	struct thread_node *parent;
	int wait_status;
	struct thread_node *waiting_on_thread;
};

struct list_of_threads{
	struct thread_node *node;
	int status;
	struct list_of_threads *next;
};

struct list_of_threads *list = NULL;

struct thread_node *ready_queue_head = NULL, *blocked_queue_head = NULL;
struct thread_node *running_thread;
struct thread_node *temp;
struct thread_node *unix_context;

void add_to_list(struct thread_node *node){
	struct list_of_threads *list_temp = malloc(sizeof(struct list_of_threads));
	struct list_of_threads *list_temp1;
	list_temp->node = node;
	list_temp->next = NULL;
	list_temp->status = TH_ALIVE;
	if(list == NULL) list = list_temp;
	else{
		list_temp1 = list;
		while(list_temp1->next != NULL) list_temp1 = list_temp1->next;
		list_temp1->next = list_temp;
	}

}

void kill_thread(struct thread_node *node){
	struct list_of_threads *list_temp1 = list;
	while(list_temp1->node != node) list_temp1 = list_temp1->next;
	list_temp1->status = TH_FINISHED;
}

int is_terminated(struct thread_node *node){
	struct list_of_threads *list_temp1 = list;
	while(list_temp1->node != node) list_temp1 = list_temp1->next;
	if(list_temp1->status == TH_FINISHED) return 1;
	return 0;

}

void fifo_scheduler(){

	struct thread_node *temp;
	while(ready_queue_head != NULL){
		running_thread = ready_queue_head;
		ready_queue_head = ready_queue_head->next;
		running_thread->next = NULL;
		swapcontext(&unix_context->context,&(running_thread->context));
		if(running_thread != NULL) printf("you have to exit the thread\n");

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
	add_to_list(ready_queue_head);
	getcontext(&(ready_queue_head->context));
	ready_queue_head->context.uc_link			= &unix_context->context;
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
	add_to_list(temp);
	return (MyThread)temp;
}

void MyThreadYield(void){

	struct thread_node *temp1;
	
	temp1 = ready_queue_head;
	while(temp1->next != NULL) 
		temp1 = temp1->next;
	
	temp1->next 	= running_thread;
	running_thread 	= NULL;
	swapcontext(&(temp1->next->context),&unix_context->context);
}

void MyThreadExit(void){
	//ucontext_t to_destroy;
	//swapcontext(&to_destroy,unix_context);
	struct thread_node *temp1,*bl_temp1,*bl_temp2;
	ucontext_t temp2 = running_thread->context;
	 if (((running_thread->parent)->wait_status == WAIT_ON_JOIN) && ((running_thread->parent)->waiting_on_thread == running_thread)){
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
			bl_temp1->next = NULL;
		}

		if(ready_queue_head == NULL) ready_queue_head = running_thread->parent;
		else {
			temp1 = ready_queue_head;
			while(temp1->next != NULL) temp1 = temp1->next;
			temp1->next = running_thread->parent;
		}
	}
	else if( running_thread->parent->wait_status == WAIT_ON_JOIN_ALL){
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
	swapcontext(&temp2,&unix_context->context);
}

int MyThreadJoin(MyThread thread){

	struct thread_node *temp1, *temp2;
	temp = (struct thread_node *)thread;
	int n;

	temp1 = ready_queue_head;
	temp2 = blocked_queue_head;
	while(temp1 != NULL && temp1 != temp) temp1 = temp1->next;
	while(temp2 != NULL && temp2 != temp) temp2 = temp2->next;
	if(is_terminated(temp)){
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
		swapcontext(&(temp1->context),&unix_context->context);
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
			swapcontext(&(temp2->next->context), &unix_context->context);
		}
	}
}




void fib(void *in)
{
	MyThread  T1,T2;
  int *n = (int *)in;	 	/* cast input parameter to an int * */

  if (*n == 0)
    /* pass */;			/* return 0; it already is zero */

  else if (*n == 1)
    /* pass */;			/* return 1; it already is one */

  else {
    int n1 = *n - 1;		/* child 1 param */
    int n2 = *n - 2;		/* child 2 param */

    // create children; parameter points to int that is initialized.
    // this is the location they will write to as well.
    T1 = MyThreadCreate(fib, (void*)&n1);
    T2 = MyThreadCreate(fib, (void*)&n2);
    // after creating children, wait for them to finish
    MyThreadJoin(T1);
    MyThreadJoin(T2);
    //MyThreadJoinAll();
    //  write to addr n_ptr points; return results in addr pointed to
    //  by input parameter
    *n = n1 + n2;
  }

  MyThreadExit();		// always call this at end
}

main(int argc, char *argv[])
{
  int n;

  if (argc != 2) {
    printf("usage: %s <n>\n", argv[0]);
    exit(-1);
  }
  n = atoi(argv[1]);
  if (n < 0 || n > 10) {
    printf("invalid value for n (%d)\n", n);
    exit(-1);
  }

  printf("fib(%d) = ", n);
  MyThreadInit(fib, (void *)&n);
  printf("%d\n", n);
}
