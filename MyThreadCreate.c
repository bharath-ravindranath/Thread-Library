#include<stdio.h>
#include<stdlib.h>
#include<ucontext.h>

#define STACK_SIZE 8192

#define TH_READY 1
#define TH_RUNNING 2
#define TH_BLOCKED 3

struct thread_node{
	struct thread_node *next;
	ucontext_t context;
	struct thread_node *parent;
};

struct thread_node *ready_queue_head = NULL;
struct thread_node *running_thread, uc_link;
struct thread_node *temp = NULL;
ucontext_t unix_context;
typedef void *MyThread;



MyThread MyThreadCreate (void (*start_funct)(void *), void *args){

	struct thread_node *temp1;
	temp = malloc(sizeof(struct thread_node));
	getcontext(&(temp->context));
	temp->parent 					= running_thread;
	temp->context.uc_link			= &unix_context;
	temp->context.uc_stack.ss_sp	= malloc(STACK_SIZE);
	temp->context.uc_stack.ss_size	= STACK_SIZE;
	temp->next = NULL;
	makecontext(&(temp->context), (void (*)(void))start_funct, 1, args);
	
	if(ready_queue_head == NULL) ready_queue_head = temp;

	else {
		temp1 = ready_queue_head;
		while(temp1->next != NULL) temp1 = temp1->next;
		temp1->next = temp;
	}

	return (MyThread)temp;

}

void MyThreadJoin(MyThread thread){

	temp = (struct thread_node *)thread;
	if(temp->parent == running_thread) {
		temp->context.uc_link = &(unix_context);
		swapcontext(&unix_context,&(temp->context));
		getcontext(&(running_thread->context));
		printf("After getcontext");
		swapcontext(&unix_context,&(running_thread->context));
	}
}



void MyThreadInit(void(*start_funct)(void*), void *args){


	running_thread = malloc(sizeof(struct thread_node));
	running_thread->next 	= NULL;
	running_thread->parent  = NULL;
	getcontext(&(running_thread->context));


	running_thread->context.uc_link				= &unix_context;
	running_thread->context.uc_stack.ss_sp		= malloc(STACK_SIZE);
	running_thread->context.uc_stack.ss_size	= STACK_SIZE;

	makecontext(&(running_thread->context), (void (*)(void))start_funct, 1, args);
	
	swapcontext ( &unix_context, &(running_thread->context));

}

void test_func2(void *x){
	printf("in test funtion 2\n");
}

void test_func(void *x){
	
	MyThread thread1;
	int a=10;
	printf("In test funtion\n");
	thread1 = MyThreadCreate(test_func2,(void *)&a);
	MyThreadJoin(thread1);
	printf("after the test funtion\n");

}

int main(){
	//MyThread new_thread;
	int n = 10;
	MyThreadInit(test_func, (void *)&n);

}
