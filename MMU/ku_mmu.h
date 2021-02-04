#include <stdio.h>
#include <stdlib.h>
#define MAX 100

typedef struct ku_pte{
    char pte_bit;
}Ku_pte;

Ku_pte* pmem;
Ku_pte* swap;
int pmem_freelist[64];
int swap_freelist[64];

int queue[MAX];
int front = -1;
int rear = -1;

int jw_mem_size;
int jw_swap_size;

void addq(int value){
    int tmp = (rear+1)%100;
    if(tmp == front){
        return 0;
    }
    else{
        rear = (rear + 1) % 100;
        queue[rear] = value;
    }
}
int deleteq(){
    if(rear == front){
        return 0;
    }
    else{
        front = (front +1) %MAX;
        return queue[front];
    }
}


typedef struct NODE{
    char pcb_pid;
    long long pcb_pdbr;
    struct NODE *next;
}Node;

typedef struct list{
    Node *head;
    Node *tail;
}List;

List *list;


void node_insert(char pid, long long pdbr){
    Node *newnode = (Node*)malloc(sizeof(Node));
    
    newnode->pcb_pid = pid;
    newnode->pcb_pdbr = pdbr;
    newnode->next = NULL;
    
    if(list->head == NULL && list->tail == NULL){
        list->head = newnode;
        list->tail = newnode;
    }
    else{
        list->tail->next = newnode;
        list->tail = newnode;
    }
}
long long check_pid(char pid){
    Node* curr = list->head;
    while(curr != NULL){
        if(curr->pcb_pid == pid){
            return curr->pcb_pdbr; //중복된 process의 pcbr 리턴
        }
        else{
            curr = curr->next;
        }
    }
    return 0; //중복 없음  
}
char pt_index(char va){ // bit operation for pt
    char b = va & 12;
    char result = b >> 2;
    return result;
}
char pmd_index(char va){ // bit operation for pmd
    char b = va & 48;
    char result = b >> 4;
    return result;
}
char pd_index(char va){ // bit operation for pd
    char b = va & 192;
    char result = b >> 6;
    return result;
}
Ku_pte cal_pfn(int index){// 
    Ku_pte k;
    k.pte_bit = (index << 2) + 1; // pfn index + present bit
    return k;
}
int check_freelist(){
    int check_num = jw_mem_size/4;
    int count = 1;

    for(int i = 0; i < check_num; i++){
        if(pmem_freelist[i] == 1){
            count = count + 1;
        }
    }
    if(count == check_num){
        return 0; // full
    }
    else{
        return 1;
    }
}
int check_swap_freelist(){
    int check_num = jw_swap_size/4;
    int count = 1;

    for(int i = 0; i < check_num; i++){
        if(swap_freelist[i] == 1){
            count = count + 1;
        }
    }
    if(count == check_num){
        return 0; // full
    }
    else{
        return 1;
    }
}

int ku_page_fault(char pid, char va){
    long long cur_pdbr = check_pid(pid); //pdbr of pid
    char index_pd = pd_index(va); 
    char index_pmd = pmd_index(va);
    char index_pt = pt_index(va);
    Ku_pte pde;
    Ku_pte pmde;
    Ku_pte pte;


    Ku_pte *ptr = cur_pdbr; // pd의 시작주소
    char present_bit = (ptr+index_pd)->pte_bit & 1;
    if(present_bit == 0){
        for(int i = 1; i < jw_mem_size/4; i++){
            if(pmem_freelist[i] == 0){
                pde = cal_pfn(i);
                (ptr+index_pd)->pte_bit = pde.pte_bit;//pde 넣어줌
                Ku_pte pageMiddleDirectory[4];
                for(int j = 0; j<4; j++){
                    pageMiddleDirectory[j].pte_bit = 0;
                    pmem[(i*4)+j] = pageMiddleDirectory[j];
                }
                pmem_freelist[i] = 1; // pfn할당
                break;
            }
            else if(check_freelist() == 0){
                 pmem_freelist[i] = 0;
                for(int j = 0; j<4; j++){ 
                    pmem[(i*4)+j].pte_bit = 0;
                }
                return -1; // pfn 공간 없음
            }
        }
    }
    else{
        Ku_pte a = *(ptr + index_pd);
        pde = a;
    }
    

    Ku_pte *new_pmd = &pmem[(pde.pte_bit >> 2)*4]; //pmd의 시작주소

    char present_bit2 = (new_pmd + index_pmd)->pte_bit & 1;
    if(present_bit2 == 0){
        for(int i = 1; i < jw_mem_size/4; i++){
            if(pmem_freelist[i] == 0){
                pmde = cal_pfn(i);
                (new_pmd+index_pmd)->pte_bit = pmde.pte_bit;
                Ku_pte pageTable[4];
                for(int j = 0; j<4; j++){
                    pageTable[j].pte_bit = 0;
                    pmem[(i*4)+j] = pageTable[j];
                }
                pmem_freelist[i] = 1; // pfn할당
                break;
            }
            else if(check_freelist() == 0){
                pmem_freelist[i] = 0;
                for(int j = 0; j<4; j++){ 
                    pmem[(i*4)+j].pte_bit = 0;
                }
                return -1; // pfn 공간 없음
            }
        }
    }
    else{
        Ku_pte a = *(new_pmd + index_pmd);
        pmde = a;
    }
 
    Ku_pte *new_pt = &pmem[(pmde.pte_bit >> 2)*4]; //pt의 시작주소
    char present_bit3 = (new_pt + index_pt)->pte_bit & 1;

    if(present_bit3 == 0){
        for(int i = 1; i < jw_mem_size/4; i++){
            if(pmem_freelist[i] == 0){//page 생성
                pte = cal_pfn(i);
                (new_pt+index_pt)->pte_bit = pte.pte_bit;

                Ku_pte page[4];
                for(int j = 0; j<4; j++){
                    page[j].pte_bit = 0;
                    pmem[(i*4)+j] = page[j];
                }
                int page_pfn = (pte.pte_bit >> 2);
                addq(page_pfn);
                pmem_freelist[i] = 1; // pfn할당
                break;
            }
            else if(check_freelist() == 0){//swap 
                int a = deleteq();
                Ku_pte *evict = &pmem[a*4];
                for(int k = 1; k< jw_swap_size/4; k++){
                    if(swap_freelist[k] == 0){
                        for(int l = 0; l<4; l++){
                            swap[(k*4)+l] = evict[l];
                        }
                        pmem[(pmde.pte_bit >> 2)*4 + index_pt].pte_bit = (k*4) << 1;
                        swap_freelist[k] = 1;
                    }
                }
               
                Ku_pte page[4];
                for(int m = a*4; m < 4; m++){
                    page[m].pte_bit = 0;
                    pmem[pte.pte_bit] = pmem[m]; 
                }
                if(check_swap_freelist() == 0){
                    printf("swpa space is full\n");
                    return -1;
                }
            }
        }   
    }
    else{
        Ku_pte a = *(new_pt + index_pd);
        pte = a;
    }
    
    return 0;
}

void *ku_mmu_init(unsigned int mem_size, unsigned int swap_size){
    if(mem_size <= 4 || swap_size <=4){
        return 0;
    }
    list = (List*)malloc(sizeof(List));
    
    jw_mem_size = mem_size;
    jw_swap_size = swap_size;

    pmem = (Ku_pte*)malloc(mem_size);
    swap = (Ku_pte*)malloc(swap_size);
    pmem_freelist[0] = 1;
    swap_freelist[0] = 1;
    for(int i = 1;i< mem_size/4; i++){//
        pmem_freelist[i] = 0;
    }
    for(int i = 0; i < swap_size/4; i++){
        swap_freelist[i] = 0;
    }
    return pmem;
}

int ku_run_proc(char pid, struct ku_pte **ku_cr3){
    
    if(check_pid(pid) == 0){
        Ku_pte pagedirectory[4];
        for(int i = 1; i < sizeof(pmem_freelist); i++){
            if(pmem_freelist[i] == 0){
                for(int j = 0; j < 4; j++){
                    pagedirectory[j].pte_bit = 0;
                    pmem[(i*4)+j] = pagedirectory[j];//
                }
                *ku_cr3 = &pmem[(i*4)]; // insert in ku_cr3
                pmem_freelist[i] = 1;
                node_insert(pid, &pmem[(i*4)]);
                return 0;
            }
        }
    }
    else{
       *ku_cr3 = check_pid(pid);
       return 0;
    }
    
}