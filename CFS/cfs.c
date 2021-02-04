#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>

double vruntime = 0;
double deltaExec = 1;
double nicevalue[5] = {0.64,0.8,1,1.25,1.5625};
int tsnum;
pid_t runpid = 0;

typedef struct NODE{
    double data; // vruntime
    double niceval; // nicevalue 
    pid_t pid;
    struct NODE *next;
}NODE;

NODE* list;
NODE* list2;

void n_init(){
    if(list ==NULL){
        return;
    }
    else{
        NODE* cur;
        cur = list;
        while(cur != NULL){
            list = cur->next;
            free(cur);
            cur = list;
        }
    }
}
void n2_init(){
    if(list2 == NULL){
        return;
    }
    else{
        NODE* cur;
        cur = list2;
        while(cur != NULL){
            list2 = cur->next;
            free(cur);
            cur = list2;
        }
    }
}

void ascending_insert(pid_t pid, double vrtime, double nv){
    NODE* newNode = (NODE*)malloc(sizeof(NODE));
    newNode->data = vrtime;
    newNode->niceval = nv;
    newNode->pid = pid;
    newNode->next = NULL;

    if(list == NULL){
        list = newNode;
    }
    
    else{
        NODE* cur = list;
        NODE* prev = NULL;
        if(cur->data > newNode->data){
            newNode->next = cur;
            list = newNode;
        }
        else{
            while(cur != NULL && cur->data <= newNode->data){
                prev = cur;
                cur = cur->next;
            }
            if(cur != NULL){
                newNode->next = cur;
                prev->next = newNode;
            }
            else{
                prev->next = newNode;
            }
        }
    }
}
void insert2(NODE* n){
    if(list2 == NULL){
        list2 = n;
        n->next = NULL;
    }

}
void del2(NODE* n){
    NODE* cur = n->next;
    list2 = NULL;
}

NODE* first_node(){
    if(list == NULL){
        return NULL;
    }
    else{
        NODE* cur = list->next;
        NODE* prev = list;
        prev->next = NULL;
        list = cur;
        insert2(prev);
        return prev;
    }
}

void timer_handler(){
    kill(list2->pid, SIGSTOP);
    list2->data = list2->data + (list2->niceval);
    
    ascending_insert(list2->pid,list2->data,list2->niceval);
    del2(list2);
    
    if(--tsnum <=0){
        free(list);
        free(list2);
        exit(0);
    }
    NODE* run_p = first_node();
    
    kill(run_p->pid, SIGCONT);
}

int StartTimer(){
    struct itimerval set_time_val;
    struct sigaction act;

    memset(&act,0,sizeof(act));
    act.sa_handler = timer_handler;
    
    sigaction(SIGALRM, &act, NULL); 

    set_time_val.it_value.tv_sec = 1;
    set_time_val.it_value.tv_usec = 0;

    set_time_val.it_interval.tv_sec = 1;
    set_time_val.it_interval.tv_usec = 0;

    setitimer(ITIMER_REAL, &set_time_val, NULL);

    while(1);
}

int main(int argc, char* argv[]){
    int nval1 = atoi(argv[1]);
    int nval2 = atoi(argv[2]);
    int nval3 = atoi(argv[3]);
    int nval4 = atoi(argv[4]);
    int nval5 = atoi(argv[5]);
    tsnum = atoi(argv[6]);

    n_init();
    n2_init();
    int index = 0;
    for(int i = 0; i<5; i++){
        for(int j = 0; j < atoi(argv[i+1]); j++){
            int pid = fork();
            if(pid == 0){
                char arg[2];
                arg[0] = 'A' + index;
                arg[1] = '\0';
                execl("./ku_app", "./ku_app", arg, NULL);
            }
            if(pid != 0){
                ascending_insert(pid,0.0,nicevalue[i]);
                index++;
            }
        }
    }
    sleep(5);
    kill(first_node()->pid,SIGCONT);
    
    StartTimer();

    return 0;
}
