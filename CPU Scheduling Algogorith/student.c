/*
 * student.c
 * Multithreaded OS Simulation for CS 2200, Project 4
 * Fall 2014
 *
 * This file contains the CPU scheduler for the simulation.
 * Name: Ankit Khanal
 * GTID: akhanal7
 */

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "os-sim.h"

/* Function Declarations */
void add_readyqueue(pcb_t *que);
pcb_t *remove_readyqueue();
int shortest_time(pcb_t *process);

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

/* ready queue information */
static pcb_t *head;
static pthread_mutex_t readyqueue_mutex;

/* scheduler information */
static int cpu_count;
static int time_slice;
static int scheduler_type;

/* condition information */
static pthread_cond_t empty;

/* Linked List implementation */
void add_readyqueue(pcb_t *que) {
    pcb_t *current = head;
    que->next = NULL;

    /* Check if the queue is empty */
    if (current == NULL) {
        head = que;
    } else {
        while (current->next != NULL) {
          current = current->next;
        }
        current->next = que;
    }
}

pcb_t *remove_readyqueue() {
    pcb_t *current;
    pcb_t *next_node;

    if (head == NULL) {
        return NULL;
    } else {
        current = head;
        next_node = head->next;
        head = NULL;
  	    head = next_node;
    }
    return current;
}

int shortest_time(pcb_t *process) {
  	int pos = -1;

    for(int i = 0; i < cpu_count; i++) {
      if(current[i] != NULL) {
        if ((current[i]->time_remaining) < process->time_remaining) {
          pos = i;
        }
      }
    }
    return pos;
}

/*
 * schedule() is your CPU scheduler.  It should perform the following tasks:
 *
 *   1. Select and remove a runnable process from your ready queue which
 *	you will have to implement with a linked list or something of the sort.
 *
 *   2. Set the process state to RUNNING
 *
 *   3. Call context_switch(), to tell the simulator which process to execute
 *      next on the CPU.  If no process is runnable, call context_switch()
 *      with a pointer to NULL to select the idle process.
 *	The current array (see above) is how you access the currently running process indexed by the cpu id.
 *	See above for full description.
 *	context_switch() is prototyped in os-sim.h. Look there for more information
 *	about it and its parameters.
 */
static void schedule(unsigned int cpu_id)
{
    /* FIX ME */
    pthread_mutex_lock(&readyqueue_mutex);
    pcb_t* value = remove_readyqueue();
    pthread_mutex_unlock(&readyqueue_mutex);

    if (value == NULL) {
      context_switch(cpu_id, NULL, time_slice);
    } else {
      pthread_mutex_lock(&readyqueue_mutex);
      value->state = PROCESS_RUNNING;
      pthread_mutex_unlock(&readyqueue_mutex);

      pthread_mutex_lock(&current_mutex);
      current[cpu_id] = value;
      pthread_mutex_unlock(&current_mutex);

      context_switch(cpu_id, value, time_slice);
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
    /* FIX ME */
    pthread_mutex_lock(&readyqueue_mutex);

    while (head == NULL) {
        pthread_cond_wait(&empty, &readyqueue_mutex);
    }

    pthread_mutex_unlock(&readyqueue_mutex);
    schedule(cpu_id);

    /*
     * REMOVE THE LINE BELOW AFTER IMPLEMENTING IDLE()
     *
     * idle() must block when the ready queue is empty, or else the CPU threads
     * will spin in a loop.  Until a ready queue is implemented, we'll put the
     * thread to sleep to keep it from consuming 100% of the CPU time.  Once
     * you implement a proper idle() function using a condition variable,
     * remove the call to mt_safe_usleep() below.
     */
    // mt_safe_usleep(1000000);
}


/*
 * preempt() is the handler called by the simulator when a process is
 * preempted due to its timeslice expiring.
 *
 * This function should place the currently running process back in the
 * ready queue, and call schedule() to select a new runnable process.
 */
extern void preempt(unsigned int cpu_id)
{
    /* FIX ME */
    pcb_t *running_pcb;
    pthread_mutex_lock(&current_mutex);

    running_pcb = current[cpu_id];
    running_pcb->state = PROCESS_READY;

    pthread_mutex_unlock(&current_mutex);
    add_readyqueue(running_pcb);
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
    /* FIX ME */
    pcb_t *pcb;
    pthread_mutex_lock(&current_mutex);

    pcb = current[cpu_id];
    pcb->state = PROCESS_WAITING;

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
    /* FIX ME */
    pcb_t *pcb;
    pthread_mutex_lock(&current_mutex);

    pcb = current[cpu_id];
    pcb->state = PROCESS_TERMINATED;

    pthread_mutex_unlock(&current_mutex);
    schedule(cpu_id);
}


/*
 * wake_up() is the handler called by the simulator when a process's I/O
 * request completes.  It should perform the following tasks:
 *
 *   1. Mark the process as READY, and insert it into the ready queue.
 *
 *   2. If the scheduling algorithm is SRTF, wake_up() may need
 *      to preempt the CPU with the highest remaining time left to allow it to
 *      execute the process which just woke up.  However, if any CPU is
 *      currently running idle, or all of the CPUs are running processes
 *      with a lower remaining time left than the one which just woke up, wake_up()
 *      should not preempt any CPUs.
 *	To preempt a process, use force_preempt(). Look in os-sim.h for
 * 	its prototype and the parameters it takes in.
 */
extern void wake_up(pcb_t *process)
{
    /* FIX ME */
    int value = -1;

    if (scheduler_type == 3) {
      pthread_mutex_lock(&readyqueue_mutex);
      pthread_mutex_lock(&current_mutex);

      value = shortest_time(process);

      pthread_mutex_unlock(&readyqueue_mutex);
      pthread_mutex_unlock(&current_mutex);

      if(value > -1) {
        if (head != NULL) {
          force_preempt(value);
        }
      }
    }

    pthread_mutex_lock(&readyqueue_mutex);
    pthread_mutex_lock(&current_mutex);

    process->state = PROCESS_READY;
    add_readyqueue(process);

    pthread_mutex_unlock(&readyqueue_mutex);
    pthread_mutex_unlock(&current_mutex);
    pthread_cond_signal(&empty);
}


/*
 * main() simply parses command line arguments, then calls start_simulator().
 * You will need to modify it to support the -r and -s command-line parameters.
 */
int main(int argc, char *argv[])
{
    cpu_count = 0;
    scheduler_type = 0;

    /* Parse command-line arguments */
    if (argc < 2 || argc > 4)
    {
        fprintf(stderr, "CS 2200 Project 4 -- Multithreaded OS Simulator\n"
            "Usage: ./os-sim <# CPUs> [ -r <time slice> | -s ]\n"
            "    Default : FIFO Scheduler\n"
            "         -r : Round-Robin Scheduler\n"
            "         -s : Shortest Remaining Time First Scheduler\n\n");
        return -1;
    }


    /* FIX ME - Add support for -r and -s parameters*/
    if (argc == 2) {
        scheduler_type = 1;
        time_slice = -1;
    } else {
        if (argv[2][1] == 'r') {
            scheduler_type = 2;
            time_slice = atoi(argv[3]);
        }
        if (argv[2][1] == 's') {
            scheduler_type = 3;
            time_slice = -1;
        }
    }
    cpu_count = atoi(argv[1]);

    head = NULL;
    /* Allocate the current[] array and its mutex */
    current = malloc(sizeof(pcb_t*) * cpu_count);
    assert(current != NULL);
    pthread_mutex_init(&current_mutex, NULL);
    pthread_mutex_init(&readyqueue_mutex, NULL);
    pthread_cond_init(&empty, NULL);

    /* Start the simulator in the library */
    start_simulator(cpu_count);

    return 0;
}
