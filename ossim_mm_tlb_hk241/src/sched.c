#include "queue.h"
#include "sched.h"
#include <pthread.h>
#include "mm.h"

#include <stdlib.h>
#include <stdio.h>
static struct queue_t ready_queue;
static struct queue_t run_queue;
static pthread_mutex_t queue_lock;

#ifdef MLQ_SCHED
static struct queue_t mlq_ready_queue[MAX_PRIO];
#endif

int queue_empty(void) {
#ifdef MLQ_SCHED
	unsigned long prio;
	for (prio = 0; prio < MAX_PRIO; prio++)
		if(!empty(&mlq_ready_queue[prio])) 
			return -1;
#endif
	return (empty(&ready_queue) && empty(&run_queue));
}

void init_scheduler(void) {
#ifdef MLQ_SCHED
    int i ;

	for (i = 0; i < MAX_PRIO; i ++) {
		mlq_ready_queue[i].size = 0;
		mlq_ready_queue[i].slots = 0;
	}
#endif
	ready_queue.size = 0;
	run_queue.size = 0;
	pthread_mutex_init(&queue_lock, NULL);
}

#ifdef MLQ_SCHED
/* 
 *  Stateful design for routine calling
 *  based on the priority and our MLQ policy
 *  We implement stateful here using transition technique
 *  State representation   prio = 0 .. MAX_PRIO, curr_slot = 0..(MAX_PRIO - prio)
 */

void reset_slots ()
{
    unsigned long prio;
    for (prio = 0; prio < MAX_PRIO; prio++)
        mlq_ready_queue[prio].slots = 0; // Reset the round-robin counter
                                         // for each every queue
}

struct pcb_t * get_mlq_proc(void) {
    // pthread_mutex_lock(&queue_lock);
    // struct pcb_t * proc = NULL;
    // for (unsigned long prio = 0; prio < MAX_PRIO; prio++) {
    //     if (!empty(&mlq_ready_queue[prio])) {
    //         proc = dequeue(&mlq_ready_queue[prio]);
    //         break;
    //     }
    // }

    // pthread_mutex_unlock(&queue_lock);
    // return proc;

	struct pcb_t *proc = NULL;
    if (queue_empty () == 1)
        return proc;

    int designated_slots;
	int count = 0;
    while(count<MAX_PRIO)
        {
            designated_slots = MAX_PRIO - count;
            struct queue_t *priority_queue = &mlq_ready_queue[count];
            if (priority_queue->size != 0)
                {
                    if (priority_queue->slots == designated_slots)
                        {
                            count++;
			    if(count==MAX_PRIO){
                                
                            	count = 0;
                    		reset_slots ();
                            }
                            continue;
                        }
                    else
                        {
                            pthread_mutex_lock (&queue_lock);
                            //printf("%s", "flag");
                            proc = dequeue (priority_queue);
                            priority_queue->slots++;
                            pthread_mutex_unlock (&queue_lock);
                            return proc;
                        }
                }

            count++;

            if (count == MAX_PRIO) 
                {
                    count = 0;
                    reset_slots ();
                }
        }
	return NULL;
}

void put_mlq_proc(struct pcb_t * proc) {
	pthread_mutex_lock(&queue_lock);
	enqueue(&mlq_ready_queue[proc->prio], proc);
	pthread_mutex_unlock(&queue_lock);
}

void add_mlq_proc(struct pcb_t * proc) {
	pthread_mutex_lock(&queue_lock);
	enqueue(&mlq_ready_queue[proc->prio], proc);
	pthread_mutex_unlock(&queue_lock);	
}

//dọn rác
void finish_proc(struct pcb_t **proc){
	pthread_mutex_lock(&queue_lock);
	mlq_ready_queue[(*proc)->prio].slots++;
#ifdef CPU_TLB
	tlb_flush_tlb_of((*proc),(*proc)->tlb);
	struct vm_area_struct *vma = get_vma_by_num((*proc)->mm, 0);
	struct mm_struct* mm = (*proc)->mm;
	int pg_st = vma->vm_start;
	int pg_ed = (vma->vm_end-1) / PAGING_PAGESZ ;

	for(int pgn = pg_st; pgn <=pg_ed; pgn++) {
		int pte = mm->pgd[pgn];
		if(PAGING_PAGE_PRESENT(pte)) {
			int fpn = PAGING_FPN(pte);
			MEMPHY_free_frame((*proc)->mram, fpn);
		}
		else {
			int fpn = PAGING_SWP(pte);
			MEMPHY_free_frame((*proc)->active_mswp, fpn);
		}
	}
#endif
	pthread_mutex_unlock(&queue_lock);
	free(*proc);
}

struct pcb_t * get_proc(void) {
	return get_mlq_proc();
}

void put_proc(struct pcb_t * proc) {
	return put_mlq_proc(proc);
}

void add_proc(struct pcb_t * proc) {
	return add_mlq_proc(proc);
}

#else

struct pcb_t * get_proc(void) {
    pthread_mutex_lock(&queue_lock);

    struct pcb_t * proc = NULL;
    if (!empty(&ready_queue)) {
        proc = dequeue(&ready_queue);
    }

    pthread_mutex_unlock(&queue_lock);
    return proc;
}



void put_proc(struct pcb_t * proc) {
	pthread_mutex_lock(&queue_lock);
	enqueue(&ready_queue, proc);
	pthread_mutex_unlock(&queue_lock);
}

void add_proc(struct pcb_t * proc) {
	pthread_mutex_lock(&queue_lock);
	enqueue(&ready_queue, proc);
	pthread_mutex_unlock(&queue_lock);	
}
#endif

