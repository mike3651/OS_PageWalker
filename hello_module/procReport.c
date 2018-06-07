/*
HW 3

This code writes to log files accessable by dmesg in the correct format all pid and their info
above 650. 
It is able to create the procfile correctly, but is only able to "print" the header to it. We
weren't able to figure out how to make a big csv list and show it in the procfile, because
it isn't written to like a normal userspace file.
We have left in the commented out non-working code to show where we were headed with our thought process.

Alyssa Ingersoll, Samantha Shoecraft, and Mike Wilson
*/

#include <linux/sched.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/sched/signal.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/pgtable.h>
#include <linux/kernel.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/unistd.h>

#define BUFSIZE 100

// A struct to hold each process's data to print later.
struct Procdata {
  int pid;
  char *name;
  int contig;
  int noncontig;
} Procdata;

// A global string to write our CSV to, to be shown in the procfile
static char *str = NULL;

/*********************
Signature Declarations
**********************/
unsigned long virt2phys(struct mm_struct *mm, unsigned long vpage);
int write_procdata(struct Procdata *procdata);
int write_header(void);
int write_footer(struct Procdata *procdata);

/************
Proc File Ops
*************/
static int proc_show(struct seq_file *m, void *v) {
  seq_printf(m, "%s\n", str);
  return 0;
}

static int open_callback(struct inode *inode, struct file *file) {
  return single_open(file, proc_show, NULL);
}

static ssize_t write_callback(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos) {
  int num, c;
  char buf[BUFSIZE];
  if (*ppos > 0 || count > BUFSIZE) {
    return -EFAULT;
  }
  if (copy_from_user(buf, ubuf, count)) {
    return -EFAULT;
  }
  num = sscanf(buf, "test");
  c = strlen(buf);
  *ppos = c;
  return c;
}

static ssize_t read_callback(struct file *file, char __user *ubuf, size_t count, loff_t *ppos) {
  char buf[BUFSIZE];
  int len = 0;
  if (*ppos > 0 || count < BUFSIZE) {
    return 0;
  }

  len += sprintf(buf, str);

  if (copy_to_user(ubuf, buf, len)) {
    return -EFAULT;
  }
  *ppos = len;
  return len;
}

static const struct file_operations proc_file_fops = {
  .owner = THIS_MODULE,
  .open = open_callback,
  // .read = seq_read,
  .read = read_callback,
  .llseek = seq_lseek,
  .release = single_release,
  .write = write_callback,
};

static struct proc_dir_entry *procentry;


/********************************************
Performed on kernel module insertion (insmod)
*********************************************/
int proc_init (void) {
  unsigned long vpage;
  struct vm_area_struct *vma = 0;
  struct task_struct *task = current;
  struct Procdata proc_totals = { .contig = 0, .noncontig = 0 };
  // struct Procdata procdata = {};
  int pid_n = 0;
  procentry = proc_create("proc_report",0644,NULL, &proc_file_fops);

  printk(KERN_INFO "proc_report: kernel module test initialized\n");
  write_header();

  int contig_sum = 0;
  int noncontig_sum = 0;

  // running through each task
  for_each_process(task) {
    if (task->mm && task->mm->mmap && task != NULL && task->pid > 650) {
        int contiguous = 0;
        int non_contiguous = 0;

      for (vma = task->mm->mmap; vma; vma = vma->vm_next){


        unsigned long prev_page_addr;
        int pageCounter = 0;

        // print the task name and pid to logs
        if (pid_n != task->pid) {
          // printk(KERN_INFO "%d,%s,", task->pid, task->comm);
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
        // procdata.name = task->comm;
        // procdata.pid = task->pid;
        // procdata.contig = contiguous;
        // procdata.noncontig = non_contiguous;
        // write_procdata(&procdata);
      }
      printk(KERN_INFO "%d,%s,%d,%d,%d\n", task->pid, task->comm, contiguous, non_contiguous, contiguous+non_contiguous);
      contig_sum += contiguous;
      noncontig_sum += non_contiguous;
    }
  }
  // For some reason, this line only prints once the module is unloaded
  printk(KERN_INFO "TOTALS,,%d,%d,%d", contig_sum, noncontig_sum, contig_sum + noncontig_sum);
  return 0;
}


/*****************************************
Performed on kernel module removal (rmmod)
******************************************/
void proc_cleanup(void) {
  printk(KERN_INFO "proc_report: performing cleanup of module\n");
  proc_remove(procentry);
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


/********************************************************
Writes a comma separated row of process id, process name, 
contiguous memory pages, non-contiguous memory pages, 
and total pages, given a pointer to an open FILE and 
a procdata struct. Returns 0 if successful.
*********************************************************/
int write_procdata(struct Procdata *procdata) {
  char *new_str;
  char proc_str[63];

  sprintf(proc_str, "%d,%s,%d,%d,%d\n",
    procdata->pid, procdata->name, 
    procdata->contig, procdata->noncontig,
    procdata->contig + procdata->noncontig);

  if ((new_str = kzalloc(strlen(str) + strlen(proc_str) + 1, GFP_KERNEL)) != NULL) {
    strcat(new_str, str);
    strcat(new_str, proc_str);
    str = new_str;
  } 

  printk(proc_str);

  return 0;
}


/********************
Writes the csv header
*********************/
int write_header(void) {
  str = ("PROCESS REPORT:\nproc_id,proc_name,contig_pages,noncontig_pages,total_pages\n0,placeholder,0,0,0\n");
  return 0;
}


/********************
Writes the csv footer
*********************/
int write_footer(struct Procdata *procdata) {
  // printk("TOTALS,,%d,%d,%d", 
  //   procdata->contig, procdata->noncontig,
  //   procdata->contig + procdata->noncontig);
  return 0;
}


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Samantha Shoecraft, Alyssa Ingersoll, Michael Wilson");
module_init(proc_init);
module_exit(proc_cleanup);
