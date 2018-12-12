#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <pthread.h>

#define INDICATOR __attribute__((annotate("indicator")))
#define DELEGATOR __attribute__((annotate("delegator")))
#define IND_IDENTIFIER __attribute__((annotate("ind_indentifier")))
#define DEL_IDENTIFIER __attribute__((annotate("del_indentifier")))


/* MPI Static Analysis
 *
 * The MPI Static Analysis Pass does the following 4 steps:
 *
 * 1. Identify updates to indicator vars, delegator indicators, add
 * instrumentation for unit-context updates.
 * 2. All creation/initialization locations need instrumentation added for the
 * inheritance of unit context
 * 3. Instrumentation for montioring reads/writes of channel vars/fields.
 * 4. For all system/library calls that may lead to syscalls we need to add
 * instrumentation for unit-event emission and redundancy detection.
 *
 * 2,3 - Type-based analysis is used. 
 * 4 - Predefine a list of library functions and system calls of interest and
 * scan the llvm bitcode to identify all syscalls and library calls on the
 * lists.
 *
 * 1. There are two methods for achieving 1:
 *
 * 1.1 Naive walk-through of the LLVM code to identify all definitions to
 * indicator varaibles or to their aliaces (default alias analhysis in llvm).
 * This may lead to redundant instrumentation. 
 *  ** A indicator may be defined multiple times and there may not be any
 *  system calls. Since this is the case, tracking context is unnecessary.
 * 1.2 To fix the issues in 1.1, the problem can be formulated as a
 * readcing-defintion problem. i.e. from the definition of point x, can we read
 * program point l (a system call) from any path. 
 *
 */

/*--------------------------------------------------------------------------
 * MPI runtime data structures.
 *
 * The MPI runtime leverages the instrumented binary to maintain the context
 * that each thread is executing under. To achieve this, a hash table called
 * "delegation_table" will be created. The table is keyed on delegator structure and
 * the value is a context_id, which is a unique value that identifies a context. 
 *
 * The runtime has the following responsibilities:
 *
 * 1. When a new delegator structure is created, a new key will be added to the
 * hashmap and the value will be set to the current context in which we are
 * executing in.
 *
 * 2. If a delegator's indicator variable's value changes, which will occur
 * when it starts a new subtask, the context of the current thread will be set
 * to the context of the subtask that is about to be executed. 
 *-------------------------------------------------------------------------*/

// (Pretend this is a hashmap). which maps a delegator to its current ;context.
// -- Maps delegators --> context_id
/*--------------------------------------------------------------------------*/


/* 
 * The underlying intuition behind MPI is that the context that a unit 
 * of execution is running under is usually defined as a data structure in 
 * an application. For example, in this sample program if we wanted to
 * partition the application's behavior based on each user input, we need
 * to monitor the `struct input_buffer` which is a data structure used to
 * store an instance of a user's input. Additionally in order to monitor a
 * "context" we need a method to distinquish instances of this data structure. 
 * In this case, we can use the id field, which is a unique id for each user input.
 */
struct input_buffer {
  char *user_input;
  size_t size;
  //@identifier - The id field can be used to differentiate the instances of
  //a unit data structure, which services as an identifier for each context.
  IND_IDENTIFIER int id;
};


/* 
 * The curr_input indicates a change in context. In this case, curr_input
 * represents the current user input we are applying operations on. Since the
 * goal of this approach is to map the set of system calls invoked by a
 * specific user input, we need to consider this variable a @indicator. When
 * the value of a indicator variable changes, it implies a change in context,
 * which will need to be monitored.
 */
INDICATOR struct input_buffer curr_input;

/* 
 * The subtask data structure represents a task that will be invoked under a
 * specific context that we are trying to attach to the system-call
 * information. For MPI, the subtask data structure is considered a
 * @delegator. In this sense, we need to think of a delegator as a "delegator"
 * for the top-level thread, because this data-structure represents the task
 * passed to a worker thread to be executed.
 *
 * XXX: It is important to distinquish that @delegator structures do not represent
 * the context of a thread. Instead, it represents a subtask 
 * assigned to this thread. One issue that arrises because of this is that in
 * many real-world applications, tasks will be assigned to a working thread,
 * and in order to maintain the correct context, we would need to track 
 * a @indicator variable for each worker thread. A change in a working thread's indicator
 * variable implies a potential context switch, which will need to be
 * monitored. In this toy case, this is not an issue, because the run() 
 * function creates a new background thread for each subtask, so each thread
 * only runs in the context of one single subtask. Since this is the case,
 * we don't need to monitor "context switches" when a worker thread starts a
 * new task. However, for real-world applications @indicator variables for each 
 * worker thread will be needed.
 */
struct subtask {
  void (*run)(struct subtask *cur_task, struct input_buffer *);
  // Each subtask (delegator data structure) needs its own unique id, similar
  // to identifier structures.
  DEL_IDENTIFIER int id;
};


/* 
 * The run_upper function creates a new thread, which is responsible for converting the
 * current user_input into a uppercase string and printing it to stdout. 
*/
void run_upper(struct subtask *task, struct input_buffer* input);


int main(int argc, char **argv) {
  char *user_input = NULL;
  size_t n = 1024;
  size_t bytes_read = 0;
  int counter = 0;
  DELEGATOR struct subtask *sub_task = NULL;

  while (1) {
  //Get user input. For this program we want to track what "user_input"
  //caused a set of system calls.
  user_input = (char *) malloc(n);
  bytes_read = getline(&user_input, &n, stdin);

  // The `curr_input` variable is one of our @indicator variables for the unit
  // we are using as context. Therefore, when we see an update to this value,
  // we need to update the context of the current thread to express this
  // update.
  // XXX. During LLVM passes, we need to identify defs of any indicator
  // vars.
  curr_input.user_input = user_input;
  curr_input.size = bytes_read;
  curr_input.id = counter++;
  // Instrumentation: 
  //   thread_context = curr_input.id;

  // Create a new subtask, which is responsible for converting the string
  // into uppercase:
  
  // The creation of the subtask needs to be tracked since it is a
  // delegator. Recal a  delegator data structure represents a structure that 
  // defines a subtask, which will be potentially be executed by a different
  // thread. MPI needs to track creation of delegation structures. 
  // This is because MPI assigns a "context" to each delegation structure,
  // and the delegation structure will inherit the current context that is
  // running in the thread that created/instatiated the new sub task.
  sub_task = malloc(sizeof(struct subtask));
  sub_task->id = counter++;
  // MPIs instrumentation would be similar to the following:
  // 1. Set context for subtask to current context.
  // 2. Update delegation table.
  // sub_task->context = delegation_table[cur_task];
  // delegation_table[sub_task] = sub_task->context.
  sub_task->run = run_upper;
  sub_task->run(sub_task, &curr_input);
  }
  return 0;
}

void *upper(void *in) {
  printf("Background Thread.\n");
  struct input_buffer *input = (struct input_buffer*) in;
  for (size_t i = 0; i < input->size; i++) {
    input->user_input[i] = toupper(input->user_input[i]);
  }
  printf("Input (%lu): %s", input->size, input->user_input);
  return NULL;
}

void run_upper(struct subtask *task, struct input_buffer* input) {
  pthread_t thread;
  int rc = 0;
  rc = pthread_create(&thread, NULL, upper, (void *) &curr_input);
  if (rc) {
    fprintf(stderr, "Error; return code from pthread_create() is %d\n", rc);
    exit(-1);
  }
}

