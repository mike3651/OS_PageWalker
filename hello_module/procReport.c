#include<linux/module.h>
#include<linux/slab.h>
#include<linux/unistd.h>
#include<linux/types.h>
#include <asm/pgtable.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>

//static unsigned long PAGE_SIZE = sysconf(_SC_PAGE_SIZE);
//void getPageData();
unsigned long virt2phys(struct mm_struct *mm, unsigned long vpage);

int proc_init (void) {
  int contiguous = 0;
  int non_contiguous = 0;
  int pageCounter = 0;
  struct vm_area_struct *vma = 0;
  struct task_struct *task = current;
  unsigned long vpage;
  printk(KERN_INFO "helloModule: kernel module initialized\n");
  if (task->mm && task->mm->mmap)
  //  printk(KERN_NOTICE "assignment: parent process: %s, PID: %d", task->comm, task->pid);
for(task=current; task!=&init_task; task=task->parent){
for_each_process(task) {
   if(task->mm && task->mm->mmap && task!=NULL){
    /* this pointlessly prints the name and PID of each task */
    printk("%s[%d]\n", task->comm, task->pid);

  //printk(KERN_NOTICE "assignment: parent process: %s, PID: %d", task->comm, task->pid);
  for (vma = task->mm->mmap; vma; vma = vma->vm_next){
    unsigned long prev_page_addr;
    contiguous = 0;
    non_contiguous = 0;

    pageCounter = 0;
    //printk(KERN_NOTICE "start: %d end: %d", vma->vm_start,vma->vm_end );
    // Grab each page data from the given process
    for (vpage = vma->vm_start; vpage < vma->vm_end; vpage += PAGE_SIZE){

      // gets the physical page address
      unsigned long physical_page_addr = virt2phys(task->mm, vpage);

      // only start comparing once we have a page to compare to
      if(pageCounter != 0) {

        // verify that the pages are contiguous or non contiguous
        unsigned long page_diff = physical_page_addr - prev_page_addr;
        if(page_diff == PAGE_SIZE) {
          contiguous++;
        } else {
          non_contiguous++;
        }
      }
      prev_page_addr = physical_page_addr;
      pageCounter++;
    }
    if (task->pid > 650)
      printk(KERN_NOTICE "parent process: %s PID: %d contiguous: %d noncontiguous: %d total pages:%d",task->comm, task->pid, contiguous, non_contiguous, pageCounter);
  }

}
}}

  return 0;
}

void proc_cleanup(void) {
  printk(KERN_INFO "helloModule: performing cleanup of module\n");
}

unsigned long virt2phys(struct mm_struct *mm, unsigned long vpage) {
  pgd_t *pgd;
  p4d_t *p4d;
  pud_t *pud;
  pmd_t *pmd;
  pte_t *pte;
  unsigned long physical_page_addr;
  struct page *page;
  pgd = pgd_offset(mm, vpage);
  if (pgd_none(*pgd) || pgd_bad(*pgd))
    return 0;
  p4d = p4d_offset(pgd, vpage);
  if (p4d_none(*p4d) || p4d_bad(*p4d))
    return 0;
  pud = pud_offset(p4d, vpage);
  if (pud_none(*pud) || pud_bad(*pud))
    return 0;
  pmd = pmd_offset(pud, vpage);
  if (pmd_none(*pmd) || pmd_bad(*pmd))
    return 0;
  if (!(pte = pte_offset_map(pmd, vpage)))
    return 0;
  if (!(page = pte_page(*pte)))
    return 0;
  physical_page_addr = page_to_phys(page);
  pte_unmap(pte);
  return physical_page_addr;
}


  MODULE_LICENSE("GPL");
  module_init(proc_init);
  module_exit(proc_cleanup);
