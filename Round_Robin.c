#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
struct thr_queue{
	int* queue;
	
	int maxSize;
	int curSize;
	int head;
	int tail;
	
	pthread_mutex_t q_mut;
	pthread_cond_t q_cond_full;//can push
	pthread_cond_t q_cond_empty;//can pop
};
int thr_queue_size(const struct thr_queue* self)
{
	return self->curSize;
}
int thr_queue_capacity(const struct thr_queue* self)
{
	return self->maxSize;
}
int q_create(struct thr_queue* q,int max)//create queue, 0 good; -1 fail
{
	struct thr_queue* myQueue =(struct thr_queue*)malloc(sizeof(struct thr_queue));
	if(myQueue==NULL)
		return -1;
	myQueue->queue=(int*)malloc(max*sizeof(int));
	if(myQueue->queue==NULL){
		free(myQueue);
		myQueue=NULL;
		return -1;
	}
	myQueue->maxSize = max;
	myQueue->curSize = myQueue->head = myQueue->tail = 0;
	pthread_mutex_init(&myQueue->q_mut,NULL);
	pthread_cond_init(&myQueue->q_cond_full,NULL);
	pthread_cond_init(&myQueue->q_cond_empty,NULL);
	q = myQueue;
	return 0;
}
void thr_queue_destroy(struct thr_queue* q)
{
	struct thr_queue* myQueue;
	if(q==NULL)
		return;
	myQueue=q;
	pthread_cond_destroy(&myQueue->q_cond_full);
	pthread_cond_destroy(&myQueue->q_cond_empty);
	pthread_mutex_destroy(&myQueue->q_mut);
	free(myQueue->queue);
	free(myQueue);
}
void thr_queue_push(struct thr_queue* q, int in)
{
	struct thr_queue* self=q;
	pthread_mutex_lock(&self->q_mut);
	while(self->curSize == self->maxSize){//full, wait
		pthread_cond_wait(&self->q_cond_full,&self->q_mut);
	}
	self->queue[self->tail++]=in;
	if(self->tail == self->maxSize)
		self->tail=0;
	if(self->curSize++ ==0)//not empty, inform pop()
		pthread_cond_signal(&self->q_cond_empty);
	pthread_mutex_unlock(&self->q_mut);
	
}
int thr_queue_pop(struct thr_queue* q)
{
	struct thr_queue* self=q;
	pthread_mutex_lock(&self->q_mut);
	while(self->curSize==0)//empty, wait
		pthread_cond_wait(&self->q_cond_empty,&self->q_mut);
	return self->queue[self->head];
	self->head++;
	if(self->head == self->curSize)
		self->head = 0;
	if(self->curSize-- == self->maxSize)//not full, tell push()
		pthread_cond_signal(&self->q_cond_full);
	pthread_mutex_unlock(&self->q_mut);
	
}





struct threadInfo{
	int threadN;
	int arrivalTime;
	int burstTime;
	int priority;
};
static __thread struct threadInfo thread;//every thread has an independent struct copy
struct threadInfo** thrInfos;//pthread datas
static struct thr_queue trd_qs[11];//11 queues
static int quantum = 3;
static int tableAlloc[11] = {17,16,15,11,10,9,7,6,4,3,2}; //default CPU assignment to 11 queues
static int currentTime=0;
void* handler(void* info){
	thread = *(struct threadInfo*) info;
	thrInfos[thread.threadN] = &thread;
	printf("Thread: %d, ID: %ld, Arrival Time: %d, Burst Time: %d, Priority:%d\n",thread.threadN,pthread_self(),thread.arrivalTime,thread.burstTime,thread.priority);
	
	pthread_exit(NULL);
}
void errExit(const char* ch){//error handler
	perror(ch);
	exit(1);
}
int t=1;
void showAllocInfo(){
	if(quantum==3)
		printf("The current quantum is %d second(s)(default).\n",quantum);
	else
		printf("The current quantum is %d second(s)(modified).\n",quantum);
	if(t==1){
		printf("The default CPU assignment percentage to 11 queues is shown as follows: \n");
		printf("Queue 0	| Queue 1 | Queue 2 | Queue 3 | Queue 4 | Queue 5 | Queue 6 | Queue 7 | Queue 8 | Queue 9 | Queue 10\n");
		printf("%%%d	  %%%d         %%%d       %%%d       %%%d        %%%d        %%%d       %%%d        %%%d        %%%d        %%%d\n",tableAlloc[0],tableAlloc[1],tableAlloc[2],tableAlloc[3],tableAlloc[4],tableAlloc[5],tableAlloc[6],tableAlloc[7],tableAlloc[8],tableAlloc[9],tableAlloc[10]);
		
		}	
	if(t<=0){
		printf("The current CPU assignment percentage to 11 queues is shown as follows: \n");
		printf("Queue 0	| Queue 1 | Queue 2 | Queue 3 | Queue 4 | Queue 5 | Queue 6 | Queue 7 | Queue 8 | Queue 9 | Queue 10\n");
		printf("%%%d	  %%%d         %%%d       %%%d       %%%d        %%%d        %%%d       %%%d        %%%d        %%%d        %%%d\n",tableAlloc[0],tableAlloc[1],tableAlloc[2],tableAlloc[3],tableAlloc[4],tableAlloc[5],tableAlloc[6],tableAlloc[7],tableAlloc[8],tableAlloc[9],tableAlloc[10]);
		}
		t--;
}

int main(int argc, char* argv[]){
	printf("/* Rappel: Arrival Time  = Date de soumission; Burst Time = Temps d'exécution. */\n");
	printf("/* Attention: Basic time unit is second. */\n");
	int threadNum,arrivalTime,burstTime,rtn;//rtn is the return value
/*Initialization of threads*/
	struct threadInfo thread;
	printf("Please enter the number of threads you want to create:");
	scanf("%d",&threadNum);
	printf("Please enter the maximum burst time for each thread:");
	scanf("%d",&burstTime);
	printf("Please enter the latest arrival time for all threads:");
	scanf("%d",&arrivalTime);

	srand((unsigned)time(NULL));
	pthread_t pthreadID[threadNum];//pthread id
	thrInfos = (struct threadInfo**)malloc(sizeof(struct threadInfo*)*threadNum);//malloc pthread data
	int totalTime=0;
	for(int i=0;i<threadNum;i++){
		thread.threadN = i;
		thread.arrivalTime = rand()%arrivalTime; //[0,threadNum)
		thread.burstTime = rand()%burstTime+1; //(0,burstTime]
		thread.priority = rand()%10;//[0,10)
		rtn = pthread_create(pthreadID+i,NULL,handler,&thread);
		usleep(100);
		if(rtn)
			errExit("error on thread creation\n");
			
	}
	for(int i=0;i<threadNum;i++)
	{
		totalTime+=thrInfos[i]->burstTime;
	}
	printf("totalTime is %d",totalTime);
	printf("%d threads randomly created!\n",threadNum);
	for(int i=0;i<threadNum;i++){
		rtn = pthread_join(pthreadID[i],NULL);
		if(rtn)
			errExit("error on joining thread\n");
	}
	showAllocInfo();
/*end of initialization*/
/*quantum change before execution*/
	char t;
	printf("Do you want to change the current quantum?\n");
	
	while(t!='y'&&t!='n'){
		printf("Please type 'y' or 'n':");
		scanf("%c",&t);
		
		}
	if(t=='y'){
		printf("Please enter the new quantum:");
		scanf("%d",&quantum);
		printf("The new quantum is %d.\n",quantum);
	}
	else
		printf("Stay in default mode.\n"); 
/*end of quantum change*/
/*CPU change before execution*/
	printf("Do you want to change the CPU assignment percentage to 11 queues?\n");
	t='a';
	int num;
	int i=0;
	int tableAlloc_new[11];
	int sum=0;
	while(t!='y'&&t!='n'){
		printf("Please type 'y' or 'n':");
		scanf("%c",&t);
		}
	if(t=='y'){
		while(sum!=100){
			printf("Please enter the 11 new precentage separated by space:");
			while(1){
				scanf("%d",&num);
				char c=getchar();
				tableAlloc_new[i++]=num;
				if(c=='\n'){
					break;
				}
				}	
			for(int i=0;i<11;i++)
				sum+=tableAlloc_new[i];
		}
		for(int i=0;i<11;i++)
				tableAlloc[i]=tableAlloc_new[i];
		
	}
	else
		printf("Stay in default mode.\n"); 
/*end of CPU change*/
	showAllocInfo();


























	free(thrInfos);
	return 0;
}
