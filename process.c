#include <stdio.h>
#include "prioque.h"
#include "process.h"

int process_compare(const void *e1, const void *e2)
{
    Process *p1 = (Process *)e1;
    Process *p2 = (Process *)e2;
    if (p1->PID == p2->PID)
        return 0;
    else
        return 1;
}
//
int exec_process(Process *p)
{
    p->CPU_Usage++;

    if (p->PID != 0)
    {
        p->CPUTime--;
        // printf("PID %d consume 1 CPU time, current CPUTime: %ld\n", p->PID, p->CPUTime);
        if (p->CPUTime == 0)
        {
            // when need to do IO, reset CPUTime and return DO_IO
            if (p->IOTime > 0)
            {
                p->CPUTime = p->saveCPUTime;
                return DO_IO;
            }
            else
                return FINISH;
        }
        else
            return NOT_FINISH;
    }
    else
        return FINISH;
}

int do_IO(Process *p)
{
    p->IOTime--;
    if (p->IOTime == 0)
    {
        p->repeat--;

        // when still need to repeat, reset IOTime
        if (p->repeat > 0)
            p->IOTime = p->saveIOTime;
        else
        {
            // when there more behaviors, dequeue and populate the process  fields
            if (!empty_queue(&(p->Behaviors)))
            {
                ProcessBehavior behavior;
                remove_from_front(&(p->Behaviors), &behavior);
                p->CPUTime = behavior.CPUBurst;
                p->saveCPUTime = p->CPUTime;
                p->IOTime = behavior.IOBurst;
                p->saveIOTime = p->IOTime;
                p->repeat = behavior.repeat;
            }
        }
        return FINISH;
    }
    else
        return NOT_FINISH;
}

void init_process(Process *p)
{
    p->CPU_Usage = 0;
    p->PID = 0;
    p->priority = 1;
    p->promoteFactor = 3;
    p->demoteFactor = 1;
    p->quantum = 10;
    init_queue(&(p->Behaviors), sizeof(ProcessBehavior), TRUE, NULL, TRUE);
}