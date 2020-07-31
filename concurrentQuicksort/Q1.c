#define _POSIX_C_SOURCE 199309L //required for clock
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <inttypes.h>
#include <math.h>


struct arg{
    int l;
    int r;
    int* A;    
};

int * shareMem(size_t size){
    key_t mem_key = IPC_PRIVATE;
    int shm_id = shmget(mem_key, size, IPC_CREAT | 0666);
    return (int*)shmat(shm_id, NULL, 0);
}

int B[1000000+1];


void swap(int *a,int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

int part(int A[],int l,int r)
{
    int x = A[r];
    int i = l-1;
    for(int j=l;j<=r;++j)
    {
        if(A[j]<x)
        {
            i++;
            swap(&A[i],&A[j]);
        }
    }
    i++;
    swap(&A[i],&A[r]);
    return i;
}

void quickSort(int A[], int l, int r)  
{  
    if (l < r)  
    {  
        if(r-l+1<=5){            for(int i=l;i<=r;i++)   for(int j=i+1;j<=r;j++)        if(A[j]<A[i])    swap(&A[j],&A[i]);}
        else
        {
            int p = part(A, l, r);  
            quickSort(A, l, p - 1);  
            quickSort(A, p + 1, r);  
        }
    }  
}  


void doqsort(int *A, int l, int r)
{
    if(l < r)
    {
        if(r-l+1<=5){            for(int i=l;i<=r;i++)       for(int j=i+1;j<=r;j++)         if(A[j]<A[i])    swap(&A[j],&A[i]);}
        else
        {
            int status;

            int pid1,pid2;
            int p = part(A,l,r);
            pid1 = fork();
            if(pid1==0){
                doqsort(A, l, p-1);
                _exit(1);
            }
            else{
                pid2 = fork();
                if(pid2==0){
                    doqsort(A,p+1,r);
                    _exit(1);
                }
                else{
                    waitpid(pid1, &status, 0);
                    waitpid(pid2, &status, 0);
                }
            }
        }
    }  
}


void *threaded_doqsort(void* a){
    struct arg *args = (struct arg*) a;

    int l = args->l;
    int r = args->r;
    int *A = args->A;
    if(l < r)
    {
        if(r-l+1<=5){    for(int i=l;i<=r;i++)       for(int j=i+1;j<=r;j++)         if(A[j]<A[i])    swap(&A[j],&A[i]);}
        else
        {
            pthread_t tid1,tid2;

            int p = part(A,l,r);
            struct arg a1;
            a1.l = l;
            a1.r = p-1;
            a1.A = A;
            pthread_create(&tid1, NULL, threaded_doqsort, &a1);
            
            struct arg a2;
            a2.l = p+1;
            a2.r = r;
            a2.A = A;
            pthread_create(&tid2, NULL, threaded_doqsort, &a2);
            
            pthread_join(tid1, NULL);
            pthread_join(tid2, NULL);
        }
    }
    return NULL;
}

void runSorts(long long int n){
    
    int *A = shareMem(sizeof(int)*(n+1));
    for(int i=0;i<n;i++) A[i] = n - i;

    struct timespec ts;

    printf("Running concurrent_quicksort\n");
    clock_gettime(CLOCK_MONOTONIC, &ts);
    long double st = ts.tv_nsec/(1e9)+ts.tv_sec;
    long double t1,t2,t3;
    doqsort(A, 0, n-1);
    pthread_t tid;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    long double en = ts.tv_nsec/(1e9)+ts.tv_sec;
    printf("time = %Lf\n", en - st);
    t1 = en-st;

    for(int i=0;i<n;i++) B[i] = A[i];


    struct arg a;
    a.l = 0;
    a.r = n-1;
    a.A = B;
    printf("Running threaded_concurrent_quicksort\n");
    clock_gettime(CLOCK_MONOTONIC, &ts);
    st = ts.tv_nsec/(1e9)+ts.tv_sec;

    pthread_create(&tid, NULL, threaded_doqsort, &a);
    pthread_join(tid, NULL);    
    
    clock_gettime(CLOCK_MONOTONIC, &ts);
    en = ts.tv_nsec/(1e9)+ts.tv_sec;
    printf("time = %Lf\n", en - st);
    t2 = en-st;

    printf("Running normal_quicksort\n");
    clock_gettime(CLOCK_MONOTONIC, &ts);
    st = ts.tv_nsec/(1e9)+ts.tv_sec;

    quickSort(B, 0, n-1);    
    shmdt(A);

    clock_gettime(CLOCK_MONOTONIC, &ts);
    en = ts.tv_nsec/(1e9)+ts.tv_sec;
    printf("time = %Lf\n", en - st);
    t3 = en - st;

    printf("normal_quicksort ran:\n\t[ %Lf ] times faster than concurrent_quicksort\n\t[ %Lf ] times faster than threaded_concurrent_quicksort\n\n\n", t1/t3, t2/t3);
}

int main(){

    long long int n;
    scanf("%lld", &n);
    runSorts(n);
    return 0;
}