#include <semaphore.h>
#include<stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>
#define maxcount 10000


typedef struct Server{
    int in_use;
    pthread_t server_thread;
    pthread_mutex_t server_lock;
}server;

int riderocc = 0;
int rid = 0;

typedef struct Rider{
    int cabtype; 
    long int maxwait;
    int ridetime;
    int arrivaltime;

    int pno;
    pthread_t rider_thread;
    pthread_mutex_t rider_lock;
    int s_no;
}rider;

int cab_no,rider_no,server_no;


typedef struct Driver{
    int pno;
    int type; 

    int waitstate;
    int onPrem;
    int onPool1;
    int onPool2;

    pthread_mutex_t lock;

}driver;

driver d[maxcount];
rider r[maxcount];
server s[maxcount];

void initialize_driver(int no);


void bookcab(rider *current, driver *pDriver,int * booked) {
    pthread_mutex_lock(&pDriver->lock); 
    if(current->cabtype){ 
        pDriver->onPrem=1;
        printf("Passenger %d is assigned driver %d for Premier ride\n",current->pno,pDriver->pno);
        *booked=1;
    }
    else if(current->cabtype==0){ 
        if(pDriver->onPool1){
            pDriver->onPool2=1;
            pDriver->onPool1=0;
        } else{
            pDriver->onPool1=1;
            pDriver->onPool2=0;
        }
        printf("Passenger %d is assigned driver %d for Pool ride\n",current->pno,pDriver->pno);
        *booked=1;
    }
    else{ 
        *booked=0;
    }
    pDriver->waitstate=0; 
    pthread_mutex_unlock(&pDriver->lock);
}

void * accept_payment(void *arg) {
    int statuspayment=0;
    rider * current=(rider *) arg;
    statuspayment=1;
    printf("Payment of rider %d is being processed \n",current->pno);
    statuspayment=2;
    sleep(2);
    printf("Payment of rider %d is completed \n",current->pno);
    statuspayment=0;
    s[current->s_no].in_use=0;
    return NULL;
}

void *rider_f(void *args) {
    rider *current=(rider *)args;
    int booked=0;

    sleep(current->arrivaltime);
    int i = 1;

    time_t time1;
    time_t time2;
    long int timer=-1;
    time(&time1); 
    while(current->maxwait > timer && !booked){
        i=1;
        int flag = 0;
        for (; i <= cab_no && current->maxwait > timer; ++i) { 
            if (current->cabtype != d[i].type);
            else
            {
                if(current->cabtype){ 
                    if(d[i].waitstate){
                        bookcab(current,&d[i],&booked);
                        if(booked)  flag = 1;
                    }
                }else{
                    if(d[i].onPool1 || d[i].waitstate){
                        bookcab(current, &d[i], &booked);
                        if(booked) flag = 1;
                    }
                }
                pthread_mutex_unlock(&d[i].lock);
            }
            if(flag == 1)   break;
            time(&time2);
            timer=time2-time1; 
            if(current->maxwait > timer)    break;
        }
    }

    if(!booked) {
        printf("The passenger number %d has waited for too long and has hence Timed out \n", current->pno);
        return NULL;
    }

    printf("Rider %d has started his ride with Driver %d\n",current->pno,i);
    riderocc++;
    sleep(current->ridetime);
    riderocc--;
    printf("Rider %d has finished his ride with Driver %d\n",current->pno,i);
    initialize_driver(i);

    i = -1;

    for(;1;){
        i++;
        i%=server_no;
        if(!s[i].in_use && (pthread_mutex_trylock(&s[i].server_lock)==0)){
            s[i].in_use=1;
            current->s_no=i;
            pthread_create(&s[i].server_thread,NULL,accept_payment,current);
            pthread_mutex_unlock(&s[i].server_lock);
            break;
        }
        
    }
    for(;1;){
        int flag = 1 - s[i].in_use;
        if(flag == 1)    break;
    }

    return NULL;
}

void initialize_driver(int no) {
    pthread_mutex_lock(&d[no].lock); 
    if(d[no].onPrem)
    {
        d[no].onPrem=0;
        d[no].onPool1=0;
        d[no].onPool2=0;
        d[no].waitstate=1;
    } 
    if(d[no].onPool1){
        d[no].onPrem=0;
        d[no].onPool1=0;
        d[no].onPool2=0;
        d[no].waitstate=1;
    }
    if(d[no].onPool2){
        d[no].onPool1=1;
        d[no].onPool2=d[no].onPool1 - 1;
    }
    pthread_mutex_unlock(&d[no].lock); 
}

int random_between(int l, int e)
{
    return l + rand() % (e - l + 1);
}

int main(){
    printf("Enter number of cabs, riders and servers :");
    scanf("%d %d %d",&cab_no,&rider_no,&server_no);
    srand(time(NULL));

    for (int m = 0; m < server_no; ++m) {
        s->in_use=0;
        pthread_mutex_init(&s[m].server_lock,NULL);
    }

    int h = cab_no;

    while(h > 0) {  ///initializing the mutex locks of drivers
        pthread_mutex_init(&d[h].lock,NULL);
        d[h].type=random_between(0,1); ///assingns the cab its type
        printf("Driver %d assigned with cabtype %d\n",h,d[h].type);
        d[h].pno=h; ///stores cab number
        d[h].waitstate=1; ///initializes the wait state
        h--;
    }
    h = rider_no;

    while(h > 0) {
        r[h].cabtype=random_between(0,1);
        r[h].ridetime=random_between(1,3);
        r[h].arrivaltime=random_between(1,2);
        r[h].pno=h;
        r[h].maxwait=random_between(1,4);
        rid++;
        pthread_mutex_init(&r[h].rider_lock,NULL);
        printf("Rider %d arrived with cabtype %d,ridetime %d,maxwait time %ld,arrival time %d\n",h,r[h].cabtype,r[h].ridetime,r[h].maxwait,r[h].arrivaltime);
        pthread_create(&r[h].rider_thread,NULL,rider_f,&r[h]);
        rid--;
        h--;

    }
    h = rider_no;
    while(h > 0){///wait till all riders are done
        pthread_join(r[h].rider_thread,NULL);
        h--;
    }

    printf("All riders have been processed \n");
}