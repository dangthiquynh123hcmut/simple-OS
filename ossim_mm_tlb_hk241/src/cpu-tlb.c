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
    for (int i = 0; i < mp->maxsz; i++) {
        // Đặt các giá trị của mỗi trang về trạng thái mặc định
        mp->storage[i] = -1; // Hoặc giá trị mặc định phù hợp với loại dữ liệu
    }
    return 0;
}

/*tlballoc - CPU TLB-based allocate a region memory
 *@proc:  Process executing the instruction
 *@size: allocated size 
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int tlballoc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
{
  int addr, val;

  /* By default using vmaid = 0 */
  val = __alloc(proc, 0, reg_index, size, &addr);

  /* TODO update TLB CACHED frame num of the new allocated page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/

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
    printf("Error in tlbread(): read vùng chưa khởi tạo!\n");
    return -1;
  }

  // Lấy ra frame number
  int address = proc->mm->symrgtbl[source].rg_start + offset;
  int pgnum = PAGING_PGN(address);
  //int index_in_tlb = pgnum % proc->tlb->maxsz;
  //printf("ADDRESS = %d, pgnum = %d\n", address, pgnum);

  tlb_cache_read(proc->tlb, proc->pid, pgnum, &frmnum);

#ifdef IODUMP
  if (frmnum >= 0)
    printf("TLB hit at read region=%d offset=%d\n", 
	         source, offset);
  else 
    printf("TLB miss at read region=%d offset=%d\n", 
	         source, offset);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif

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

    if( val == -1 ) printf("Error in tlbread(): __read() trong trường hợp MISS thất bại.\n");

    destination = (uint32_t) data; 
    // frame fpn vừa được đọc và không có trong TLB nên cần thêm vào 
    uint32_t miss_pte = proc->mm->pgd[pgnum];
    int fpn = PAGING_FPN(miss_pte);
  
    tlb_cache_write(proc->tlb, proc->pid, pgnum,(BYTE) fpn); 
  }

  /* TODO update TLB CACHED with frame num of recent accessing page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/

  int des_pgnum = PAGING_PGN(destination);
  uint32_t pte = proc->mm->pgd[des_pgnum];
  int des_fpn = PAGING_FPN(pte);
  // Register destionation vừa được sử dụng để ghi kết quả đọc được vào
  // nên cần cập nhật TLB 
  tlb_cache_write(proc->tlb, proc->pid, des_pgnum, (BYTE) des_fpn); 

  printf("Sau khi read:\n");
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
  MEMPHY_dump(proc->mram);
  
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
    printf("Error in tlbwrite(): write vào vùng chưa khởi tạo!\n");
    return -1;
  }

  int address = proc->mm->symrgtbl[destination].rg_start + offset;
  int pgnum = PAGING_PGN(address);
  //printf("ADDRESS = %d, pgnum = %d\n", address, pgnum);
  tlb_cache_read(proc->tlb, proc->pid, pgnum, &frmnum);

#ifdef IODUMP
  if (frmnum >= 0)
    printf("TLB hit at write region=%d offset=%d value=%d\n",
	          destination, offset, data);
	else
    printf("TLB miss at write region=%d offset=%d value=%d\n",
            destination, offset, data);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif

  if( frmnum >= 0 ) {
    int off = PAGING_OFFST(address);
    int phyaddr = (frmnum << PAGING_ADDR_FPN_LOBIT) + off;

    proc->mram->storage[phyaddr] = data;

    #ifdef IODUMP
      printf("write region = %d offset = %d value = %d\n", destination, offset, data);
      printf("Trước khi write:\n");
    #ifdef PAGETBL_DUMP
      print_pgtbl(proc, 0, -1); // print max TBL
    #endif
      MEMPHY_dump(proc->mram);
    #endif

    // tlb_cache_write(proc->tlb, proc->pid, pgnum, value);
  } else {
    val = __write(proc, 0, destination, offset, data);

    if( val == -1 ) printf("Error in tlbwrite(): __write() trong trường hợp MISS thất bại.\n");
    
    uint32_t miss_pte = proc->mm->pgd[pgnum];
    int fpn = PAGING_FPN(miss_pte);
    //printf("after: %u\n", miss_pte);
    //printf("tlbwrite: pgnum = %d, fpn = %d\n", pgnum, fpn);
    // miss nên cần cập nhật TLB
    int res = tlb_cache_write(proc->tlb, proc->pid, pgnum, fpn); 
    
    printf("Sau khi write:\n");
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
  MEMPHY_dump(proc->mram);
  }

  /* TODO update TLB CACHED with frame num of recent accessing page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/

  return val;
}

//#endif
