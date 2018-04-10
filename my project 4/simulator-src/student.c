/*
 * Yuen Han Chan
 */

/*
 * student.c
 * Multithreaded OS Simulation for CS 2200, Project 4
 * Spring 2015
 *
 * This file contains the CPU scheduler for the simulation.
 * Name: Yuen Han Chan
 * GTID: 902983019
 */

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "os-sim.h"


/*
 * current[] is an array of pointers to the currently running processes.
 * There is one array element corresponding to each CPU in the simulation.
 *
 * current[] should be updated by schedule() each time a process is scheduled
 * on a CPU.  Since the current[] array is accessed by multiple threads, you
 * will need to use a mutex to protect it.  current_mutex has been provided
 * for your use.
 */
static pcb_t **current;
static pthread_mutex_t current_mutex;
static pthread_mutex_t ready_mutex;

static pcb_t* head;
static pcb_t* tail;
static int size = 0;

static pthread_cond_t is_ready;
static int time_slice;
static int static_priority_true;
static int round_robin;
static int cpu_count;

void add_list(pcb_t* new_pcb){
    if(head==NULL){
        head = new_pcb;
        tail = new_pcb;
    }
    else{
        tail->next = new_pcb;
        tail = new_pcb;
    }
    
    tail->next = NULL;
    size++;
}

pcb_t* remove_list(){
    pcb_t* toReturn;
    if(head==NULL){
        return NULL;
    }
    if(size==1){
        toReturn = head;
        head = NULL;
        tail = NULL;
    }
    else{
        toReturn = head;
        head = head->next;
    }
    size--;
    return toReturn;
}

pcb_t* remove_list_priority(){
    int highest = 0;
    pcb_t* toReturn = NULL;

    if(head==NULL){
        return NULL;
    }
    else{
        pcb_t* temp = head;

        while(temp!=NULL){
            if(temp->static_priority>highest){
                highest = temp->static_priority;
            }
            temp = temp->next;
        }

        pcb_t* prev = head;
        pcb_t* find = head;
        while(find!=NULL){
            if(find->static_priority == highest){
                if(find==head){
                    toReturn = find;
                    head = find->next;
                }
                else if(find==tail){
                    toReturn = tail;
                    tail = prev;
                    prev->next = NULL;
                }
                else{
                    toReturn = find;
                    prev->next = find->next;
                }
                find->next = NULL;
            }
            prev = find;
            find = find->next;
        }
    }
    if(toReturn!=NULL){
        size--;
    }
    return toReturn;
}

/*
 * schedule() is your CPU scheduler.  It should perform the following tasks:
 *
 *   1. Select and remove a runnable process from your ready queue which
 *  you will have to implement with a linked list or something of the sort.
 *
 *   2. Set the process state to RUNNING
 *
 *   3. Call context_switch(), to tell the simulator which process to execute
 *      next on the CPU.  If no process is runnable, call context_switch()
 *      with a pointer to NULL to select the idle process.
 *  The current array (see above) is how you access the currently running process indexed by the cpu id.
 *  See above for full description.
 *  context_switch() is prototyped in os-sim.h. Look there for more information
 *  about it and its parameters.
 */
static void schedule(unsigned int cpu_id)
{
    pcb_t* ready_queue;
    pthread_mutex_lock(&ready_mutex);
    if(static_priority_true){
        ready_queue = remove_list_priority();
    }
    else{
        ready_queue = remove_list();
    }
    pthread_mutex_unlock(&ready_mutex);
    if(ready_queue == NULL) {
        context_switch(cpu_id, NULL, time_slice);
        
    } else {
        ready_queue->state = PROCESS_RUNNING;
        pthread_mutex_lock(&current_mutex);
        current[cpu_id] = ready_queue;
        pthread_mutex_unlock(&current_mutex);
        context_switch(cpu_id,ready_queue,time_slice);
    }
    
}



/*
 * idle() is your idle process.  It is called by the simulator when the idle
 * process is scheduled.
 *
 * This function should block until a process is added to your ready queue.
 * It should then call schedule() to select the process to run on the CPU.
 */
extern void idle(unsigned int cpu_id)
{
    pthread_mutex_lock(&ready_mutex);
    while(head==NULL){
        pthread_cond_wait(&is_ready, &ready_mutex);
    }
    pthread_mutex_unlock(&ready_mutex);
    schedule(cpu_id);
}




/*
 * preempt() is the handler called by the simulator when a process is
 * preempted due to its time_slice expiring.
 *
 * This function should place the currently running process back in the
 * ready queue, and call schedule() to select a new runnable process.
 */
extern void preempt(unsigned int cpu_id)
{
    pthread_mutex_lock(&current_mutex);
    current[cpu_id]->state = PROCESS_READY;
    pthread_mutex_unlock(&current_mutex);
    add_list(current[cpu_id]);
    schedule(cpu_id);
}


/*
 * yield() is the handler called by the simulator when a process yields the
 * CPU to perform an I/O request.
 *
 * It should mark the process as WAITING, then call schedule() to select
 * a new process for the CPU.
 */
extern void yield(unsigned int cpu_id)
{
    pthread_mutex_lock(&current_mutex);
    current[cpu_id]->state = PROCESS_WAITING;
    pthread_mutex_unlock(&current_mutex);
    schedule(cpu_id);
}


/*
 * terminate() is the handler called by the simulator when a process completes.
 * It should mark the process as terminated, then call schedule() to select
 * a new process for the CPU.
 */
extern void terminate(unsigned int cpu_id)
{
    pthread_mutex_lock(&current_mutex);
    current[cpu_id]->state = PROCESS_TERMINATED;
    pthread_mutex_unlock(&current_mutex);
    schedule(cpu_id);
}


/*
 * wake_up() is the handler called by the simulator when a process's I/O
 * request completes.  It should perform the following tasks:
 *
 *   1. Mark the process as READY, and insert it into the ready queue.
 *
 *   2. If the scheduling algorithm is static priority, wake_up() may need
 *      to preempt the CPU with the lowest priority process to allow it to
 *      execute the process which just woke up.  However, if any CPU is
 *      currently running idle, or all of the CPUs are running processes
 *      with a higher priority than the one which just woke up, wake_up()
 *      should not preempt any CPUs.
 *  To preempt a process, use force_preempt(). Look in os-sim.h for
 *  its prototype and the parameters it takes in.
 */
extern void wake_up(pcb_t *process)
{
    int preempt = -1;
    int low;
    
    process->state = PROCESS_READY;
    add_list(process);
    low = process->static_priority;

    if (static_priority_true) {
        pthread_mutex_lock(&current_mutex);
        for(int i = 0; i<cpu_count; i++){
            if(current[i]!=NULL && current[i]->static_priority < low){
                preempt = i;
                low = current[i]->static_priority;
            }
        }
        pthread_mutex_unlock(&current_mutex);
    }

    if(preempt!= -1){
        force_preempt(preempt);
    
    }
    pthread_cond_signal(&is_ready);
}


/*
 * main() simply parses command line arguments, then calls start_simulator().
 * You will need to modify it to support the -r and -p command-line parameters.
 */
int main(int argc, char *argv[])
{
    round_robin = 0;
    time_slice = -1;

    cpu_count = atoi(argv[1]);
    
    if(argc!=2){
        if(argv[2][1]=='r'){
            round_robin = 1;
        }
        
        if(argv[2][1]=='p'){
            static_priority_true = 1;
        }
    }
    
    if(round_robin) {
        time_slice = atoi(argv[3]);
    }


    current = malloc(sizeof(pcb_t*) * cpu_count);
    assert(current != NULL);
    pthread_mutex_init(&current_mutex, NULL);
    pthread_mutex_init(&ready_mutex, NULL);
    pthread_cond_init(&is_ready, NULL);


    /* Start the simulator in the library */
    start_simulator(cpu_count);
    
    return 0;
}


