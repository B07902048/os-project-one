#define _GNU_SOURCE
#include <stdio.h>
#include <stdbool.h>

typedef struct{
	char name[20];
	int ready_time, exec_time;
	pid_t pid;
}Task;

void Read_input();
void Run_a_clock_time(int iter);
int Cmp(const void *p1,const void *p2);
void Set_cpu();
void Init_shm();
void Scheduler();
void PSJF();
void SJF();
void Pick_shortest_job(int *i);
void Pick_next_job(int *i);
void RR();
void FIFO();
void Assign_time_to_child(int i, int time);
int Time_remain_create_task();
bool Task_is_in_ready_queue();
void Start_new_tasks();
void Start_new_task();
void Set_priority(pid_t pid, int priority, int i);
bool Is_terminated();
