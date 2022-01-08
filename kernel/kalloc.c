// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.
extern struct spinlock cowlock;

//lab6
// uint16 refcntarr[(PHYSTOP - KERNBASE + 1) / PGSIZE];
// real number of free page is 32714.
extern int refcntarr[];
uint64 rela_idx(uint64 pa){
  return ((uint64)pa - (uint64)KERNBASE) / PGSIZE;
}

struct run {
  struct run *next;
};

extern struct kernelmem kmem;

// struct {
//   struct spinlock lock;
//   struct run *freelist;
// } kmem;

void
kinit()
{
  // memset(refcntarr, 0, sizeof(refcntarr));
  // printf("sizeof refcntarr %d\n", sizeof(refcntarr));
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    kfree(p);
  }
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)

void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  //lab6
  // if(refcntarr[rela_idx((uint64)pa)] >= 1){
  //   printf("warning: kfree: free a refered page. cnt: %d\n", refcntarr[rela_idx((uint64)pa)]);
  //   refcntarr[rela_idx((uint64)pa)] = 0;
  //   // release(&kmem.lock);
  //   // return;
  // }

  r = (struct run*)pa;

  acquire(&kmem.lock);

  r->next = kmem.freelist;
  kmem.freelist = r;

  release(&kmem.lock);
}

// void
// kfree(void *pa)
// {
//   struct run *r;

//   if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
//     panic("kfree");

//   // acquire(&kmem.lock);
//   //lab6
//   if(refcntarr[rela_idx((uint64)pa)] >= 1){
//     printf("warning: kfree: free a refered page. cnt: %d\n", refcntarr[rela_idx((uint64)pa)]);
//     refcntarr[rela_idx((uint64)pa)] = 0;
//     // release(&kmem.lock);
//     // return;
//   }

//   // Fill with junk to catch dangling refs.
//   memset(pa, 1, PGSIZE);

//   r = (struct run*)pa;

//   acquire(&kmem.lock);
//   r->next = kmem.freelist;
//   kmem.freelist = r;
//   release(&kmem.lock);
// }



void
cow_kfree(void *pa)
{
  struct run *r;
  

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("cow_kfree");
  
  acquire(&kmem.lock);
  if(refcntarr[rela_idx((uint64)pa)] >= 1){
    refcntarr[rela_idx((uint64)pa)] -= 1;
  }
  if(refcntarr[rela_idx((uint64)pa)] >= 1){
    release(&kmem.lock);
    return;
  }
  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;
  

  if(refcntarr[rela_idx((uint64)pa)] == 0){
    r->next = kmem.freelist;
    kmem.freelist = r;
  }

  release(&kmem.lock);
}

// void
// cow_kfree(void *pa)
// {
//   struct run *r;
  

//   if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
//     panic("cow_kfree");
  
//   acquire(&cowlock);
//   if(refcntarr[rela_idx((uint64)pa)] >= 1){
//     refcntarr[rela_idx((uint64)pa)] -= 1;
//   }
//   if(refcntarr[rela_idx((uint64)pa)] >= 1){
//     release(&cowlock);
//     return;
//   }
//   release(&cowlock);
//   // Fill with junk to catch dangling refs.
//   memset(pa, 1, PGSIZE);

//   r = (struct run*)pa;
//   acquire(&kmem.lock);

//   if(refcntarr[rela_idx((uint64)pa)] == 0){
//     r->next = kmem.freelist;
//     kmem.freelist = r;
//   }

//   release(&kmem.lock);
// }

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}



void *
cow_kalloc(void){
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;

  if(r)
    refcntarr[rela_idx((uint64)r)] = 1;

  release(&kmem.lock);


  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}


// void *
// cow_kalloc(void){
//   struct run *r;

//   acquire(&kmem.lock);
//   r = kmem.freelist;
//   if(r)
//     kmem.freelist = r->next;
  
//   release(&kmem.lock);


//   //lab6
//   acquire(&cowlock);
//   if(r)
//     refcntarr[rela_idx((uint64)r)] = 1;
//   release(&cowlock);

//   if(r)
//     memset((char*)r, 5, PGSIZE); // fill with junk
//   return (void*)r;
// }


uint64 freemem(){
  uint64 num = 0;
  struct run *r;
  r = kmem.freelist;
  while(r){
    num ++;
    r = r->next;
  }
  // return num * 4096;
  return num;
}