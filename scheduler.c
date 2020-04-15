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
void Set_cpu();
void Init_shm();

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
	fprintf(stderr, "[scheduler] switch to child %d\n", i+1);
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
		//fprintf(stderr, "[scheduler] task new: %d\n", ready_num);
		Set_priority(task[ready_num].pid, 99, ready_num);
		ready_num++;
	}
	return;
}
void Start_new_tasks(){
	while(ready_num < N && task[ready_num].ready_time == now_time){
		Start_new_task();
	}
}
bool Task_is_in_ready_queue(){
	for(int i = 0; i < ready_num; i++){
		if(task[i].exec_time > 0) return true;
	}
	return false;
}
int Time_remain_create_task(){
	if(ready_num < N){
		return task[ready_num].ready_time - now_time;
	}
	return 10000000;
}
void Assign_time_to_child(int i, int time){
	task[i].exec_time -= time;
	now_time += time;
	fprintf(stderr, "[scheduler] assigned child %d time = %d\n", i+1, time);
	memcpy(shm_ptr, &time, sizeof(int));
	//write(task[i].fd, buf, strlen(buf));
	//fprintf(stderr, "%d %d\n", i, task[i].pid);
	Set_priority(task[i].pid, 99, i);
}
void Pick_next_job(int *i){
	do{
		*i = (*i + 1) % ready_num;
	}while(task[*i].exec_time <= 0);
}
void FIFO(){
	int i = -1;
	while(1){
		fprintf(stderr, "[scheduler] now_time = %d\n", now_time);
		if(ready_num < N && !Task_is_in_ready_queue()){
			Run_a_clock_time(task[ready_num].ready_time - now_time);
			now_time = task[ready_num].ready_time;
			Start_new_tasks();
			continue;
		}
		Pick_next_job(&i);
		while(Time_remain_create_task() < task[i].exec_time){	
			Assign_time_to_child(i, task[ready_num].ready_time - now_time);
			Start_new_tasks();	
		}
		Assign_time_to_child(i, task[i].exec_time);
		Start_new_tasks();
		if(Is_terminated()) break;
	}
}
void RR(){
	int i = -1;
	while(1){
		fprintf(stderr, "[scheduler] now_time = %d\n", now_time);
		if(ready_num < N && !Task_is_in_ready_queue()){
			Run_a_clock_time(task[ready_num].ready_time - now_time);
			now_time = task[ready_num].ready_time;
			Start_new_tasks();
			continue;
		}
		int round_remain = 500;
		Pick_next_job(&i);
		if(task[i].exec_time > round_remain){
			while(Time_remain_create_task() < round_remain){
				//fprintf(stderr, "in\n");
				round_remain -= task[ready_num].ready_time - now_time;
				Assign_time_to_child(i, task[ready_num].ready_time - now_time);
				Start_new_tasks();
			}
			Assign_time_to_child(i, round_remain);
			Start_new_tasks();
		}
		else{
			while(Time_remain_create_task() < task[i].exec_time){	
				Assign_time_to_child(i, task[ready_num].ready_time - now_time);
				Start_new_tasks();	
			}
			Assign_time_to_child(i, task[i].exec_time);
			Start_new_tasks();
		}
		if(Is_terminated()) break;
	}
}
void Pick_shortest_job(int *i){
	int shortest = 10000000;
	for(int j = 0; j < ready_num; j++){
		//fprintf(stderr, "task[%d].exec_time = %d\n", j, task[j].exec_time);
		if(task[j].exec_time > 0 && task[j].exec_time < shortest){
			*i = j;
			shortest = task[j].exec_time;
		}
	}
	//fprintf(stderr, "[scheduler] ready_num = %d, Pick child %d\n", ready_num, *i + 1);
}
void SJF(){
	int i = -1;
	while(1){
		fprintf(stderr, "[scheduler] now_time = %d\n", now_time);
		if(ready_num < N && !Task_is_in_ready_queue()){
			Run_a_clock_time(task[ready_num].ready_time - now_time);
			now_time = task[ready_num].ready_time;
			Start_new_tasks();
			continue;
		}
		Pick_shortest_job(&i);
		while(Time_remain_create_task() < task[i].exec_time){	
			Assign_time_to_child(i, task[ready_num].ready_time - now_time);
			Start_new_tasks();	
		}
		Assign_time_to_child(i, task[i].exec_time);
		Start_new_tasks();
		if(Is_terminated()) break;
	}
}
void PSJF(){
	int i = -1;
	while(1){
		fprintf(stderr, "[scheduler] now_time = %d\n", now_time);
		if(ready_num < N && !Task_is_in_ready_queue()){
			Run_a_clock_time(task[ready_num].ready_time - now_time);
			now_time = task[ready_num].ready_time;
			Start_new_tasks();
			continue;
		}
		Pick_shortest_job(&i);
		if(Time_remain_create_task() < task[i].exec_time){	
			Assign_time_to_child(i, task[ready_num].ready_time - now_time);
			Start_new_tasks();	
		}
		else{
			Assign_time_to_child(i, task[i].exec_time);
			Start_new_tasks();
		}
		if(Is_terminated()) break;
	}
}
void Scheduler(){
	if(strcmp(policy, "FIFO") == 0) FIFO();
	else if(strcmp(policy, "RR") == 0) RR();
	else if(strcmp(policy, "SJF") == 0) SJF();
	else if(strcmp(policy, "PSJF") == 0) PSJF();
}
int main(){
	Set_cpu();
	Set_priority(getpid(), 50, -1);
	Read_input();
	Init_shm();
	Scheduler();
	for(int i = 0; i < N; i++){
		wait(NULL);
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