#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "prioque.h"
#include "process.h"

////////////////////////GLOBAL VARIABLES/////////////////////

// Queues
Queue ArrivalQueue, HighQueue, MediumQueue, LowQueue, IOQueue;

// Special Processes
Process IdleProcess;    // the <<null>> process
Process *exeProcess;    // the pointer MLFQS used to execute a process. Either points to <<null>> process or highest priority process
Process frontReadyQ;    // the process in the front of the readyQueue
Process higherPriority; // the highest priority process removed from the readyQueue

// Other variables
int quantum = 0;     // CPU time given to a process with corresponding priority
int result = FINISH; // the result of an execution, can be DO_IO, NOT_FINISH, FINISH
int CPUclock = 0;    // counter to model the clock of the system
char report[255];    // String for reporting

//////////////////////FUNCTIONS/////////////////////////////

void init_all_queues()
{
    init_queue(&ArrivalQueue, sizeof(Process), FALSE, process_compare, FALSE);
    init_queue(&HighQueue, sizeof(Process), FALSE, process_compare, FALSE);
    init_queue(&MediumQueue, sizeof(Process), FALSE, process_compare, FALSE);
    init_queue(&LowQueue, sizeof(Process), FALSE, process_compare, FALSE);
    init_queue(&IOQueue, sizeof(Process), FALSE, process_compare, TRUE);
}

void read_process_descriptions(void)
{
    Process p;
    ProcessBehavior b;
    int pid = 0, first = 1;
    unsigned long arrival;

    init_process(&p);
    arrival = 0;
    while (scanf("%lu", &arrival) != EOF)
    {
        scanf("%d %lu %lu %d", &pid, &b.CPUBurst, &b.IOBurst, &b.repeat);

        if (!first && p.PID != pid)
        {
            add_to_queue(&ArrivalQueue, &p, p.arrival_time);
            init_process(&p);
        }
        p.PID = pid;
        p.arrival_time = arrival;
        first = 0;
        add_to_queue(&p.Behaviors, &b, 1);
    }
    add_to_queue(&ArrivalQueue, &p, p.arrival_time);
}

void final_report()
{
    printf("Scheduler shutdown at time %d.\n", CPUclock - 1);
    printf("Total CPU usage for all processes scheduled:\n");
    printf("Process <<null>>:\t%d time units.\n", IdleProcess.CPU_Usage - 1);
    puts(report);
}

// Helper function to check if all level queues are empty
int all_queues_empty()
{
    return (empty_queue(&HighQueue) && empty_queue(&MediumQueue) && empty_queue(&LowQueue));
}

/* A process exists when one of the following condition is true:
   At least one of level queues is not empty.
   IOQueue is not empty.
   ArrivalQueue is not empty.
   Currently executed process is not <<null>> process
*/
int processes_exist()
{
    return (!all_queues_empty() || !empty_queue(&IOQueue) || !empty_queue(&ArrivalQueue) || process_compare(&IdleProcess, exeProcess));
}

void add_to_scheduling_queue(Process *p)
{
    if (p->priority == 1)
        add_to_queue(&HighQueue, p, p->quantum);
    else if (p->priority == 2)
        add_to_queue(&MediumQueue, p, p->quantum);
    else if (p->priority == 3)
        add_to_queue(&LowQueue, p, p->quantum);
}

void queue_new_arrivals()
{
    Process currentProcess;
    init_process(&currentProcess);

    // Peek at the front element of the ArrivalQueue
    rewind_queue(&ArrivalQueue);
    peek_at_current(&ArrivalQueue, &currentProcess);

    if (CPUclock == currentProcess.arrival_time)
    {
        remove_from_front(&ArrivalQueue, &currentProcess);

        // Poulate the process's fields with the dequeued behavior.
        ProcessBehavior behavior;
        remove_from_front(&(currentProcess.Behaviors), &behavior);
        currentProcess.CPUTime = behavior.CPUBurst;
        currentProcess.saveCPUTime = behavior.CPUBurst;
        currentProcess.IOTime = behavior.IOBurst;
        currentProcess.saveIOTime = behavior.IOBurst;
        currentProcess.repeat = behavior.repeat;

        add_to_scheduling_queue(&currentProcess);
        printf("CREATE: Process %d entered the ready queue at time %d\n", currentProcess.PID, CPUclock);
    }
}

void demote_process(Process *p)
{
    p->priority++;

    // reset demoteFactor, promoteFactor, quantum coressponding to priority
    if (p->priority == 2)
    {
        p->demoteFactor = 2;
        p->promoteFactor = 2;
        p->quantum = 30;
    }
    else
    {
        p->promoteFactor = 1;
        p->quantum = 100;
    }
}

void promote_process(Process *p)
{
    p->priority--;
    // reset demoteFactor, promoteFactor, quantum coressponding to priority
    if (p->priority == 2)
    {
        p->demoteFactor = 2;
        p->promoteFactor = 2;
        p->quantum = 30;
    }
    else
    {
        p->demoteFactor = 1;
        p->quantum = 10;
    }
}

void execute_highest_priority_process()
{
    // CASE 1: quantum is 0, but not finish
    if (quantum == 0)
    {
        if (result == NOT_FINISH)
        {
            // demote process for priority 1 & 2
            if (exeProcess->priority < 3)
            {
                exeProcess->demoteFactor--;
                if (exeProcess->demoteFactor == 0)
                {
                    demote_process(exeProcess);
                }
            }
            // reset quantum for priority 3
            else
                exeProcess->quantum = 100;

            add_to_scheduling_queue(exeProcess);
            printf("QUEUED: Process %d queued at level %d at time %d.\n", exeProcess->PID, exeProcess->priority, CPUclock);
            exeProcess = &IdleProcess;
        }
    }

    // CASE 2: quantum is either 0 or > 0, need to do IO
    // action: decrease promoteFactor, when 0 then promote
    if (result == DO_IO)
    {
        // promote process for priority 2 & 3
        if (exeProcess->priority > 1)
        {
            exeProcess->promoteFactor--;
            if (exeProcess->promoteFactor == 0)
            {
                promote_process(exeProcess);
            }
        }
        // reset quantum for priority 1
        else
            exeProcess->quantum = 10;

        printf("I/O: Process %d blocked for I/O at time %d.\n", exeProcess->PID, CPUclock);
        add_to_queue(&IOQueue, exeProcess, 1);
        exeProcess = &IdleProcess;
    }

    // CASE 3: quantum is either 0 or > 0, finsish
    else if (result == FINISH)
    {
        if (process_compare(&IdleProcess, exeProcess))
        {
            char process_report[50];
            printf("FINISHED: Process %d finished at time %d.\n", exeProcess->PID, CPUclock);
            sprintf(process_report, "Process %d:\t\t%d time units.\n", exeProcess->PID, exeProcess->CPU_Usage);
            strcat(report, process_report);
            exeProcess = &IdleProcess;
        }
    }

    // When exeProcess is <<null>> process, choose to execute the process in the front Queues in order high med low
    if (!process_compare(&IdleProcess, exeProcess))
    {
        if (!all_queues_empty())
        {
            if (!empty_queue(&HighQueue))
            {
                remove_from_front(&HighQueue, &frontReadyQ);
            }
            else if (!empty_queue(&MediumQueue))
            {
                remove_from_front(&MediumQueue, &frontReadyQ);
            }
            else if (!empty_queue(&LowQueue))
            {
                remove_from_front(&LowQueue, &frontReadyQ);
            }

            higherPriority = frontReadyQ;
            exeProcess = &higherPriority;
            quantum = exeProcess->quantum;
            printf("RUN: Process %d started execution from level %d at time %d; wants to execute for %lu ticks.\n", exeProcess->PID, exeProcess->priority, CPUclock, exeProcess->CPUTime);
        }
    }
    // When exeProcess is not <<null>> process, look for higher priority process
    else
    {
        if (!empty_queue(&HighQueue) || !empty_queue(&MediumQueue))
        {
            if (!empty_queue(&HighQueue) && exeProcess->priority > 1)
            {
                remove_from_front(&HighQueue, &frontReadyQ);
            }
            else if (!empty_queue(&MediumQueue) && exeProcess->priority > 2)
            {
                remove_from_front(&MediumQueue, &frontReadyQ);
            }

            // This condition only met when successfully remove front process in either High/Med priority queues
            if (exeProcess->priority > frontReadyQ.priority)
            {
                printf("QUEUED: Process %d queued at level %d at time %d.\n", exeProcess->PID, exeProcess->priority, CPUclock);
                exeProcess->quantum = quantum;
                add_to_scheduling_queue(exeProcess);

                higherPriority = frontReadyQ;
                exeProcess = &higherPriority;
                quantum = exeProcess->quantum;
                printf("RUN: Process %d started execution from level %d at time %d; wants to execute for %lu ticks.\n", exeProcess->PID, exeProcess->priority, CPUclock, exeProcess->CPUTime);
            }
        }
    }

    result = exec_process(exeProcess);
    quantum--;
}

void do_io_for_processes()
{
    if (!empty_queue(&IOQueue))
    {
        Process IOProcess;
        init_process(&IOProcess);
        for (int i = 0; i < IOQueue.queuelength; i++)
        {
            remove_from_front(&IOQueue, &IOProcess);
            int result = do_IO(&IOProcess);

            // If finish IO, add back to readyQ
            if (result != FINISH)
                add_to_queue(&IOQueue, &IOProcess, 1);
            else
                add_to_scheduling_queue(&IOProcess);
        }
    }
    else
        return;
}

int main(int argc, char *argv[])
{
    init_all_queues();
    init_process(&IdleProcess);
    exeProcess = &IdleProcess;
    init_process(&frontReadyQ);
    init_process(&higherPriority);
    read_process_descriptions();

    while (processes_exist())
    {
        CPUclock++;
        queue_new_arrivals();
        execute_highest_priority_process();
        do_io_for_processes();
    };
    CPUclock++;
    final_report();
    return 0;
}