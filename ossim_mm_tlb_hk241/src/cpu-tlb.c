/*
 * Copyright (C) 2024 pdnguyen of the HCMC University of Technology
 */
/*
 * Source Code License Grant: Authors hereby grants to Licensee 
 * a personal to use and modify the Licensed Source Code for 
 * the sole purpose of studying during attending the course CO2018.
 */
//#ifdef CPU_TLB
/*
 * CPU TLB
 * TLB module cpu/cpu-tlb.c
 */
 
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

static pthread_mutex_t mmvm_lock = PTHREAD_MUTEX_INITIALIZER;
static int stat_hit_time=0;
static int stat_miss_time=0;
int tlb_change_all_page_tables_of(struct pcb_t *proc,  struct memphy_struct * mp)
{
  /* TODO update all page table directory info 
   *      in flush or wipe TLB (if needed)
   */
    return tlb_flush_tlb_of(proc, mp);
}

int tlb_flush_tlb_of(struct pcb_t *proc, struct memphy_struct * mp)
{
  // Lặp qua tất cả các trang trong cache TLB
    pthread_mutex_lock(&mmvm_lock);
    int i;
    for (i = 0; i < mp->maxsz; i++) {
        // Đặt các giá trị của mỗi trang về trạng thái mặc định
        mp->help[i].valid = 0;
    }
    pthread_mutex_unlock(&mmvm_lock);
    return 0;
}

/*tlballoc - CPU TLB-based allocate a region memory
 *@proc:  Process executing the instruction
 *@size: allocated size 
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int tlballoc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
{

  // printf("Before ALLOC:\n");
  // if(proc->tlb->tlb_fifo) {
  //   struct node* temp = proc->tlb->tlb_fifo;
  //   while(temp) {
  //     printf("TLB FIFO: %d\n", temp->data); 
  //     temp = temp->next;
  //   }
  // } else {
  //   printf("TLB FIFO is NULL\n"); 
  // }

  int addr, val;

  /* By default using vmaid = 0 */
  val = __alloc(proc, 0, reg_index, size, &addr);
  if(val == -1 ) {
    printf("Error in cpu-tlb.c/ tlballoc(): Allocation fails.\n");
    return -1;
  }

  /* TODO update TLB CACHED frame num of the new allocated page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/
  
  int realSize = PAGING_PAGE_ALIGNSZ(size);
  int numOfPage = realSize / PAGING_PAGESZ;

  int i;
  for(i = 0; i<numOfPage; i++) {
    int pgnum = PAGING_PGN(addr);
    uint32_t pte = proc->mm->pgd[pgnum];
    int fpn = pte & 0xFFF; //PAGING_FPN(pte);
    tlb_cache_write(proc->tlb, proc->pid, pgnum,(BYTE) fpn); 

    addr += PAGING_PAGESZ;
  }

#ifdef IODUMP
  printf("In tlballoc print:\n");

  pthread_mutex_lock(&mmvm_lock);
  print_pgtbl(proc, 0, -1); // print max TBL
  pthread_mutex_unlock(&mmvm_lock);

   printf("In tlballoc print end.\n");
#endif
  //printf("pgnum = %d, fpn = %d.\n", pgnum, fpn);

  

  // printf("After ALLOC:\n");
  // if(proc->tlb->tlb_fifo) {
  //   struct node* temp = proc->tlb->tlb_fifo;
  //   while(temp) {
  //     int idx = temp->data;
  //     printf("TLB FIFO: pgnum = %d\n", proc->tlb->help[idx].pgnum); 
  //     temp = temp->next;
  //   }
  // } else {
  //   printf("TLB FIFO is NULL\n"); 
  // }

  return val;
}

/*pgfree - CPU TLB-based free a region memory
 *@proc: Process executing the instruction
 *@size: allocated size 
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int tlbfree_data(struct pcb_t *proc, uint32_t reg_index)
{
  __free(proc, 0, reg_index);

  /* TODO update TLB CACHED frame num of freed page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/
  int address = proc->mm->symrgtbl[reg_index].rg_start;
  int pgnum = PAGING_PGN(address);

  pthread_mutex_lock(&mmvm_lock);
  int i;
  for(i = 0; i<proc->tlb->maxsz; i++) {
    if( proc->tlb->help[i].valid == 1 && proc->tlb->help[i].pid == proc->pid && proc->tlb->help[i].pgnum == pgnum) {
      proc->tlb->help[i].valid == 0;

      // xóa trong fifo
      struct node* deleted = proc->tlb->tlb_fifo;
      if (deleted->data == i) {
          // Nếu nút cần xóa là nút đầu tiên
          proc->tlb->tlb_fifo = proc->tlb->tlb_fifo->next;
          free(deleted);
      } else {
          struct node* prev = deleted;
          deleted = deleted->next;
          while (deleted != NULL) {
              if (deleted->data == i) {
                  prev->next = deleted->next;
                  free(deleted);
                  break; // Đã xóa nút, không cần duyệt nữa
              }
              prev = deleted;
              deleted = deleted->next;
          }
      }

      proc->tlb->help[i].fpn = -1; 
      //TLBMEMPHY_write(proc->tlb, i, -1);
    }
  }
  pthread_mutex_unlock(&mmvm_lock);
  //printf("Error in tlbfree_data(): Không tìm thấy trong TLB.\n");

  return 0;
}


/*tlbread - CPU TLB-based read a region memory
 *@proc: Process executing the instruction
 *@source: index of source register
 *@offset: source address = [source] + [offset]
 *@destination: destination storage
 */
int tlbread(struct pcb_t * proc, uint32_t source,
            uint32_t offset, 	uint32_t destination) 
{ 
  //if( proc->tlb == NULL ) printf("TLB is NULL\n");
  int val;
  BYTE data, frmnum = -1;
	
  /* TODO retrieve TLB CACHED frame num of accessing page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/
  /* frmnum is return value of tlb_cache_read/write value*/

  if( proc->mm->symrgtbl[source].rg_start == proc->mm->symrgtbl[source].rg_end ) {
    printf("Error in tlbread(): Read uninitialized memory region!\n");
    return -1;
  }

  // Lấy ra frame number
  int address = proc->mm->symrgtbl[source].rg_start + offset;
  int pgnum = PAGING_PGN(address);
  //int index_in_tlb = pgnum % proc->tlb->maxsz;
  //printf("ADDRESS = %d, pgnum = %d\n", address, pgnum);

  // tlb_cache_read(proc->tlb, proc->pid, pgnum, &frmnum, index);
  tlb_cache_read(proc->tlb, proc->pid, pgnum, &frmnum);
#ifdef IODUMP
    if (frmnum >= 0) {
    printf("\tTLB hit at read region=%d offset=%d\n", 
	         source, offset);
    pthread_mutex_lock(&mmvm_lock);
    stat_hit_time ++;
    pthread_mutex_unlock(&mmvm_lock);
  }
  else {    
    printf("\tTLB miss at read region=%d offset=%d\n", 
	         source, offset);
    pthread_mutex_lock(&mmvm_lock);
    stat_miss_time ++;
    pthread_mutex_unlock(&mmvm_lock);
   // tlb_cache_write(proc->tlb, proc, pgn);
 printf("\tPrint TLB before reading from memory:\n" );
   #ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif
  }

  printf("\n");
  float rate=stat_hit_time/(stat_hit_time+ stat_miss_time);
  printf("Rate hit now: %.2f\n",rate);
  printf("\n");

  // Hit
  if( frmnum >= 0 ) {
    // truy cập mram trực tiếp
    int off = PAGING_OFFST(address);
    // frmnum lấy ra từ tlb_cache_read
    frmnum = (int) frmnum;
    int phyaddr = (frmnum << PAGING_ADDR_FPN_LOBIT) + off;
    
    destination = (uint32_t) proc->mram->storage[phyaddr];
  } 
  // Miss
  else {
    // sử dụng page table
    val = __read(proc, 0, source, offset, &data);

    if( val == -1 ){ printf("Error in tlbread(): __read() Reading fails when a TLB miss occurs.\n");
     return -1;
    }

    destination = (uint32_t) data; 
    // frame fpn vừa được đọc và không có trong TLB nên cần thêm vào 
    // uint32_t miss_pte = proc->mm->pgd[pgnum];
    // int fpn = miss_pte & 0xFFF; //PAGING_FPN(miss_pte);
  
    // tlb_cache_write(proc->tlb, proc->pid, pgnum,(BYTE) fpn); 
    // int des_index = destination % proc->tlb->maxsz;
    // tlb_cache_write(proc->tlb, proc->pid, des_pgnum, (BYTE) des_fpn, des_index); 
printf("After reading from memory into TLB\n");
#ifdef PAGETBL_DUMP

  pthread_mutex_lock(&mmvm_lock);
  print_pgtbl(proc, 0, -1); //print max TBL
  pthread_mutex_unlock(&mmvm_lock);

#endif
  MEMPHY_dump(proc->mram);
}

  /* TODO update TLB CACHED with frame num of recent accessing page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/

  BYTE occured = -1;
  int des_pgnum = PAGING_PGN(destination);
  tlb_cache_read(proc->tlb, proc->pid, des_pgnum, &occured);
  if(occured == -1) {
    uint32_t pte = proc->mm->pgd[des_pgnum];
    int des_fpn = pte & 0xFFF; //PAGING_FPN(pte);
    tlb_cache_write(proc->tlb, proc->pid, des_pgnum, (BYTE) des_fpn); 
  }
  return val;
}

/*tlbwrite - CPU TLB-based write a region memory
 *@proc: Process executing the instruction
 *@data: data to be wrttien into memory
 *@destination: index of destination register
 *@offset: destination address = [destination] + [offset]
 */
int tlbwrite(struct pcb_t * proc, BYTE data,
             uint32_t destination, uint32_t offset)
{
  int val;
  BYTE frmnum = -1;

  /* TODO retrieve TLB CACHED frame num of accessing page(s))*/
  /* by using tlb_cache_read()/tlb_cache_write()
  frmnum is return value of tlb_cache_read/write value*/
  
  if( proc->mm->symrgtbl[destination].rg_start == proc->mm->symrgtbl[destination].rg_end ) {
    printf("Error in tlbwrite(): Write to uninitialized memory region!\n");
    return -1;
  }

  int address = proc->mm->symrgtbl[destination].rg_start + offset;
  int pgnum = PAGING_PGN(address);
  printf("ADDRESS = %d, pgnum = %d\n", address, pgnum);


  // tlb_cache_read(proc->tlb, proc->pid, pgnum, &frmnum, index);
  tlb_cache_read(proc->tlb, proc->pid, pgnum, &frmnum);

#ifdef IODUMP
  
  if (frmnum >= 0) {
    printf("\tTLB hit at write region=%d offset=%d value=%d\n",
	          destination, offset, data);
    pthread_mutex_lock(&mmvm_lock);
    stat_hit_time ++;
    pthread_mutex_unlock(&mmvm_lock);
  }
	else {
    printf("\tTLB miss at write region=%d offset=%d value=%d\n",
            destination, offset, data);
    pthread_mutex_lock(&mmvm_lock);
    stat_miss_time ++;
    pthread_mutex_unlock(&mmvm_lock);
    //tlb_cache_write(proc->tlb, proc, pgn);
  }
  printf("\n");
  float rate = (float)stat_hit_time / (stat_hit_time+ stat_miss_time);
  printf("Rate hit now: %.2f\n",rate);
  printf("\n");
  
  printf("pgnum = %d, fpn = %d.\n", pgnum, frmnum);
  printf("before write:\n");
#ifdef PAGETBL_DUMP

pthread_mutex_lock(&mmvm_lock);
  print_pgtbl(proc, 0, -1); // print max TBL
pthread_mutex_unlock(&mmvm_lock);

#endif
  MEMPHY_dump(proc->mram);
#endif

  if( frmnum >= 0 ) {
    int off = PAGING_OFFST(address);
    int phyaddr = (frmnum << PAGING_ADDR_FPN_LOBIT) + off;
    printf("phyaddr = %d\n", phyaddr);
    proc->mram->storage[phyaddr] = data;
    // tlb_cache_write(proc->tlb, proc->pid, pgnum, value);
  } else {
    val = __write(proc, 0, destination, offset, data);

    if( val == -1 ) {printf("Error in tlbwrite(): __write() Writing fails when a TLB miss occurs.\n");
      return -1;
    }
    
    uint32_t miss_pte = proc->mm->pgd[pgnum];
    int fpn = miss_pte & 0xFFF; //PAGING_FPN(miss_pte);

    // miss nên cần cập nhật TLB
    // int res = tlb_cache_write(proc->tlb, proc->pid, pgnum, fpn, index); 
    int res = tlb_cache_write(proc->tlb, proc->pid, pgnum, fpn); 

    printf("fpn = %d, pgnum = %d\n", fpn, pgnum);
  }

  printf("After write:\n");
#ifdef PAGETBL_DUMP
pthread_mutex_lock(&mmvm_lock);
  print_pgtbl(proc, 0, -1); //print max TBL
pthread_mutex_unlock(&mmvm_lock);

#endif
  MEMPHY_dump(proc->mram);
  

  /* TODO update TLB CACHED with frame num of recent accessing page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/

  return val;
}

//#endif
