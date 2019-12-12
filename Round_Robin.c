#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
struct threadInfo{
	int threadN;
	int arrivalTime;
	int burstTime;
	int priority;
};
static __thread struct threadInfo thread;
static int quantum = 3;
static int tableAlloc[11] = {17,16,15,11,10,9,7,6,4,3,2}; //default CPU assignment to 11 queues
void* handler(void* info){
	thread = *(struct threadInfo*) info;
	printf("Thread: %d, ID: %ld, Arrival Time: %d, Burst Time: %d, Priority: %d\n",thread.threadN,pthread_self(),thread.arrivalTime,thread.burstTime,thread.priority);
	pthread_exit(NULL);
}
void errExit(const char* ch){
	perror(ch);
	exit(1);
}

void showAllocInfo(){
	printf("The default quantum is %d second(s).\n",quantum);
	printf("The default CPU assignment percentage to 11 queues is shown as follows: \n");
	printf("Queue 0	| Queue 1 | Queue 2 | Queue 3 | Queue 4 | Queue 5 | Queue 6 | Queue 7 | Queue 8 | Queue 9 | Queue 10\n");
	printf("%%%d	  %%%d         %%%d       %%%d       %%%d        %%%d        %%%d       %%%d        %%%d        %%%d        %%%d\n",tableAlloc[0],tableAlloc[1],tableAlloc[2],tableAlloc[3],tableAlloc[4],tableAlloc[5],tableAlloc[6],tableAlloc[7],tableAlloc[8],tableAlloc[9],tableAlloc[10]);
}
int main(int argc, char* argv[]){
	printf("/* Rappel: Arrival Time  = Date de soumission; Burst Time = Temps d'exécution. */\n");
	printf("/* Attention: Basic time unit is second. */\n");
	int threadNum,arrivalTime,burstTime,rtn;
	struct threadInfo thread;
	printf("Please enter the number of threads you want to create:");
	scanf("%d",&threadNum);
	printf("Please enter the maximum burst time for each thread:");
	scanf("%d",&burstTime);
	printf("Please enter the latest arrival time for all threads:");
	scanf("%d",&arrivalTime);

	

	srand((unsigned)time(NULL));
	pthread_t pthreadID[threadNum];
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
	printf("%d threads randomly created!\n",threadNum);
	for(int i=0;i<threadNum;i++){
		rtn = pthread_join(pthreadID[i],NULL);
		if(rtn)
			errExit("error on joining thread\n");
	}
	showAllocInfo();
	return 0;
}
