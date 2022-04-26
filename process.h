#define DO_IO 0
#define NOT_FINISH 1
#define FINISH 2

//
typedef struct Process
{
    unsigned int PID; // PID of the process, it is unique for every process
    unsigned int arrival_time; // arrival time of the process
    long unsigned int CPUTime; // Time of CPU which the processes need to consume 
    long unsigned int saveCPUTime; // The CPU time needed after finishing IO
    long unsigned int IOTime; // Time of IO which the processes need to consume
    long unsigned int saveIOTime; // The IO time needed when finish IO but not finish repeating
    unsigned int repeat; // Numvber of times the process repeats for a behaviors
    unsigned int CPU_Usage; // Used to report CPU usage
    unsigned int priority; //priority when enter the MLFQA, 1 highest , 2 , 3 lowest
    unsigned int promoteFactor; // promote factor, when 0 get promoted then reset, decrease by 1 when execute without exhausting quantum, initial with max value 3
    unsigned int demoteFactor; // demote factor, when 0 get demoted then reset, decrease by 1 when exhaust the quantum without IO or finish, initial with value 1, max is 3
    unsigned int quantum;
    Queue Behaviors; //FIFO queue holds the behaviors of the process
} Process;

// Struct of a behavior of a process
typedef struct ProcessBehavior
{
    long unsigned int CPUBurst;
    long unsigned int IOBurst;
    unsigned int repeat;
} ProcessBehavior;

/* compare 2 processes by their PID,
   if PIDs are the same, they are the same process, return 0,
   otherwise return 1
*/
int process_compare(const void *e1, const void *e2);

/* model the execution of the process in 1 tick.
   increase CPU_usage for reporting purpose
   return DO_IO to signal being blockced for IO
   return FINISH to signal the process has been finished execution
   return NOT_FINISH to signal not finish yet
*/
int exec_process(Process *p);

/* model the IO operation needed of the process in 1 tick
   rerturn FINISH to signal finish IO
   return NOT_FINISH to siganl not finish yet
*/
int do_IO(Process *p);

/* initializes a process with following default values:
   PID = 0, indicate the <<null>> process
   CPU_usage = 0
   priority = 1, every process enters to MLFQS with highest priority
   promoteFactor = 3
   demoteFactor = 1
   quantum = 10
*/
void init_process(Process *p);