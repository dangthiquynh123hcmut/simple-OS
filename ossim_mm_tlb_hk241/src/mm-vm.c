// #ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Virtual memory module mm/mm-vm.c
 */

#include "string.h"
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>

/*enlist_vm_freerg_list - add new rg to freerg_list
 *@mm: memory region
 *@rg_elmt: new region
 *
 */
// add head, add rg_elmt in the head of mm->mmap->vm_freerg_list (vmaid = 0)
int enlist_vm_freerg_list(struct mm_struct *mm, struct vm_rg_struct* rg_elmt)
{
  struct vm_rg_struct *rg_node = mm->mmap->vm_freerg_list;

  if (rg_elmt->rg_start >= rg_elmt->rg_end)
    return -1;

  if (rg_node != NULL)
    rg_elmt->rg_next = rg_node;

  /* Enlist the new region */
  mm->mmap->vm_freerg_list = rg_elmt;

  return 0;
}

/*get_vma_by_num - get vm area by numID
 *@mm: memory region
 *@vmaid: ID vm area to alloc memory region
 *
 */
struct vm_area_struct *get_vma_by_num(struct mm_struct *mm, int vmaid)
{
  struct vm_area_struct *pvma = mm->mmap;

  if (mm->mmap == NULL)
    return NULL;

  int vmait = 0;

  while (vmait < vmaid)
  {
    if (pvma == NULL)
      return NULL;

    vmait++;
    pvma = pvma->vm_next;
  }

  return pvma;
}

/*get_symrg_byid - get mem region by region ID
 *@mm: memory region
 *@rgid: region ID act as symbol index of variable
 *
 */
struct vm_rg_struct *get_symrg_byid(struct mm_struct *mm, int rgid)
{
  if (rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ || mm->symrgtbl[rgid].rg_start == mm->symrgtbl[rgid].rg_end)
    return NULL;

  return &mm->symrgtbl[rgid];
}

/*__alloc - allocate a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *@alloc_addr: address of allocated memory region
 *
 */
int __alloc(struct pcb_t *caller, int vmaid, int rgid, int size, int *alloc_addr)
{
  /*Allocate at the toproof */
  struct vm_rg_struct rgnode;
  //printf("Checked here\n");
  if (get_free_vmrg_area(caller, vmaid, size, &rgnode) == 0)
  { 
    //printf("Checked here\n");
    caller->mm->symrgtbl[rgid].rg_start = rgnode.rg_start;
    caller->mm->symrgtbl[rgid].rg_end = rgnode.rg_end;

    *alloc_addr = rgnode.rg_start;

    printf("Case 1: ");
    printf ("Alloc region = %d, size = %d, pid = %d\n", rgid, size, caller->pid);
    printf("Check: region->rg_start = %d, region->rg_end = %d.\n", caller->mm->symrgtbl[rgid].rg_start, caller->mm->symrgtbl[rgid].rg_end);
    return 0;
  }
  //printf("Checked here\n");

  /* TODO get_free_vmrg_area FAILED handle the region management (Fig.6)*/
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  if (cur_vma == NULL)
  {
    printf ("Error in: mm-vm.c/ __alloc() :");
    printf (" virtual memory area %d does not exist.\n", vmaid);
    return -1;
  }
  int gap = cur_vma->vm_end - cur_vma->sbrk;
  printf("Gap: %d\n", gap);

  if (size <= gap)
  {
    caller->mm->symrgtbl[rgid].rg_start = cur_vma->sbrk;
    caller->mm->symrgtbl[rgid].rg_end = cur_vma->sbrk + size;
    *alloc_addr = cur_vma->sbrk;
    cur_vma->sbrk = cur_vma->sbrk + size;

    printf("Case 2: ");
    printf ("Alloc region = %d, size = %d, pid = %d\n", rgid, size, caller->pid);
    printf("Check: region->rg_start = %d, region->rg_end = %d.\n", caller->mm->symrgtbl[rgid].rg_start, caller->mm->symrgtbl[rgid].rg_end);
    return 0;
  }

  /*Attempt to increate limit to get space */
  int inc_sz = PAGING_PAGE_ALIGNSZ(size - gap);
  // int inc_limit_ret
  int old_sbrk;

  old_sbrk = cur_vma->sbrk;

  /* TODO INCREASE THE LIMIT
   * inc_vma_limit(caller, vmaid, inc_sz)
   */
  if (inc_vma_limit (caller, vmaid, inc_sz) != 0)
  {
    printf ("Error in: mm-vm.c/ __alloc() :");
    printf (" inc_vma_limit() The limit cannot be increased.\n");
    return -1;
  }

  /*Successful increase limit */
  caller->mm->symrgtbl[rgid].rg_start = old_sbrk;
  caller->mm->symrgtbl[rgid].rg_end = old_sbrk + size;

  *alloc_addr = old_sbrk;
  
  //printf("alloc_ADDRESS = %d.\n", *alloc_addr);

  cur_vma->sbrk = old_sbrk + size;

  printf("Case 3: ");
  printf ("Alloc region = %d, size = %d, pid = %d\n", rgid, size, caller->pid);
  printf("Check: region->rg_start = %d, region->rg_end = %d.\n", caller->mm->symrgtbl[rgid].rg_start, caller->mm->symrgtbl[rgid].rg_end);
  return 0;
}

/*__free - remove a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *
 */

int __free(struct pcb_t *caller, int vmaid, int rgid)
{

  if (rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
    return -1;

  /* TODO: Manage the collect freed region to freerg_list */
  struct vm_rg_struct *temp = get_symrg_byid(caller->mm, rgid);
  if (temp == NULL) /* Invalid memory identify */
  {
    printf ("Error in: mm-vm.c/ __free() :");
    //printf ("Region %d hoặc virtual memory area %d không tồn tại.\n", rgid, vmaid);
    printf (" region %d does not exist.\n", rgid);
    return -1;
  }

  // xóa trong fifo
  int pgnum = PAGING_PGN(temp->rg_start);
  struct pgn_t* deleted = caller->mm->fifo_pgn;
  if (deleted->pgn == pgnum) {
    // Nếu nút cần xóa là nút đầu tiên
    caller->mm->fifo_pgn = caller->mm->fifo_pgn->pg_next;
    free(deleted);
  } else {
    struct pgn_t* prev = deleted;
    deleted = deleted->pg_next;
    while (deleted != NULL) {
      if (deleted->pgn == pgnum) {
        prev->pg_next = deleted->pg_next;
        free(deleted);
        break; // Đã xóa nút, không cần duyệt nữa
      }
      prev = deleted;
      deleted = deleted->pg_next;
    }
  }

  struct vm_rg_struct *rgnode = malloc(sizeof(struct vm_rg_struct));
  rgnode->rg_start = temp->rg_start;
  rgnode->rg_end = temp->rg_end;
  rgnode->rg_next = NULL;

  caller->mm->symrgtbl[rgid].rg_start = -1;
  caller->mm->symrgtbl[rgid].rg_end = -1;

  /*enlist the obsoleted memory region */
  enlist_vm_freerg_list(caller->mm, rgnode);

  printf ("Free region = %d, pid = %d.\n", rgid, caller->pid);

  return 0;
}

/*pgalloc - PAGING-based allocate a region memory
 *@proc:  Process executing the instruction
 *@size: allocated size
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int pgalloc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
{
  int addr;

  /* By default using vmaid = 0 */
  return __alloc(proc, 0, reg_index, size, &addr);
}

/*pgfree - PAGING-based free a region memory
 *@proc: Process executing the instruction
 *@size: allocated size
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */

int pgfree_data(struct pcb_t *proc, uint32_t reg_index)
{
  return __free(proc, 0, reg_index);
}

/*pg_getpage - get the page in ram
 *@mm: memory region
 *@pagenum: PGN
 *@framenum: return FPN
 *@caller: caller
 *
 */
// pgn = PAGING_PGN (addr); // get the page
// pgn lấy từ 14 bit đầu của address
int pg_getpage(struct mm_struct *mm, int pgn, int *fpn, struct pcb_t *caller)
{
  if (pgn < 0 || pgn >= PAGING_MAX_PGN) // check if pgn is invalid
  {
    return -1;
  }

  uint32_t pte = mm->pgd[pgn];

  if (!PAGING_PAGE_PRESENT(mm->pgd[pgn]))
  { /* Page is not online, make it actively living */
    int freefpn;
    int get_freefp_status = MEMPHY_get_freefp(caller->mram, &freefpn);

    if (get_freefp_status != -1)
    {
      //printf("check pte = %u\n", mm->pgd[pgn]);
      pte_set_fpn(&mm->pgd[pgn], freefpn);

      uint32_t miss_pte = mm->pgd[pgn];
      int fpn = miss_pte & 0xFFF;
      //printf("check pgn = %d, fpn = %d, mm->pgd[pgn] = %u\n", pgn, fpn, mm->pgd[pgn]);
      //printf("check pte = %u, pgnu = %d\n", pte, pgn);

      printf("Successfully acquired a free physical frame: pgnum = %d, freefpn = %d.\n", pgn, freefpn);

      //uint32_t miss_pte = mm->pgd[pgn];
      //printf("check pte: %u\n", miss_pte);
      //int fpn = PAGING_FPN(miss_pte);
      //printf("Get fpn from mm->pgd[pgn]: %d\n", fpn);
    }
    else
    {

      int vicpgn; 
      /* TODO: Play with your paging theory here */
      /* Find victim page */
      if (find_victim_page(caller->mm, &vicpgn) == -1)
      {
        printf("Failed to retrieve victim page.\n");
        return -1;
      }
      
      int temp = mm->pgd[vicpgn]; //32 bit
      int swpfpn;                
      if (MEMPHY_get_freefp(caller->active_mswp, &swpfpn) == -1)
      {
        return -1; // If current mswp full, return error.
      }
      /* Do swap frame from MEMRAM to MEMSWP and vice versa*/
      /* Copy victim frame to swap */
     
      // temp = mm->pgd[vicpgn] ~ 32 bit => lấy ra bit 0-12 là fpn
      // swpfpn ~ fpn
      uint32_t sourceRAM = temp & 0xFFF;  //PAGING_FPN(temp);
      __swap_cp_page(caller->mram, sourceRAM, caller->active_mswp, swpfpn);
      /* Copy target frame from swap to mem */

      // pte = mm->pgd[pgn] hiện đang là 25 bit (đang ở swap)
      int tgtfpn = PAGING_SWP(mm->pgd[pgn]); // the target frame storing our variable
      int dstfpn_in = temp & 0xFFF; // PAGING_FPN(temp);
      __swap_cp_page(caller->active_mswp, tgtfpn, caller->mram, dstfpn_in);

     /* Update its online status of the target page */
      pte_set_fpn(&mm->pgd[pgn], sourceRAM); // pgd[pgn] đang là 25 bit (trên SWAP) => set lại thành 32 bit

      /* Update page table */
      int phyaddr_swp = (swpfpn << PAGING_ADDR_FPN_LOBIT);  // dịch trái để lấy offset
      int swptyp = 0; // In this assignment, we assume swptyp = 0
      int swpoff = phyaddr_swp; // SWP OFFSET, where the frame actually is on MSWP
      pte_set_swap(&mm->pgd[vicpgn], swptyp, swpoff); // set pgd[vicpgn] thành 25 bit (đã chuyển ra SWAP)

      // chỉnh bit 31 của pte thành present
      SETBIT(pte, PAGING_PTE_PRESENT_MASK);  // Make pte "present"
                                             // Duplicate with
                                             // pte_set_fpn's macro.
                                             // However, this macro is
                                             // still put here to
                                             // clarify the flow of
                                             // pg_getpage algorithm.
      // chỉnh bit 31 của temp thành unpresent
      CLRBIT(temp, PAGING_PTE_PRESENT_MASK); // Make vicpte "unpresent"
                                          // Note that this CLRBIT must
                                          
      mm->pgd[pgn] = pte;     // Update page table
      mm->pgd[vicpgn] = temp; // Update page table

      printf("Swapped sucessfully, frame %d updated.\n", dstfpn_in); // come AFTER pte_set_swap()
     
    }
  }
#ifdef CPU_TLB
  /* Update its online status of TLB (if needed) */
#endif
  // add head => first (page đến sớm nhất) sẽ ở tail
  enlist_pgn_node(&caller->mm->fifo_pgn, pgn);


*fpn = mm->pgd[pgn] & 0xFFF; //PAGING_FPN(mm->pgd[pgn]);
// printf("check 331 fpn = %d\n");

return 0;
}

/*pg_getval - read value at given offset
 *@mm: memory region
 *@addr: virtual address to acess
 *@value: value
 *
 */
int pg_getval(struct mm_struct *mm, int addr, BYTE *data, struct pcb_t *caller)
{
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  /* Get the page to MEMRAM, swap from MEMSWAP if needed */
  if (pg_getpage(mm, pgn, &fpn, caller) != 0)
  {
    printf ("Error in: mm-vm.c/ pg_getval() :");
    printf (" pg_getpage() is not sucessful.\n");
    return -1; /* invalid page access */
  }

  int phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off;

  if (MEMPHY_read (caller->mram, phyaddr, data) != 0)
  {
    printf ("Error in: mm-vm.c / pg_getval() :");
    printf (" MEMPHY_write() is not sucessful.\n");
    return -1;
  }

  return 0;
}

/*pg_setval - write value to given offset
 *@mm: memory region
 *@addr: virtual address to acess
 *@value: value
 *
 */
int pg_setval(struct mm_struct *mm, int addr, BYTE value, struct pcb_t *caller)
{
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  /* Get the page to MEMRAM, swap from MEMSWAP if needed */
  if (pg_getpage(mm, pgn, &fpn, caller) != 0) 
  {
    printf ("Error in: mm-vm.c/ pg_setval() :");
    printf (" pg_getpage() is not sucessful.\n");
    return -1; /* invalid page access */
  }

  int phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off;

  if (MEMPHY_write (caller->mram, phyaddr, value) != 0)
  {
    printf ("Error in: mm-vm.c / pg_setval() :");
    printf (" MEMPHY_write() is not sucessful.\n");
    return -1;
  }

  return 0;
}

/*__read - read value in region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@offset: offset to acess in memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *
 */
int __read(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE *data)
{
  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);

  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  if (currg == NULL) /* Invalid memory identify */
  {
    printf ("Error in: mm-vm.c/ __read() :");
    printf (" region %d does not exist.\n", rgid, vmaid);
    return -1;
  }

  if (currg->rg_start + offset < currg->rg_start || currg->rg_start + offset > currg->rg_end)
  {
    printf ("Error in: mm-vm.c/ __read() :");
    printf (" Access out-of-range region.\n");
    return -1;
  }

  pg_getval(caller->mm, currg->rg_start + offset, data, caller);

  return 0;
}

/*pgwrite - PAGING-based read a region memory */
int pgread(
    struct pcb_t *proc, // Process executing the instruction
    uint32_t source,    // Index of source register
    uint32_t offset,    // Source address = [source] + [offset]
    uint32_t destination)
{
  BYTE data;
  int val = __read(proc, 0, source, offset, &data);
  if(val != 0) {
    printf("Error in: mm-vm.c/ pgread().\n");
    return -1;
  }

  destination = (uint32_t)data;
#ifdef IODUMP
  printf("read region = %d offset = %d value = %d\n", source, offset, data);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); // print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif

  return val;
}

/*__write - write a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@offset: offset to acess in memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *
 */
int __write(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE value)
{
  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);

  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  if (currg == NULL) /* Invalid memory identify */
  {
    printf ("Error in: mm-vm.c/ __read() :");
    printf (" region %d does not exist.\n", rgid, vmaid);
    return -1;
  }

  if (currg->rg_start + offset < currg->rg_start || currg->rg_start + offset > currg->rg_end)
  {
    printf ("Error in: mm-vm.c/ __write() :");
    printf (" Access out-of-range region.\n");
    return -1;
  }

  pg_setval(caller->mm, currg->rg_start + offset, value, caller);

  return 0;
}

/*pgwrite - PAGING-based write a region memory */
int pgwrite(
    struct pcb_t *proc,   // Process executing the instruction
    BYTE data,            // Data to be wrttien into memory
    uint32_t destination, // Index of destination register
    uint32_t offset)
{
//trước khi write
#ifdef IODUMP
  printf("write region = %d offset = %d value = %d\n", destination, offset, data);
  printf("Before write:\n");
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); // print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif
///

  int res = __write(proc, 0, destination, offset, data);
  if(res != 0) {
    printf("Error in: mm-vm.c/ pgwrite().\n");
    return -1;
  }
//sau khi write
#ifdef IODUMP
  printf("After khi write:\n");
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); // print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif
///

  return res;
}

/*free_pcb_memphy - collect all memphy of pcb
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 */
int free_pcb_memph(struct pcb_t *caller)
{
  int pagenum, fpn;
  uint32_t pte;

  for (pagenum = 0; pagenum < PAGING_MAX_PGN; pagenum++)
  {
    pte = caller->mm->pgd[pagenum];

    if (!PAGING_PAGE_PRESENT(pte))
    {
      fpn = pte & 0xFFF; //PAGING_FPN(pte);
      MEMPHY_put_freefp(caller->mram, fpn);
    }
    else
    {
      fpn = PAGING_SWP(pte);
      MEMPHY_put_freefp(caller->active_mswp, fpn);
    }
  }

  return 0;
}

/*get_vm_area_node - get vm area for a number of pages
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
// lấy break + size và trả về region từ break đến break+size => có khả năng overlap
struct vm_rg_struct *get_vm_area_node_at_brk(struct pcb_t *caller, int vmaid, int size, int alignedsz)
{
  struct vm_rg_struct *newrg;
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  newrg = malloc(sizeof(struct vm_rg_struct));

  newrg->rg_start = cur_vma->sbrk;
  newrg->rg_end = newrg->rg_start + size; // lỡ + size vào lớn hơn end thì sao?

  return newrg;
}

/*validate_overlap_vm_area
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
// lỡ vmastart và vmaend nó trùng qua
int validate_overlap_vm_area(struct pcb_t *caller, int vmaid, int vmastart, int vmaend)
{
  return 0;
  // struct vm_area_struct *vma = caller->mm->mmap;

  // /* TODO validate the planned memory area is not overlapped */
  // while (vma)
  // {
  //   // thực ra chỉ cần check vmaend
  //   if ((vmastart >= vma->vm_start && vmastart <= vma->vm_end) || (vmaend >= vma->vm_start && vmaend <= vma->vm_end))
  //     return -1;
  //   vma = vma->vm_next;
  // }
  // return 0;
}

/*inc_vma_limit - increase vm area limits to reserve space for new variable
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@inc_sz: increment size
 *
 */
// tăng vm_end ~ lấy thêm frame
int inc_vma_limit(struct pcb_t *caller, int vmaid, int inc_sz)
{
  struct vm_rg_struct *newrg = malloc(sizeof(struct vm_rg_struct));
  int inc_amt = PAGING_PAGE_ALIGNSZ(inc_sz);
  int incnumpage = inc_amt / PAGING_PAGESZ;
  struct vm_rg_struct *area = get_vm_area_node_at_brk(caller, vmaid, inc_sz, inc_amt);
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  int old_end = cur_vma->vm_end;

  /*Validate overlap of obtained region */
  if (validate_overlap_vm_area(caller, vmaid, area->rg_start, area->rg_end) < 0) {
    printf("Error in: mm-vm.c/ inc_vma_limit(): overlap and failed allocation.\n");
    return -1; /*Overlap and failed allocation */
  }
    

  /* The obtained vm area (only)
   * now will be alloc real ram region */
  cur_vma->vm_end += inc_sz; // thêm vào bắt đầu từ break + size, tăng limit lại tăng từ end => khoảng cách giữa end và break vẫn như cũ
  // int vm_map_ram(struct pcb_t *caller, int astart, int aend, int mapstart, int incpgnum, struct vm_rg_struct *ret_rg)
  if (vm_map_ram(caller, area->rg_start, area->rg_end, old_end, incnumpage, newrg) < 0) {
    printf ("Error in: mm-vm.c/ inc_vma_limit() :");
    printf(" vm_map_ram() is unsuccessful..\n");
    return -1; /* Map the memory to MEMRAM */
  }
  free(newrg); // cần không?
  return 0;
}

/*find_victim_page - find victim page
 *@caller: caller
 *@pgn: return page number
 *
 */
int find_victim_page(struct mm_struct *mm, int *retpgn)
{
  struct pgn_t *pg = mm->fifo_pgn; //=> FIFO algo

  /* TODO: Implement the theorical mechanism to find the victim page */
  if (pg == NULL)
    return -1;
  if (pg->pg_next == NULL)
  {
    *retpgn = pg->pgn;
    free(pg);
    mm->fifo_pgn = NULL;
    return 0;
  }

  // add head => take tail
  struct pgn_t *prev = mm->fifo_pgn;
  while (pg->pg_next != NULL)
  {
    prev = pg;
    pg = pg->pg_next;
  }
  prev->pg_next = NULL;
  *retpgn = pg->pgn;

  free(pg);

  return 0;
}

/*get_free_vmrg_area - get a free vm region
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@size: allocated size
 *
 */
// best fit
int get_free_vmrg_area(struct pcb_t *caller, int vmaid, int size, struct vm_rg_struct *newrg)
{
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  if (cur_vma == NULL) /* Invalid memory identify */
  {
    printf ("Error in: mm-vm.c/ get_free_vmrg_area() :");
    printf (" virtual memory area %d does not exist.\n", vmaid);
    return -1;
  }

  struct vm_rg_struct *rgit = cur_vma->vm_freerg_list;

  if (rgit == NULL)
        return -1;

  /* Probe unintialized newrg */
  newrg->rg_start = newrg->rg_end = -1;

  /* Traverse on list of free vm region to find a fit space */
  // printf("Checked line 664\n");
  int count = 0;
  while (rgit != NULL)
  {
    //printf("count: %d\n", count);
    //printf("rgit => rg_start: %lu, rg_end: %lu.\n", rgit->rg_start, rgit->rg_end);
    if (rgit->rg_start + size <= rgit->rg_end)
    { /* Current region has enough space */
      //printf("Checked line 677\n");
      newrg->rg_start = rgit->rg_start;
      newrg->rg_end = rgit->rg_start + size;

      /* Update left space in chosen region */
      if (rgit->rg_start + size < rgit->rg_end)
      {
        rgit->rg_start = rgit->rg_start + size;
      }
      else
      { /*Use up all space, remove current node */
        /*Clone next rg node */
        struct vm_rg_struct *nextrg = rgit->rg_next;

        /*Cloning */
        if (nextrg != NULL)
        {
          rgit->rg_start = nextrg->rg_start;
          rgit->rg_end = nextrg->rg_end;

          rgit->rg_next = nextrg->rg_next;

          free(nextrg);
        }
        else
        {                                /*End of free list */
          rgit->rg_start = rgit->rg_end; // dummy, size 0 region
          rgit->rg_next = NULL;
        }
      }
    }
    else
    { 
      //printf("Checked line 710\n");

      //if(rgit == NULL) printf("Check before\n");
      rgit = rgit->rg_next; // Traverse next rg
      //if(rgit == NULL) printf("Check after\n");
    }
    //printf("Checked line 718\n");
    count++;

  }
  //printf("Checked line 704\n");
  if (newrg->rg_start == -1) // new region not found
    return -1;

  return 0;
}

// #endif
