//pte printout
//print ptes by recurrently travsing the pte level tree.
/*
void levelprint(pte_t pte, int level, int index){
  for(int j = 0; j < level; j++){
    printf(".. ");
  }
  printf("..");
  printf("%d: pte %p pa %p\n", index, pte, PTE2PA(pte));
}

void printpte(pagetable_t pagetable, int level){
  for(int i = 0; i < 512; i++){
    pte_t pte = pagetable[i];
    if((pte & PTE_V) && (pte & (PTE_R|PTE_W|PTE_X)) == 0){
      uint64 child = PTE2PA(pte);
      levelprint(pte, level, i);
      printpte((pagetable_t)child, level + 1);
    }else if(pte & PTE_V){
      levelprint(pte, level, i);
    }
  }
}

void vmprint(pagetable_t pagetable){
  printf("page table %p\n", pagetable);
  printpte(pagetable, 0);
}

== Test pte printout == pte printout: OK (1.5s) 
*/
