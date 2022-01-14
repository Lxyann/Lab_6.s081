// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

struct {
  //lab8
  struct spinlock hashlock[NBUCKET];
  struct spinlock lock;
  
  struct buf buf[NBUF];

  

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  // struct buf head;
  struct buf head[NBUCKET];
  
} bcache;


void
binit(void)
{
  struct buf *b;

  for(int i = 0; i < NBUCKET; i++){
    initlock(&bcache.hashlock[i], "hash bcache");
  }

  initlock(&bcache.lock, "bcache");

  for(b = bcache.buf; b < bcache.buf + NBUF; b++){
    initsleeplock(&b->lock, "buffer");
    b->ticks = ticks;
  }

  for(int i = 0; i < NBUCKET; i++){
    bcache.head[i].prev = &bcache.head[i];
    bcache.head[i].next = &bcache.head[i];
  }

}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int i = blockno % NBUCKET;
  acquire(&bcache.hashlock[i]);

  // Is the block already cached?
  for(b = bcache.head[i].next; b != &bcache.head[i]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      printf("cache!\n");
      b->refcnt++;
      release(&bcache.hashlock[i]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  acquire(&bcache.lock);
  int idx = -1;
  uint nticks = ticks;
  // printf("ticks = %d\n", ticks);
  for(int i = 0; i < NBUF; i++){
    if(bcache.buf[i].refcnt == 0 && bcache.buf[i].ticks <= nticks){
      idx = i;
      nticks = bcache.buf[i].ticks;
    }
  }
  

  //move buf to the target hash table.
  //the buf has been removed from other hash table.
  
  if(idx != -1){ // find out a free buf.
    b = &bcache.buf[idx];

    b->dev = dev;
    b->blockno = blockno;
    b->valid = 0;
    b->refcnt = 1;
    // move b to head.
    b->next = bcache.head[i].next;
    b->prev = &bcache.head[i];
    bcache.head[i].next->prev = b;
    bcache.head[i].next = b;

    release(&bcache.lock);
    release(&bcache.hashlock[i]);
    acquiresleep(&b->lock);

    return b;
  }

  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");
  
  int i = b->blockno % NBUCKET;

  releasesleep(&b->lock);

  acquire(&bcache.hashlock[i]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
  }
  b->ticks = ticks;
  release(&bcache.hashlock[i]);
}

void
bpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt++;
  release(&bcache.lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
}


