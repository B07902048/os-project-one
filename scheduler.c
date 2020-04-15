//need to exec by root
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sched.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
//#include "scheduler.h"
#define MAXN 100000
#define DEFAULT_PRIORITY 1
#define DEFAULT_CPU 0

void Read_input();
void Run_a_clock_time(int iter);
int Cmp(const void *p1,const void *p2);

typedef struct{
	char name[20];
	int ready_time, exec_time;
	pid_t pid;
}Task;

Task task[MAXN];
int N, GCD_ITER;
char policy[20];
cpu_set_t set;
int now_time = 0;
int ready_num = 0;
int *shm_ptr;
bool Is_terminated(){
	for(int i = 0; i < N; i++){
		if(task[i].exec_time > 0) return false;
	}
	return true;
}
void Set_priority(pid_t pid, int priority, int i){
	struct sched_param param;
	param.sched_priority = priority;
	fprintf(stderr, "[scheduler] switch to child (pid = %d)\n", pid);
	//fprintf(stderr, "[scheduler] task[%d] being set to %d (pid = %d)\n", i, priority, pid);
	sched_setscheduler(pid, SCHED_FIFO, &param);
	//fprintf(stderr, "[scheduler] back to scheduler from pid = %d\n", pid);
}

void Start_new_task(){
	if((task[ready_num].pid = fork()) == 0){
		char num[20], ID[20];
		sprintf(num, "%d", task[ready_num].exec_time);
		sprintf(ID, "%d", ready_num);
//		fprintf(stderr, "[scheduler] pass para %s to pid %d\n", num, getpid());
		execl("./child", "child", num, ID, NULL);
//		fprintf(stderr, "exec child failed\n"); exit(-1);
	}
	else{
		//sched_setaffinity(task[*ready_num].pid, sizeof(set), &set);
		//fprintf(stderr, "%d\n", task[ready_num].pid);
		Set_priority(task[ready_num].pid, 99, ready_num);
		ready_num++;
	}
	return;
}

bool Task_is_in_ready_queue(){
	for(int i = 0; i < ready_num; i++){
		if(task[i].exec_time > 0) return true;
	}
	return false;
}

void Assign_time_to_child(int i, int time){
	task[i].exec_time -= time;
	now_time += time;
	fprintf(stderr, "[scheduler] assigned child (pid = %d, time = %d)\n", task[i].pid, time);
	memcpy(shm_ptr, &time, sizeof(int));
	//write(task[i].fd, buf, strlen(buf));
	//fprintf(stderr, "%d %d\n", i, task[i].pid);
	Set_priority(task[i].pid, 99, i);
	
}
void FIFO(){
	int i = 0;
	while(!Is_terminated()){
		fprintf(stderr, "[scheduler] now_time = %d\n", now_time);
		if(ready_num < N && !Task_is_in_ready_queue()){
			Run_a_clock_time(task[ready_num].ready_time - now_time);
			now_time = task[ready_num].ready_time;
		}
		while(ready_num < N && task[ready_num].ready_time == now_time){
			Start_new_task();
		}
		fprintf(stderr, "ready_num = %d, i = %d\n", ready_num, i);
		if(ready_num < N && task[ready_num].ready_time - now_time < task[i].exec_time){
			
			Assign_time_to_child(i, task[ready_num].ready_time - now_time);
		}
		else{
			Assign_time_to_child(i, task[i].exec_time);
			i++;
		}
	}
}
void RR(){
	int i = 0, this_round = 500;
	while(1){
		fprintf(stderr, "[scheduler] now_time = %d\n", now_time);
		if(ready_num < N && !Task_is_in_ready_queue()){
			Run_a_clock_time(task[ready_num].ready_time - now_time);
			now_time = task[ready_num].ready_time;
		}
		while(ready_num < N && task[ready_num].ready_time == now_time){
			Start_new_task();
		}
		if(ready_num < N && task[ready_num].ready_time - now_time < task[i].exec_time \
			&& task[ready_num].ready_time - now_time < this_round){	
			this_round -= task[ready_num].ready_time - now_time;
			fprintf(stderr, "this_round remain %d\n", this_round);
			Assign_time_to_child(i, task[ready_num].ready_time - now_time);
		}
		else{
			if(task[i].exec_time < this_round) 
				Assign_time_to_child(i, task[i].exec_time);
			else 
				Assign_time_to_child(i, this_round);
			if(Is_terminated()) break;
			do{
				i = (i + 1) % ready_num;
			}while(task[i].exec_time <= 0);
			this_round = 500;
		}
	}
}
int Pick_shortest_job(int *i){
	int shortest = 10000000;
	for(int j = 0; j < ready_num; j++){
		if(task[(*i+j) % ready_num].exec_time > 0 && task[(*i+j) % ready_num].exec_time < shortest){
			*i = (*i + j) % ready_num;
			shortest = task[(*i+j) % ready_num].exec_time;
		}
	}
}
void SJF(){
	int i = 0;
	if(ready_num < N && !Task_is_in_ready_queue()){
		Run_a_clock_time(task[ready_num].ready_time - now_time);
		now_time = task[ready_num].ready_time;
	}
	while(ready_num < N && task[ready_num].ready_time == now_time){
		Start_new_task();	
	}
	Pick_shortest_job(&i);
	while(1){
		fprintf(stderr, "[scheduler] now_time = %d\n", now_time);
		if(ready_num < N && !Task_is_in_ready_queue()){
			Run_a_clock_time(task[ready_num].ready_time - now_time);
			now_time = task[ready_num].ready_time;
		}
		while(ready_num < N && task[ready_num].ready_time == now_time){
			Start_new_task();
		}
		
		if(ready_num < N && task[ready_num].ready_time - now_time < task[i].exec_time){	
			Assign_time_to_child(i, task[ready_num].ready_time - now_time);
		}
		else{
			Assign_time_to_child(i, task[i].exec_time);
			if(Is_terminated()) break;
			Pick_shortest_job(&i);
		}
	}
}
void PSJF(){

}
void Scheduler(){
	if(strcmp(policy, "FIFO") == 0){
		FIFO();
	}
	else if(strcmp(policy, "RR") == 0){
		RR();
	}
	else if(strcmp(policy, "SJF") == 0){
		SJF();
	}
	else if(strcmp(policy, "PSJF") == 0){
		PSJF();
	}
}
void Set_cpu(){
	CPU_ZERO(&set);
	CPU_SET(DEFAULT_CPU, &set);
	sched_setaffinity(getpid(), sizeof(set), &set);
}
void Init_shm(){
	int fd = shm_open("_shmEE", O_CREAT|O_RDWR, 0777);
	ftruncate(fd, sizeof(int));
	shm_ptr = (int*)mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);
}
int main(){
	//Set CPU
	Set_cpu();
	Set_priority(getpid(), 50, -1);
	//read input text
	Read_input();
	Init_shm();
	//get into scheduler
	Scheduler();

	//wait all processes
	for(int i = 0; i < N; i++){
		wait(NULL);
	}
	return 0;
}



void Read_input(){
	scanf("%s %d", policy, &N);
	for(int i = 0; i < N; i++){
		scanf("%s %d %d", task[i].name, &task[i].ready_time, &task[i].exec_time);
	}
	qsort(task, N, sizeof(Task), Cmp);
}



void Run_a_clock_time(int iter){
	volatile unsigned long i;
	while(iter--){
		for(i = 0; i < 1000000UL; i++);
	}
}


int Cmp(const void *p1,const void *p2) { 
    Task *pf1 = (Task*) p1; 
    Task *pf2 = (Task*) p2;
    if(pf1->ready_time < pf2->ready_time) return -1;
    else if(pf1->ready_time > pf2->ready_time) return 1;
    return 0;
} 