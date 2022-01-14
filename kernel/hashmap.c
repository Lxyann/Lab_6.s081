
// return buf pointer which has blockno.
//        0 , otherwise.
// static struct buf *
// get(uint blockno)
// {
//   int i = blockno % NBUCKET;
//   struct run *r = 0;
//   struct buf *b = 0;
//   for(r = bcache.linklist[i]; r != 0; r = r->next){
//     b = (struct buf *)r;
//     if(b->blockno == blockno)
//       break;
//   }
//   return b;
// }

// static void
// put(uint blockno)
// {
//   int i = blockno % NBUCKET;
//   struct buf * targetbuf = 0;
//   for(int j = 0; j < NBUF ; j++){
//     if(testbuf[j].blockno == blockno && testbuf[j].refcnt >= 1){
//       targetbuf = &testbuf[j];
//       break;
//     }
//   }

//   struct run *r = 0;
//   if(targetbuf){
//     r = (struct run *)targetbuf;
//     r->next = linklist[i];
//     linklist[i] = r;
//   }else{
//     printf("There is not buf # %d in BUFS\n", blockno);
//     panic("hashtable: put");
//   }
// }

// static void
// del(uint blockno)
// {
//   int i = blockno % NBUCKET;

//   struct run *r = bcache.linklist[i];
//   struct run *pre_r = 0;
//   struct buf *b = (struct buf *)r;
//   if(b->blockno == blockno){
//     bcache.linklist[i] = r->next;
//   }else{
//     pre_r = bcache.linklist[i];
//     r = pre_r->next;
//     for(; r != 0; pre_r = pre_r->next, r = r->next){
//       b = (struct buf *)r;
//       if(b->blockno == blockno){
//         pre_r->next = pre_r->next->next;
//         break;
//       }
//     }
//   }
// }

// static void
// print_hash()
// {
//   for(int i = 0; i < NBUCKET; i++){
//     printf("backet: %d\n", i);

//     struct run *r = bcache.linklist[i];
//     for(; r != 0; r = r->next){
//       struct buf *b = (struct buf *)r;
//       printf("(block: %d)\n", b->blockno);
//     }
//   }
// }

// static void
// testhash()
// {
//   for(int i = 0; i < NBUCKET; i++){
//     bcache.linklist[i]->next = 0;
//   }

//   for(int i = 0; i < NBUF; i++){
//     bcache.testbuf[i].blockno = i;
//     put(i);
//   }

//   print_hash();

//   for(int i = 0; i < NBUF; i++){
//     printf("hash get: %d \n", get(i));
//   }

//   for(int i = 0; i < NBUF; i++){
//     del(i);
//   }

//   print_hash();
// }
