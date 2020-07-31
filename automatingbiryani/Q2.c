#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
#include<unistd.h>
#include<time.h>
#include <sys/shm.h>

int m,n,k;

int studentsremain;
struct robot_chef{
     int index;
     int preptime;
     int num_vessels;
     int capacity;
} * chefs;

pthread_mutex_t *mutex_for_chefs,*mutex_for_tables;

struct serving_table{
     int index;
     int serving_container;
     int slots;
} * tables;

int stackofavailabletables[1000];
int availabletables = -1;

struct students{
     int index;
     int status;
} * students;


void biryani_ready(int index){
     while(chefs[index].num_vessels){
          for(int j=0; j<n; j++){
               if(chefs[index].num_vessels <= 0){
                    break;
               }
               if(pthread_mutex_trylock(&mutex_for_tables[j])){
                    continue;
               }
               if(tables[j].serving_container==0){
                    printf("Chef %d served table %d with capacity %d\n",chefs[index].index,tables[j].index,chefs[index].capacity);
                    chefs[index].num_vessels-=1;
                    tables[j].serving_container=chefs[index].capacity;
               }
               pthread_mutex_unlock(&mutex_for_tables[j]);
          }
     }
}

void student_in_slot(int j){
     tables[j].serving_container-=1;
     tables[j].slots-=1;
     pthread_mutex_unlock(&mutex_for_tables[j]);
     studentsremain--;
     while(tables[j].slots!=0 && studentsremain!=0){
     }
}

void ready_to_serve_table(int index){
     int flag = 0;
     while(flag == 0){
          if(pthread_mutex_trylock(&mutex_for_tables[index])){
               continue;
          }
          if(studentsremain==0)    flag = 1;
          if(tables[index].slots==0)      flag = 1;
          pthread_mutex_unlock(&mutex_for_tables[index]);
     }
}

void * chef_init(void * arg){
     int *index_pointer = (int*) arg;
     int index = *index_pointer;
     int flag3 = 10;
     while(flag3 == 10){
          chefs[index].num_vessels=1+(rand()%10);
          chefs[index].capacity=25+(rand()%26);
          chefs[index].preptime=2+(rand()%4);
          sleep(chefs[index].preptime);
          printf("Chef %d has prepared %d vessels which feed %d people in %d seconds\n",chefs[index].index,chefs[index].num_vessels,chefs[index].capacity,chefs[index].preptime);

          biryani_ready(index);
          if(studentsremain==0)    flag3 = -1;
     }
    
     return NULL;
}
void * table_init(void * arg){
     int *index_pointer = (int*) arg;
     int index = *index_pointer;
     
     while(1){
          if(studentsremain==0){
               break;
          }
          if(tables[index].serving_container>0){
               
               tables[index].slots=1+(rand()%10);
               if(tables[index].serving_container < tables[index].slots)   tables[index].slots = tables[index].serving_container;
               printf("Table %d serving with %d slots and has capacity %d\n",tables[index].index,tables[index].slots,tables[index].serving_container);
               fflush(stdout);
               ready_to_serve_table(index);
               printf("Table %d has finished its container\n",index);
               fflush(stdout);
          }
     }

     return NULL;
}

void * wait_for_slot(void * arg){
     int *index_pointer = (int*) arg;
     int index = *index_pointer;
     int flag2 = 10;
     printf("Student %d is waiting to be allocated a slot on the serving table\n",index);
     fflush(stdout);
     while(flag2 == 10){
          for(int j=0; j<n; j++){
               if(pthread_mutex_trylock(&mutex_for_tables[j])){
                    continue;
               }
               if(tables[j].slots>0){
                    printf("Student %d is going to eat at %d\n",students[index].index,tables[j].index);
                    fflush(stdout);
                    student_in_slot(j);
                    flag2 = -1;
                    printf("Student %d has finished eating at %d\n",students[index].index,tables[j].index);
                    fflush(stdout);
                    break;
               }
               pthread_mutex_unlock(&mutex_for_tables[j]);
          }
     }

     return NULL;
}

int main(void){
     srand(time(NULL));
     printf("Enter the number of Robot chefs, Serving Tables and Students: \n");
     scanf("%d %d %d",&m,&n,&k);
     pthread_t timid[m],tabletimid[n],studenttimid[k];

     mutex_for_tables=(pthread_mutex_t *)malloc((n)*sizeof(pthread_mutex_t));
     studentsremain=k;
     chefs=(struct robot_chef*)malloc(sizeof(struct robot_chef)*(m));
     mutex_for_chefs=(pthread_mutex_t *)malloc((m)*sizeof(pthread_mutex_t));


          
     for(int i=0; i<m; i++){
          chefs[i].index=i;
          pthread_create(&timid[i],NULL,chef_init,(void *)&chefs[i].index);
          usleep(100);
     }
     sleep(1);
     tables=(struct serving_table*)malloc(sizeof(struct serving_table)*(n));
     for(int i=0; i<n; i++){
          tables[i].index=i;
          pthread_create(&tabletimid[i],NULL,table_init,(void *)&tables[i].index);
          usleep(100);
     }
     sleep(1);
     students=(struct students*)malloc(sizeof(struct students)*(k));
     for(int i=0; i<k; i++){
          students[i].index=i;
          printf("Student %d has arrived\n",i);
          pthread_create(&studenttimid[i],NULL,wait_for_slot,(void *)&students[i].index);
          usleep(100);
     }
     for(int i=0; i<k; i++){
          pthread_join(studenttimid[i],NULL);
     }

     printf("Simulation Over\n");
     return 0;
}
