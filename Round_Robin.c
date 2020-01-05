#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <semaphore.h>
#define MAX 20
struct thr_queue{
    int* queue;
    int timeAlloc;
    int maxSize;
    int curSize;
    int head;
    int tail;

    pthread_mutex_t q_mut;
    pthread_cond_t q_cond_full;//can push
    pthread_cond_t q_cond_empty;//can pop
};
typedef struct thr_queue* thr_q_ptr;
int q_size(const thr_q_ptr* self);
int q_capacity(const thr_q_ptr* self);
bool empty(thr_q_ptr * q);
int q_create(thr_q_ptr * q,int max,int time);//create queue, 0 good; -1 fail
void q_destroy(thr_q_ptr* q);
int q_top(thr_q_ptr* q);
void q_push(thr_q_ptr* q, int in);
void q_pop(thr_q_ptr* q, int* out);
void showQueue(thr_q_ptr* q,int n);
int nextExecTime(int q);
int nextAvailQ();

struct threadInfo{
    int threadN;
    int arrivalTime;
    int burstTime;
    int priority;
};
static __thread struct threadInfo thread; //every thread has an independent struct copy
static struct threadInfo** thrInfos; //pthread data interface
static thr_q_ptr trd_qs[11]; // 11 queues
static int quantum = 3; // time slice
static int tableAlloc[11] = {17,16,15,11,10,9,7,6,4,3,2}; //default CPU assignment to 11 queues
static int timer = 0; // to simulate clock
static int trd_elu,trd_now = 0,trd_temp; // trd_elu = thread elu a chaque second; trd_now = thread in execution every second
static int threadNum,arrivalTime,burstTime,rtn,totalTime=0;;//rtn is the return value
static int* tableAllocSec;
static int restT_Q[11];
pthread_barrier_t barrier; //barrier
sem_t sem_queue[11]; // semaphore for 11 queues
pthread_mutex_t threadTime;

void* handler(void* info){
    thread = *(struct threadInfo*) info;
    thrInfos[thread.threadN] = &thread;

   // printf("thread infos %d \n",thrInfos[thread.threadN]->burstTime);
    printf("Thread: %d, ID: %ld, Arrival Time: %d, Burst Time: %d, Priority:%d\n",thread.threadN,pthread_self(),thread.arrivalTime,thread.burstTime,thread.priority);
    pthread_barrier_wait(&barrier);
    while(1){
        if(thread.threadN == trd_now)
            pthread_mutex_lock(&threadTime);
        //thread = *thrInfos[thread.threadN];
        if(thread.burstTime==0)
            break;
        if(thread.threadN == trd_now)
            pthread_mutex_unlock(&threadTime);
    }
    printf("Thread %d is killed!\n",thread.threadN);
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

int restTime();
void showTable();

int main(int argc, char* argv[]){
    printf("/* Rappel: Arrival Time  = Date de soumission; Burst Time = Temps d'exécution. */\n");
    printf("/* Attention: Basic time unit is second. */\n");
/*Initialization of threads*/
    struct threadInfo thread;
    do{
    printf("Please enter the number of threads you want to create (at least 11):");
    scanf("%d",&threadNum);
    }while(threadNum<11);
    do{
    printf("Please enter the maximum burst time for each thread:");
    scanf("%d",&burstTime);
    }while(burstTime<=0);
    do{
    printf("Please enter the latest arrival time for all threads:");
    scanf("%d",&arrivalTime);
    }while(arrivalTime<0);

    srand((unsigned)time(NULL));//gene rand seed
    pthread_barrier_init(&barrier,NULL,threadNum+1);// barrier init
    pthread_mutex_init(&threadTime,NULL); // thread time mutex init
    pthread_t pthreadID[threadNum];//pthread id
    thrInfos = (struct threadInfo**)malloc(sizeof(struct threadInfo*)*threadNum);//malloc pthread data interface

    for(int i=0;i<threadNum;i++){
        if(i<11){
            thread.arrivalTime =0;
            thread.priority = i;
        }
        else{
            thread.arrivalTime = rand()%arrivalTime;//[0,arrivalTime)
            thread.priority = rand()%10;//[0,10)
        }
        thread.threadN = i;
        thread.burstTime = rand()%burstTime+1; //(0,burstTime]
        rtn = pthread_create(pthreadID+i,NULL,handler,&thread);
        if(rtn)
            errExit("error on thread creation\n");

    }
    pthread_barrier_wait(&barrier);// barrier to ensure pthread creation
    pthread_barrier_destroy(&barrier);


    printf("Total time of execution is %d\n",restTime());
    printf("%d threads randomly created!\n",threadNum);

    showAllocInfo();
/*end of initialization*/
/*quantum change before execution*/
    char t;
    printf("Do you want to change the current quantum?\n");

    do{
        printf("Please type 'y' or 'n':");
        scanf("%c",&t);

    }while(t!='y'&&t!='n');
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
    do{
        printf("Please type 'y' or 'n':");
        scanf("%c",&t);
    }while(t!='y'&&t!='n');
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
    totalTime = restTime();
//    int help1;//number of quantum
//    float help2= (float)totalTime/(float)quantum,help3;
//    help1=(int)help2;
//    help3=help2-help1;
//    printf("%d %f\n",help1,help3);
//    if(help3!=0.0)
//        help1++;
    tableAllocSec = (int*)malloc(sizeof(int)*totalTime);



/*allouer temps a chaque queue selon le pourcentage indique*/
    int seconds;
    sum=0;
    for(int i=0;i<11;i++)
    {
        seconds=(int)(((float)(tableAlloc[i])/100)*(float)totalTime+0.5);//arrondi mathematique
        //printf("seconds %d\n",seconds);
        rtn=q_create(trd_qs+i, MAX, seconds);
        if(rtn==-1)
            errExit("error on queue creation\n ");
        //printf("%ld\n",trd_qs[i]->maxSize);
        sum+=seconds;
    }
    if((totalTime-sum)==-1 && trd_qs[10]->timeAlloc==0)
    {

        trd_qs[10]->timeAlloc++;
    }
    else if((totalTime-sum)==-1)
    {
        trd_qs[0]->timeAlloc--;//rectifier

    } else if (trd_qs[10]->timeAlloc==0){
        trd_qs[10]->timeAlloc++;
    } else{
        printf("unknown case\n");
    }
    for(int i=0;i<11;i++)
    {
        restT_Q[i]=trd_qs[i]->timeAlloc;
        printf("seconds for queue %d is %d\n",i,trd_qs[i]->timeAlloc);
    }
/*construire table allocation*/
    sum=0;// here means the time
    do{
    for(int i=0;i<11;i++){ // alloc in round robin way
        if(restT_Q[i]>0){
            tableAllocSec[sum]=i;// i means index of queue
            restT_Q[i]--;
            sum++;
        }
    }}while(sum<totalTime);
    for(int i=0;i<totalTime;i++){
        printf("alloc sec %d is %d\n",i,tableAllocSec[i]);
    }
/*end const tab alloc*/
/*end of queue time allocation*/
    showTable();
    int currentQ,countQ=0,quant=quantum;
/*initialization at second 0*/
//    for(int i=0;i<threadNum;i++){
//        if(thrInfos[i]->arrivalTime == 0)
//            q_push(&trd_qs[thrInfos[i]->priority],i);// add threads to queue at sec 0
//    }
/*end init sec 0*/
    printf("Start simulation...\n");
    printf("Recall: Total Time is %d; quantum is %d\n",restTime(),quantum);
/*start ordonnancement*/
    thr_q_ptr requisition;
    // help store same priority thread and preemption at end of quantum(or end of last thread)
    rtn = q_create(&requisition,MAX,0);
    if(rtn)
        errExit("error on preemption queue creation\n");
    bool flag=false;
    for(int i=0;i<threadNum;i++){
        if(thrInfos[i]->arrivalTime == 0){ // if arrived at sec 0
            // priority equals && not time 0
            q_push(&trd_qs[thrInfos[i]->priority],i);// directly into corresponding queue
        }
    }
    while(restTime()>0){ // si temps total reste a executer n'est pas 0
       // trd_now = q_top(&trd_qs[tableAllocSec[timer]]);

        /* add new coming threads to queue */
        if(timer>0){
        for(int i=0;i<threadNum;i++){
            if(thrInfos[i]->arrivalTime == timer){ // if arrived
                // priority equals && not time 0
                if(thrInfos[i]->priority == thrInfos[trd_temp]->priority && timer>0) {
                   q_push(&requisition,i); // wait for preemption
                } else{
                    q_push(&trd_qs[thrInfos[i]->priority],i);// directly into corresponding queue
                }
            }
        }
        }

        if(flag){// switch to the new thread
        /*choose queue*/
//        if(thrInfos[trd_now]->priority != tableAllocSec[timer]){//若这一秒将要执行的队列与分配表不同
//            int swap = tableAllocSec[timer];//swap是本来在这一秒要执行的队列
//            tableAllocSec[timer]  = thrInfos[trd_now]->priority;//borrow second
//            tableAllocSec[nextExecTime(thrInfos[trd_now]->priority)] = swap;//give back second
//        }
//        int current = tableAllocSec[timer];
//        while(empty(&trd_qs[current])){
//            if(current ==11){
//                printf("infinite loop to search queue\n");
//                return 0;
//            }
//            current++;
//        }
            int current =nextAvailQ();
            if( current!= tableAllocSec[timer]){
            int swap = tableAllocSec[timer];//swap是本来在这一秒要执行的队列
            tableAllocSec[timer]  = current;//borrow second
            tableAllocSec[nextExecTime(current)] = swap;//give back second
        }
            trd_now = q_top(&trd_qs[tableAllocSec[timer]]);
            flag=false;
        } else{// stay to the old thread
            if(thrInfos[trd_now]->priority != tableAllocSec[timer]){//若这一秒将要执行的队列与分配表不同
                int swap = tableAllocSec[timer];//swap是本来在这一秒要执行的队列
                tableAllocSec[timer]  = thrInfos[trd_now]->priority;//borrow second
                tableAllocSec[nextExecTime(thrInfos[trd_now]->priority)] = swap;//give back second
            }
        }
        // tableAllocSec[timer];//current queue number selon table allocation
        /*end of choose queue*/
        /*choose thread*/

        if(timer==0 || flag)
            trd_now=trd_elu;

        /* simulation */
        pthread_mutex_lock(&threadTime);
        if(thrInfos[trd_now]->burstTime > 0 && quant>0){ // thread not finished && quantum not finished
            thrInfos[trd_now]->burstTime--; // decrease its life
            printf("Current Time: %d, Thread %d running...Thread %d remaining time:%d\n",timer,trd_now,trd_now,thrInfos[trd_now]->burstTime); // simulate execution
            sleep(1); // simulate execution time
            quant--; // decrease quantum
            trd_temp = trd_now;
            if(thrInfos[trd_now]->burstTime ==0){// end of thread trd_now
                q_pop(&trd_qs[thrInfos[trd_now]->priority],NULL);//delete thread from queue
                flag=true;
            }
        }
        /*end of simulation*/
       // trd_elu=q_top(&trd_qs[tableAllocSec[timer]]);
        /*start of requisition*/
        if(!(thrInfos[trd_now]->burstTime > 0 && quant>0)){ // preemption

            thrInfos[trd_now]->priority--;// decrease current thread's priority
            //trd_now=trd_elu;//update current thread
            quant = quantum; //restore quantum
            while(!empty(&requisition)){
                q_pop(&requisition,&rtn);//get requisition thread
                printf("read from requisition queue: %d\n",rtn);
                q_push(&trd_qs[thrInfos[trd_now]->priority],trd_now);//put into the corresponding queue
            }
            trd_now=q_top(&trd_qs[nextAvailQ()]);
        }
        /*fin de requisition*/
        pthread_mutex_unlock(&threadTime);


        /*^^^last second^^^*/
        timer++;// to next second
        /*vvv- next second -vvv*/

    }
/*end ordonnancement*/

    q_destroy(&requisition);
    for(int i=0;i<11;i++){
        q_destroy(trd_qs+i);
    }
    for(int i=0;i<threadNum;i++){
        rtn = pthread_join(pthreadID[i],NULL); // joining threads
        if(rtn)
            errExit("error on joining thread\n");
    }
    free(tableAllocSec);
    free(thrInfos);
    printf("Simulation finished.\n");
    return 0;
}
int restTime(){
    int temp=0;
    pthread_mutex_lock(&threadTime);
    for(int i=0;i<threadNum;i++)
    {
        if(thrInfos[i])
            temp+=thrInfos[i]->burstTime;
        else
            printf("error! i  = %d, totalTime = %d, threadNum = %d\n",i,temp,threadNum);
    }
    pthread_mutex_unlock(&threadTime);
    return temp;
}
int q_size(const thr_q_ptr* self)
{
    return (*self)->curSize;
}
int q_capacity(const thr_q_ptr* self)
{
    return (*self)->maxSize;
}
bool empty(thr_q_ptr * q){
    struct thr_queue* self=*q;
    if(0 == self->curSize)
        return true;
    else
        return false;
}
int q_create(thr_q_ptr * q,int max,int time)//create queue, 0 good; -1 fail
{
    struct thr_queue* myQueue =(struct thr_queue*)malloc(sizeof(struct thr_queue));
    if(q==NULL)
        return -1;
    myQueue->queue=(int*)malloc(max*sizeof(int));
    if(myQueue->queue==NULL){
        free(myQueue);
        myQueue=NULL;
        return -1;
    }
    myQueue->timeAlloc=time;
    myQueue->maxSize = max;
    myQueue->curSize = myQueue->head = myQueue->tail = 0;
    pthread_mutex_init(&myQueue->q_mut,NULL);
    pthread_cond_init(&myQueue->q_cond_full,NULL);
    pthread_cond_init(&myQueue->q_cond_empty,NULL);
    *q = myQueue;
    return 0;
}
void q_destroy(thr_q_ptr* q)
{
    struct thr_queue* myQueue;
    if(q==NULL||*q==NULL)
        return;
    myQueue= *q;
    pthread_cond_destroy(&myQueue->q_cond_full);
    pthread_cond_destroy(&myQueue->q_cond_empty);
    pthread_mutex_destroy(&myQueue->q_mut);
    free(myQueue->queue);
    free(myQueue);
}
void q_push(thr_q_ptr* q, int in)
{
    struct thr_queue* self=*q;
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
int q_top(thr_q_ptr* q){
    if(empty(q))
        return -1;
    else
        return (*q)->queue[(*q)->head];
}
void q_pop(thr_q_ptr* q, int* out)
{
    struct thr_queue* self= *q;
    pthread_mutex_lock(&self->q_mut);
    while(self->curSize==0)//empty, wait
        pthread_cond_wait(&self->q_cond_empty,&self->q_mut);
    if(out)
        out = &self->queue[self->head];
    self->head++;
    if(self->head == self->maxSize)
        self->head = 0;
    if(self->curSize-- == self->maxSize)//not full, tell push()
        pthread_cond_signal(&self->q_cond_full);
    pthread_mutex_unlock(&self->q_mut);
}
void showQueue(thr_q_ptr* q,int n){
    struct thr_queue* self= *q;
    printf("Queue %d has: ");
    pthread_mutex_lock(&self->q_mut);
    int current = self->head;
    while(1){
        printf("%d, ",self->queue[current]);
        if(++current == self->tail){
            break;
            printf("\n");
        }
        else if(++current == self->maxSize && self->tail<self->head )
            current = 0;
    }
    pthread_mutex_unlock(&self->q_mut);
}
void showTable(){
    printf("Table  ");
    for(int i=0;i<restTime();i++){
        printf("\t%d\t",i);
    }
    printf("\n");
    int temp,lastone=0;
    for(int i=0;i<11;i++){
        printf("%d:    ",i);
        temp=lastone;
        while(temp-->0)
            printf("     ");

        temp=trd_qs[i]->timeAlloc;
        lastone+=trd_qs[i]->timeAlloc;
        printf("|");
        while(temp-->0)
            printf("  x  |");
        printf("\n");
    }
}
int nextExecTime(int q){// success on returning next execution time; return -1 if not found
    int temp=0;
    for(int i=0;i<totalTime;i++){
        if(tableAllocSec[i]==q){
            return i;
        } else
            temp++;
    }
    if(temp == totalTime)
        return -1;
}
int nextAvailQ(){
    int temp=0;
    while(empty(&trd_qs[temp])){
        if(temp ==11){
            printf("infinite loop to search queue\n");
            return 0;
        }
        temp++;
    }
    return temp;
}
