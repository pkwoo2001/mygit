#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include "synch.h"

/* States in a thread's life cycle. */
enum thread_status
  {
    THREAD_RUNNING,     /* Running thread. */
    THREAD_READY,       /* Not running but ready to run. */
    THREAD_BLOCKED,     /* Waiting for an event to trigger. */
    THREAD_DYING        /* About to be destroyed. */
  };

/* Thread identifier type.
   You can redefine this to whatever type you like. */
typedef int tid_t;
#define TID_ERROR ((tid_t) -1)          /* Error value for tid_t. */

/* Thread priorities. */
#define PRI_MIN 0                       /* Lowest priority. */
#define PRI_DEFAULT 31                  /* Default priority. */
#define PRI_MAX 63                      /* Highest priority. */

/* A kernel thread or user process.

   Each thread structure is stored in its own 4 kB page.  The
   thread structure itself sits at the very bottom of the page
   (at offset 0).  The rest of the page is reserved for the
   thread's kernel stack, which grows downward from the top of
   the page (at offset 4 kB).  Here's an illustration:

        4 kB +---------------------------------+
             |          kernel stack           |
             |                |                |
             |                |                |
             |                V                |
             |         grows downward          |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             +---------------------------------+
             |              magic              |
             |                :                |
             |                :                |
             |               name              |
             |              status             |
        0 kB +---------------------------------+

   The upshot of this is twofold:

      1. First, `struct thread' must not be allowed to grow too
         big.  If it does, then there will not be enough room for
         the kernel stack.  Our base `struct thread' is only a
         few bytes in size.  It probably should stay well under 1
         kB.

      2. Second, kernel stacks must not be allowed to grow too
         large.  If a stack overflows, it will corrupt the thread
         state.  Thus, kernel functions should not allocate large
         structures or arrays as non-static local variables.  Use
         dynamic allocation with malloc() or palloc_get_page()
         instead.

   The first symptom of either of these problems will probably be
   an assertion failure in thread_current(), which checks that
   the `magic' member of the running thread's `struct thread' is
   set to THREAD_MAGIC.  Stack overflow will normally change this
   value, triggering the assertion. */
/* The `elem' member has a dual purpose.  It can be an element in
   the run queue (thread.c), or it can be an element in a
   semaphore wait list (synch.c).  It can be used these two ways
   only because they are mutually exclusive: only a thread in the
   ready state is on the run queue, whereas only a thread in the
   blocked state is on a semaphore wait list. */
struct thread
  {
    /* Owned by thread.c. */
    tid_t tid;                          /* Thread identifier. */
    enum thread_status status;          /* Thread state. */
    char name[16];                      /* Name (for debugging purposes). */
    uint8_t *stack;                     /* Saved stack pointer. */
    int priority;                       /* Priority. */
    struct list_elem allelem;           /* List element for all threads list. */

    /* Shared between thread.c and synch.c. */
    struct list_elem elem;              /* List element. */
   /*알람 클락 추가 구현*/
    int64_t wakeup; // 일어날 tick: cur_tick + 잠자는 시간
   /*donation 추가 구현*/
   struct list donators_list; // donate 해준 threads
   struct list_elem donate_elem; // donators list_elem
   int own_priority; // 원래 자신의 priority
   struct lock* waiting_lock; // thread가 acuire한 lock: 기다리고 있음
   /*advanced scheduler 추가 구현*/
   int nice;
   int recent_cpu;
#ifdef USERPROG
    /* Owned by userprog/process.c. */
    uint32_t *pagedir;                  /* Page directory. */
    struct list child_list;
    struct list_elem child_list_elem;
    struct semaphore wait_child_sema; // 부모가 child 끝날때까지 대기
    struct semaphore wait_parent_sema; // child가 부모가 exit_status 받을때까지 대기
    struct semaphore load_sema;
    struct semaphore wait_parent_load_error; // load error 받기 위해 대기
    bool waited; // parent가 wait 호출했는지 유무
    bool loaded; // load되었는지 유무
    int exit_status;
    struct file* fd[128];
    struct file* exec_file;
#endif

    /* Owned by thread.c. */
    unsigned magic;                     /* Detects stack overflow. */
  };

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
extern bool thread_mlfqs;

void thread_init (void);
void thread_start (void);

void thread_tick (void);
void thread_print_stats (void);

typedef void thread_func (void *aux);
tid_t thread_create (const char *name, int priority, thread_func *, void *);

void thread_block (void);
void thread_unblock (struct thread *);

struct thread *thread_current (void);
tid_t thread_tid (void);
const char *thread_name (void);

void thread_exit (void) NO_RETURN;
void thread_yield (void);

/* Performs some operation on thread t, given auxiliary data AUX. */
typedef void thread_action_func (struct thread *t, void *aux);
void thread_foreach (thread_action_func *, void *);

int thread_get_priority (void);
void thread_set_priority (int);

int thread_get_nice (void);
void thread_set_nice (int);
int thread_get_recent_cpu (void);
int thread_get_load_avg (void);

/*ass1 알람 클락 구현 추가 함수*/
void thread_sleep(struct thread *cur_thread, int64_t tick);
bool compare_tick_increasing (const struct list_elem *prev, const struct list_elem *next);
void thread_wake (int64_t cur_tick);
/*ass1 priority scheduler 추가 구현 함수*/
bool compare_priority_decreasing (const struct list_elem *prev, const struct list_elem *next, void *aux UNUSED);
bool check_priority_yield(void); //ready_list 첫번째 element랑 비교해서 current thread priority가 더 작으면 yield
/*inversion 추가 구현*/
void sorting_ready_list(void);
bool donator_p_decreaing(const struct list_elem *prev, const struct list_elem *next, void *aux UNUSED);
/*advanced scheduler 추가 구현 함수*/
int int2fp(int n);
int fp2int(int x);
int fp2int_round(int x);
int add_fp(int x, int y);
int sub_fp(int x, int y);
int add_fp_int(int x, int n);
int sub_fp_int(int x, int n);
int mul_fp(int x, int y);
int mul_fp_int(int x, int n);
int div_fp(int x, int y);
int div_fp_int(int x, int n);
void mlfqs_recal_priority (void);
void mlfqs_recal_recentcpu(void);
void mlfqs_cal_load_avg (void);
void mlfqs_increment_recent_cpu (void);
void update_thread_state(const int64_t ticks);

#endif /* threads/thread.h */