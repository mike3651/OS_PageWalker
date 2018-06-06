
#include <asm/pgtable.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/unistd.h>
#include <stdio.h>

struct Procdata {
  char *pid;
  char *name;
  int contig;
  int noncontig;
} procdata;

/*********************
Signature Declarations
**********************/
unsigned long virt2phys(struct mm_struct *mm, unsigned long vpage);


/********************************************
Performed on kernel module insertion (insmod)
*********************************************/
int proc_init (void) {
  unsigned long vpage;
  struct vm_area_struct *vma = 0;
  struct task_struct *task = current;
  int pid_n = 0;


  printk(KERN_INFO "procReport: kernel module test initialized\n");

  // running through each task
  for_each_process(task) {
    if (task->mm && task->mm->mmap && task != NULL) {
      for (vma = task->mm->mmap; vma; vma = vma->vm_next){

        int contiguous = 0;
        int non_contiguous = 0;
        unsigned long prev_page_addr;
        int pageCounter = 0;

        // print the task name and pid to logs
        if (pid_n != task->pid) {
          printk(KERN_INFO "procReport: current process: %s, PID: %d", task->comm, task->pid);
          pid_n = task->pid;
        }

        // Grab each page data from the given process
        for (vpage = vma->vm_start; vpage < vma->vm_end; vpage += PAGE_SIZE){

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
      }
    }
  }
  return 0;
}


/*****************************************
Performed on kernel module removal (rmmod)
******************************************/
void proc_cleanup(void) {
  printk(KERN_INFO "procReport: performing cleanup of module\n");
}


/*************************************************************
Returns a physical address given a memory map and virtual page
**************************************************************/
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


/*
Writes a comma separated row of process id, process name, 
contiguous memory pages, non-contiguous memory pages, 
and total pages, given a pointer to an open FILE and 
a procdata struct. Returns 0 if successful.
*/
int write_procdata(FILE *fp, struct Procdata *procdata) {
  if (fp == NULL) {
    return 1;
  }

  fprintf(fp, "%d,%s,%d,%d,%d\n",
    procdata->pid, procdata->name, 
    procdata->contig, procdata->noncontig,
    procdata->contig + procdata->noncontig);

  return 0;
}

int write_header(FILE *fp) {
  if (fp == NULL) {
    return 1;
  }
  fprintf(fp, "PROCESS REPORT:\nproc_id,proc_name,contig_pages,noncontig_pages,total_pages\n");
  return 0;
}

int write_footer(FILE *fp, struct Procdata *procdata) {
  if (fp == NULL) {
    return 1;
  }
  fprintf(fp, "TOTALS,,%d,%d,%d", 
    procdata->contig, procdata->noncontig,
    procdata->contig + procdata->noncontig);
  return 0;
}


MODULE_LICENSE("GPL");
module_init(proc_init);
module_exit(proc_cleanup);
