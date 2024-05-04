/*
 * Copyright (C) 2024 pdnguyen of the HCMC University of Technology
 */
/*
 * Source Code License Grant: Authors hereby grants to Licensee 
 * a personal to use and modify the Licensed Source Code for 
 * the sole purpose of studying during attending the course CO2018.
 */
//#ifdef MM_TLB
/*
 * Memory physical based TLB Cache
 * TLB cache module tlb/tlbcache.c
 *
 * TLB cache is physically memory phy
 * supports random access 
 * and runs at high speed
 */


#include "mm.h"
#include <stdlib.h>
#include <stdio.h>

#define init_tlbcache(mp,sz,...) init_memphy(mp, sz, (1, ##__VA_ARGS__))

/*
 *  tlb_cache_read read TLB cache device
 *  @mp: memphy struct
 *  @pid: process id
 *  @pgnum: page number
 *  @value: obtained value
 */

int tlb_cache_read(struct memphy_struct * mp, int pid, int pgnum, int* value)
{
   /* TODO: the identify info is mapped to 
    *      cache line by employing:
    *      direct mapped, associated mapping etc.
    */
    if (mp == NULL || pgnum < 0 || pgnum >= mp->maxsz)
        return -1; // Tham số không hợp lệ hoặc mp không tồn tại

    int addr = pgnum * PAGE_SIZE; // Tính địa chỉ bắt đầu của trang

    // Kiểm tra xem trang đã được cache chưa
    if (mp->tlb_cache_pid == pid && mp->tlb_cache_pgnum == pgnum) {
        value = &mp->tlb_cache_value; // Trả về giá trị từ TLB cache
        return 0; // Đọc thành công từ TLB cache
    }

    // Nếu trang chưa được cache, đọc từ bộ nhớ vật lý
    TLBMEMPHY_read(mp, addr, value);

    // Cập nhật TLB cache
    mp->tlb_cache_pid = pid;
    mp->tlb_cache_pgnum = pgnum;
    mp->tlb_cache_value = *value;

    return 0; // Đọc thành công từ bộ nhớ vật lý
}

/*
 *  tlb_cache_write write TLB cache device
 *  @mp: memphy struct
 *  @pid: process id
 *  @pgnum: page number
 *  @value: obtained value
 */
int tlb_cache_write(struct memphy_struct *mp, int pid, int pgnum, int value)
{
   /* TODO: the identify info is mapped to 
    *      cache line by employing:
    *      direct mapped, associated mapping etc.
    */
     if (mp == NULL || pgnum < 0 || pgnum >= mp->maxsz)
        return -1; // Tham số không hợp lệ hoặc mp không tồn tại

    int addr = pgnum * PAGE_SIZE; // Tính địa chỉ bắt đầu của trang

    // Ghi giá trị vào bộ nhớ vật lý
    TLBMEMPHY_write(mp, addr, value);

    // Cập nhật TLB cache
    mp->tlb_cache_pid = pid;
    mp->tlb_cache_pgnum = pgnum;
    mp->tlb_cache_value = value;

    return 0; // Ghi thành công vào bộ nhớ vật lý
}

/*
 *  TLBMEMPHY_read natively supports MEMPHY device interfaces
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
int TLBMEMPHY_read(struct memphy_struct * mp, int addr, int *value)
{
   if (mp == NULL)
     return -1;

   /* TLB cached is random access by native */
   *value = mp->storage[addr];

   return 0;
}


/*
 *  TLBMEMPHY_write natively supports MEMPHY device interfaces
 *  @mp: memphy struct
 *  @addr: address
 *  @data: written data
 */
int TLBMEMPHY_write(struct memphy_struct * mp, int addr, int data)
{
   if (mp == NULL)
     return -1;

   /* TLB cached is random access by native */
   mp->storage[addr] = data;

   return 0;
}

/*
 *  TLBMEMPHY_format natively supports MEMPHY device interfaces
 *  @mp: memphy struct
 */


int TLBMEMPHY_dump(struct memphy_struct * mp)
{
   /*TODO dump memphy contnt mp->storage 
    *     for tracing the memory content
    */
    if (mp == NULL) {
        printf("Error: Invalid memory physical structure\n");
        return -1;
    }

    printf("memory physical content:\n");
    for (int i = 0; i < mp->maxsz; i++) {
        printf("%d: %d\n", i, mp->storage[i]);
    }

    return 0;
}


/*
 *  Init TLBMEMPHY struct
 */
int init_tlbmemphy(struct memphy_struct *mp, int max_size)
{
   mp->storage = (BYTE *)malloc(max_size*sizeof(BYTE));
   mp->maxsz = max_size;

   mp->rdmflg = 1;

   return 0;
}

//#endif
