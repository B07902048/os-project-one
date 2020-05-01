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
#include "scheduler.h"
#define MAXN 1000
#define DEFAULT_CPU 0

Task task[MAXN];
char policy[20];
cpu_set_t set;
int N, now_time, ready_num;
int *shm_ptr;

typedef struct{
	int data[MAXN];
	int start, end;
}Queue;
Queue ready_queue;
void Push(int data){
	ready_queue.data[ready_queue.end] = data;
	ready_queue.end = (ready_queue.end + 1) % MAXN; 
}

bool Task_is_in_ready_queue(){
	if(ready_queue.start != ready_queue.end) return true;
	return false;
}
int Pop(){
	if(!Task_is_in_ready_queue()) fprintf(stderr, "error in Pop queue\n");
	int temp = ready_queue.data[ready_queue.start];
	ready_queue.start = (ready_queue.start + 1) % MAXN;
	return temp;
}

bool Is_terminated(){
	if(!Task_is_in_ready_queue() && ready_num >= N)
		return true;
	return false;
}
void Set_priority(pid_t pid, int priority, int i){
	struct sched_param param;
	param.sched_priority = priority;
	//fprintf(stderr, "[scheduler] switch to child %d\n", i+1);
	sched_setscheduler(pid, SCHED_FIFO, &param);
}
void Start_new_task(){
	if((task[ready_num].pid = fork()) == 0){
		char num[20], ID[20];
		sprintf(num, "%d", task[ready_num].exec_time);
		sprintf(ID, "%d", ready_num);
		execl("./child", "child", num, ID, task[ready_num].name, NULL);
		fprintf(stderr, "exec child failed\n"); exit(-1);
	}
	else{
		Set_priority(task[ready_num].pid, 99, ready_num);
		//Push into queue
		Push(ready_num);
		ready_num++;
	}
	return;
}
void Start_new_tasks(){
	while(ready_num < N && task[ready_num].ready_time == now_time){
		Start_new_task();
	}
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
	//fprintf(stderr, "[scheduler] assigned child %d time = %d\n", i+1, time);
	memcpy(shm_ptr, &time, sizeof(int));
	Set_priority(task[i].pid, 99, i);
}
//
//not change yet
int Pick_next_job(){
	return Pop();
}
//
void FIFO(){
	while(1){
		//fprintf(stderr, "[scheduler] now_time = %d\n", now_time);
		if(ready_num < N && !Task_is_in_ready_queue()){
			Run_a_clock_time(task[ready_num].ready_time - now_time);
			now_time = task[ready_num].ready_time;
			Start_new_tasks();
			continue;
		}
		int i = Pick_next_job();
		//Pick_next_job(&i);
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
	while(1){
		//fprintf(stderr, "[scheduler] now_time = %d\n", now_time);
		if(ready_num < N && !Task_is_in_ready_queue()){
			Run_a_clock_time(task[ready_num].ready_time - now_time);
			now_time = task[ready_num].ready_time;
			Start_new_tasks();
			continue;
		}
		int round_remain = 500;
		int i = Pick_next_job();
		if(task[i].exec_time > round_remain){
			while(Time_remain_create_task() < round_remain){
				round_remain -= task[ready_num].ready_time - now_time;
				Assign_time_to_child(i, task[ready_num].ready_time - now_time);
				Start_new_tasks();
			}
			Assign_time_to_child(i, round_remain);
			Start_new_tasks();
			//Add
			Push(i);
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
int Pick_shortest_job(){
	int shortest = 10000000;
	int shortest_idx = -1;
	for(int j = ready_queue.start; j != ready_queue.end; j = (j + 1) % MAXN){
		if(task[ready_queue.data[j]].exec_time < shortest){
			shortest_idx = j;
			shortest = task[ready_queue.data[j]].exec_time;
		}
	}
	int temp = ready_queue.data[shortest_idx];
	ready_queue.data[shortest_idx] = ready_queue.data[ready_queue.start];
	ready_queue.data[ready_queue.start] = temp;
	return Pop();
}
void SJF(){
	while(1){
		//fprintf(stderr, "[scheduler] now_time = %d\n", now_time);
		if(ready_num < N && !Task_is_in_ready_queue()){
			Run_a_clock_time(task[ready_num].ready_time - now_time);
			now_time = task[ready_num].ready_time;
			Start_new_tasks();
			continue;
		}
		int i = Pick_shortest_job();
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
	while(1){
		//fprintf(stderr, "[scheduler] now_time = %d\n", now_time);
		if(ready_num < N && !Task_is_in_ready_queue()){
			Run_a_clock_time(task[ready_num].ready_time - now_time);
			now_time = task[ready_num].ready_time;
			Start_new_tasks();
			continue;
		}
		int i = Pick_shortest_job();
		if(Time_remain_create_task() < task[i].exec_time){	
			Assign_time_to_child(i, task[ready_num].ready_time - now_time);
			Start_new_tasks();
			Push(i);
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
	//fprintf(stderr, "start\n");
	Set_cpu();
	Set_priority(getpid(), 50, -1);
	Read_input();
	Init_shm();
	Scheduler();
	for(int i = 0; i < N; i++){
		wait(NULL);
	}
	//fprintf(stderr, "end\n");
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