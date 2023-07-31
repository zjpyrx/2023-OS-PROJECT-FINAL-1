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

extern uint ticks; //来自trap.c，使用上次使用的时间来代替时间戳缓冲区

//哈希函数，用于将给定的块号（blockno）映射到哈希桶的索引
int hash(int blockno)
{
  return blockno % NBUCKET;
}


struct {
  struct spinlock biglock; //lab8修改
  struct spinlock lock[NBUCKET]; //lab8新增
  struct buf buf[NBUF];
  

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head[NBUCKET]; //lab8修改
} bcache;

void
binit(void)
{
  struct buf* b;
 
  initlock(&bcache.biglock, "bcache_biglock"); //初始化大锁，保护整个缓存

  for (int i = 0; i < NBUCKET; i++)   //初始化每个桶的锁
    initlock(&bcache.lock[i], "bcache");

  // Create linked list of buffers
  for (int i = 0; i < NBUCKET; i++) {  //创建缓存块的双向链表
    bcache.head[i].next = &bcache.head[i]; //初始化每个桶的链表头，头的前、后指针都指向头本身
    bcache.head[i].prev = &bcache.head[i];
  }
 
  for (b = bcache.buf; b < bcache.buf + NBUF; b++) { //把所有缓存块都加入到链表中
    b->next = bcache.head[0].next;
    b->prev = &bcache.head[0];

    initsleeplock(&b->lock, "buffer");

    bcache.head[0].next->prev = b;
    bcache.head[0].next = b;
  }
}


// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b, *b2 = 0;

  int i = hash(blockno), min_ticks = 0;
  acquire(&bcache.lock[i]);

  // Is the block already cached?
  //1.判断是否命中，如果命中就直接返回
  for(b = bcache.head[i].next; b != &bcache.head[i]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[i]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.lock[i]);

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  //2.不命中，释放锁
  acquire(&bcache.biglock);
  acquire(&bcache.lock[i]);
  //释放锁后又可能会有缓存，先遍历目前的块，看是否命中，如果命中直接返回
  for (b = bcache.head[i].next; b != &bcache.head[i]; b = b->next) {
    if (b->dev == dev && b->blockno == blockno) {
      b->refcnt++;
      release(&bcache.lock[i]);
      release(&bcache.biglock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  //3.寻找当前桶对应的LRU的空闲块，使用ticks的方式
  for (b = bcache.head[i].next; b != &bcache.head[i]; b = b->next) {
    if (b->refcnt == 0 && (b2 == 0 || b->lastuse < min_ticks)) {
      min_ticks = b->lastuse;
      b2 = b;
    }
  }
  if (b2) {
    b2->dev = dev;
    b2->blockno = blockno;
    b2->refcnt++;
    b2->valid = 0;
    //acquiresleep(&b2->lock);
    release(&bcache.lock[i]);
    release(&bcache.biglock);
    acquiresleep(&b2->lock);
    return b2;
  }
  //4.当前桶中没有，向其他桶中寻找块
  for (int j = hash(i + 1); j != i; j = hash(j + 1)) {
    acquire(&bcache.lock[j]);
    for (b = bcache.head[j].next; b != &bcache.head[j]; b = b->next) {
      if (b->refcnt == 0 && (b2 == 0 || b->lastuse < min_ticks)) {
        min_ticks = b->lastuse;
        b2 = b;
      }
    }
    if (b2) {
      b2->dev = dev;
      b2->refcnt++;
      b2->valid = 0;
      b2->blockno = blockno;
      //将块从其原来的桶中移出
      b2->next->prev = b2->prev;
      b2->prev->next = b2->next;
      release(&bcache.lock[j]);

      b2->next = bcache.head[i].next;
      b2->prev = &bcache.head[i];
      bcache.head[i].next->prev = b2;
      bcache.head[i].next = b2;
      release(&bcache.lock[i]);
      release(&bcache.biglock);
      acquiresleep(&b2->lock);
      return b2;
    }
    release(&bcache.lock[j]);
  }
  release(&bcache.lock[i]);
  release(&bcache.biglock);
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

  releasesleep(&b->lock);

  int i = hash(b->blockno);

  acquire(&bcache.lock[i]); //加锁
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    //b->next->prev = b->prev;
    //b->prev->next = b->next;
    //b->next = bcache.head.next;
    //b->prev = &bcache.head;
    //bcache.head.next->prev = b;
    //bcache.head.next = b;
    b->lastuse = ticks; //遇到空闲块的时候，改为直接设置它的使用时间
  }
  
  release(&bcache.lock[i]);
}

void
bpin(struct buf *b) {
  int i = hash(b->blockno); //映射到哈希表
  acquire(&bcache.lock[i]); //更改加锁
  b->refcnt++;
  release(&bcache.lock[i]);
}

void
bunpin(struct buf *b) {
  int i = hash(b->blockno); //映射到哈希表
  acquire(&bcache.lock[i]); //更改加锁
  b->refcnt--;
  release(&bcache.lock[i]);
}


