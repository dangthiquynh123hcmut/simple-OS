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

/* frmnum is return value of tlb_cache_read/write value*/



int tlb_cache_read(struct memphy_struct * mp, int pid, int pgnum, BYTE *value)
{
   /* TODO: the identify info is mapped to 
    *      cache line by employing:
    *      direct mapped, associated mapping etc.
    */
    if (mp == NULL) 
        return -1; // Tham số không hợp lệ hoặc mp không tồn tại

    // uint32_t pte = proc->mm->pgd[pgnum];
    // int fpn = PAGING_FPN(pte);

    //printf("tlb_cache_read:: valid = %d, pid = %d, pgn = %d, fpn = %d\n", mp->help[pgnum].valid, mp->help[pgnum].pid, pgnum, mp->storage[pgnum]);
    for(int i = 0; i<mp->maxsz; i++) {
        if( mp->help[i].valid == 1 && mp->help[i].pid == pid && mp->help[i].pgnum == pgnum) 
            return TLBMEMPHY_read(mp, i, value);  // HIT
    }
    
    return -1;  
}

// /*
//  *  tlb_cache_write write TLB cache device
//  *  @mp: memphy struct
//  *  @pid: process id
//  *  @pgnum: page number
//  *  @value: obtained value
//  */
// // Nếu còn trống thì ghi vào, không thì swap
int tlb_cache_write(struct memphy_struct *mp, int pid, int pgnum, BYTE value)
{
   /* TODO: the identify info is mapped to 
    *      cache line by employing:
    *      direct mapped, associated mapping etc.
    */
    if (mp == NULL)
        return -1; // Tham số không hợp lệ hoặc mp không tồn tại

    int flag = 0;
    for(int i = 0; i<mp->maxsz; i++) {
        if( mp->help[i].valid == 0 ) {    // có entry trống 
            mp->help[i].valid = 1;
            mp->help[i].pid = pid;
            mp->help[i].pgnum = pgnum;
            
            if( mp->tlb_fifo == NULL ) {
                mp->tlb_fifo = malloc(sizeof (struct node) );
                mp->tlb_fifo->data = i;
                mp->tlb_fifo->next = NULL;
            } else {
                struct node* newNode = mp->tlb_fifo; 
                if(newNode == NULL) {
                    printf("Error in cpu-tlbcache.c/ tlb_cache_write(): mp->tlb_fifo is NULL.\n");
                    return -1;
                }
                while(newNode->next != NULL) newNode = newNode->next;
                newNode->next = malloc(sizeof (struct node) );
                newNode->next->data = i;
                newNode->next->next = NULL;
            }

            TLBMEMPHY_write(mp, i, value);

            break;
        }
    }

    struct node* victim = mp->tlb_fifo;

    if( victim == NULL ) {
        printf("Error in tlbwrite(): tlb_fifo is NULL.\n");
        return -1;
    }

    mp->tlb_fifo = mp->tlb_fifo->next;
    mp->help[victim->data].pid = pid;
    mp->help[victim->data].pgnum = pgnum;
    TLBMEMPHY_write(mp, victim->data, value);
    free(victim);

    //printf("tlb_cache_write: valid = %d, pid = %d, pgn = %d, fpn = %d\n", 
    //    mp->help[pgnum].valid, mp->help[pgnum].pid, pgnum, mp->storage[pgnum]);
    return 0;
    //return frame number;
}


// int tlb_cache_read(struct memphy_struct * mp, int pid, int pgnum, BYTE value, int index)
// {
//     if (mp == NULL) 
//         return -1; // Tham số không hợp lệ hoặc mp không tồn tại

//     printf("max size = %d, index = %d\n", mp->maxsz, index);

//     //printf("tlb_cache_read:: valid = %d, pid = %d, pgn = %d, fpn = %d\n", mp->help[pgnum].valid, mp->help[pgnum].pid, pgnum, mp->storage[pgnum]);
//     if( mp->help[index].valid == 1 && mp->help[index].pid == pid && mp->help[index].pgnum == pgnum) {
//         printf("check 126\n");
//         return TLBMEMPHY_read(mp, index, value);  // HIT
//         printf("check 128\n");
//     }
    
//     return -1;  
// }


// int tlb_cache_write(struct memphy_struct *mp, int pid, int pgnum, BYTE value, int index)
// {
//     if (mp == NULL)
//         return -1; // Tham số không hợp lệ hoặc mp không tồn tại

//         if( mp->help[index].valid == 0 ) {   // có entry trống 
//             mp->help[index].valid = 1;
//             mp->help[index].pid = pid;
//             mp->help[index].pgnum = pgnum;
//         } else {
//             if( mp->tlb_fifo == NULL ) {
//                 mp->tlb_fifo = malloc(sizeof (struct node) );
//                 mp->tlb_fifo->data = index;
//                 mp->tlb_fifo->next = NULL;
//             } else {
//                 struct node* newNode = mp->tlb_fifo; 
//                 while(newNode->next != NULL) newNode = newNode->next;
//                 newNode->next = malloc(sizeof (struct node) );
//                 newNode->next->data = index;
//                 newNode->next->next = NULL;
//             }

//             TLBMEMPHY_write(mp, index, value);
//         }
//     struct node* victim = mp->tlb_fifo;

//     if( victim == NULL ) {
//         printf("Error in tlbwrite(): tlb_fifo is NULL.\n");
//         return -1;
//     }

//     mp->tlb_fifo = mp->tlb_fifo->next;
//     mp->help[victim->data].pid = pid;
//     mp->help[victim->data].pgnum = pgnum;
//     TLBMEMPHY_write(mp, victim->data, value);
//     free(victim);
//    return 0;
// }


/*
 *  TLBMEMPHY_read natively supports MEMPHY device interfaces
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
// chưa check trường hợp storage[addr] rỗng
// => đảm bảo đọc khi đã có
int TLBMEMPHY_read(struct memphy_struct * mp, int addr, BYTE *value)
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
int TLBMEMPHY_write(struct memphy_struct * mp, int addr, BYTE data)
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
   if (mp == NULL) {
       printf("Error: Invalid memory physical structure\n");
       return -1;
   }

   printf("Memory physical content:\n");
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
   mp->help = (struct tlbEntry *)malloc(max_size*sizeof(struct tlbEntry));
   // invalid = 0, valid = 1 
   for (int i = 0; i < max_size; i++) {
    mp->help[i].valid = 0;
   }

   mp->storage = (BYTE *)malloc(max_size*sizeof(BYTE));
   mp->maxsz = max_size;
   mp->tlb_fifo = NULL;

   mp->rdmflg = 1;

   return 0;
}

//#endif
