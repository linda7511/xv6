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
#define NBUCKETS 29
#define BUCKETSZ 4
struct bucket{
  struct spinlock lock;
  struct buf bufs[BUCKETSZ];//每个bucket存储一组buf结点
};
struct bucket bhash[NBUCKETS];//声明一个哈希表
/*
struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;
} bcache;*/

int
hashkey(uint key)
{
  return key % NBUCKETS;
}

void
binit(void)
{
  //初始化锁的名称
  uint init_stamp = ticks;
  for(int i = 0; i < NBUCKETS; i++){
    initlock(&bhash[i].lock,"bcache");
    for(int j = 0; j < BUCKETSZ; j++){//初始化时间戳和所属桶号
      bhash[i].bufs[j].timestamp = init_stamp;
      bhash[i].bufs[j].bucket = i;
    }
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int key = hashkey(blockno);

  acquire(&bhash[key].lock);//获取blockno所在bucket的锁

  // Is the block already cached?
  for(b = &bhash[key].bufs[0]; b < &bhash[key].bufs[0]+BUCKETSZ; b++){
    if(b->dev == dev && b->blockno == blockno){//找到该节点
      b->refcnt++;//增加引用数
      b->timestamp = ticks;//更新时间戳
      release(&bhash[key].lock);
      acquiresleep(&b->lock);//调用睡眠锁准备读写
      return b;
    }
  }
  uint minstamp = ~0;
  struct buf* minbuf=0;

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(b = &bhash[key].bufs[0]; b < &bhash[key].bufs[0]+BUCKETSZ; b++){
    if(b->refcnt == 0&&b->timestamp < minstamp) {
      minstamp = b->timestamp;
      minbuf = b;
    }
  }
  if(minbuf){
    minbuf->dev = dev;
    minbuf->blockno = blockno;
    minbuf->valid = 0;
    minbuf->refcnt = 1;
    minbuf->timestamp =ticks;//更新时间戳
    release(&bhash[key].lock);
    acquiresleep(&minbuf->lock);//调用睡眠锁准备读写
    return minbuf;
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

  releasesleep(&b->lock);//释放在bget()调用的睡眠锁

  acquire(&bhash[b->bucket].lock);//获取b所在bucket的自旋锁
  b->refcnt--;
  
  release(&bhash[b->bucket].lock);
}

void
bpin(struct buf *b) {
  acquire(&bhash[b->bucket].lock);
  b->refcnt++;
  release(&bhash[b->bucket].lock);
}

void
bunpin(struct buf *b) {
  acquire(&bhash[b->bucket].lock);
  b->refcnt--;
  release(&bhash[b->bucket].lock);
}


