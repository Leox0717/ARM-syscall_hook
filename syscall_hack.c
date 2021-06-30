#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/syscalls.h>
#include <linux/kallsyms.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/thread_info.h>
#include <linux/gfp.h>  
#include <linux/highmem.h>  
#include <asm/page.h>  
#include <asm/memory.h>
#include <asm/pgtable.h>

unsigned long *syscall_table;

/* sys_clone function pointer type definition */
typedef long(sys_clone_type)(unsigned long, unsigned long, int __user *,
	       int __user *, unsigned long);

asmlinkage sys_clone_type *old_syscall_clone = NULL;

asmlinkage long new_syscall_clone(unsigned long x1, unsigned long x2, int __user *x3,
			  int __user *x4, unsigned long x5){
	printk(KERN_INFO "hello，I(pid = %d) have hacked this syscall\n", current->pid);

	return old_syscall_clone(x1, x2, x3, x4, x5);
}

/*Find va->pa translation */
pte_t* change_pte(unsigned long vaddr)
{
    printk("begin mtest_find_pte\n");

	printk("vaddr: %lx\n", vaddr);

    pgd_t *pgd;
    p4d_t *p4d;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
 
    pgd = pgd_offset_k(vaddr);
    printk("pgd_val = 0x%lx\n", pgd_val(*pgd));
    printk("pgd_index = %lu\n", pgd_index(vaddr));
    if (pgd_none(*pgd)) {
        goto out;
    }

    p4d = p4d_offset(pgd, vaddr);
    printk("p4d_val = 0x%lx\n", p4d_val(*p4d));
    if (p4d_none(*p4d)) {
        goto out;
    }    
 
    pud = pud_offset(p4d, vaddr);
    // printk("pud_val = 0x%lx\n", pud_val(*pud));
    if (pud_none(*pud)) {
        goto out;
    }
 
    pmd = pmd_offset(pud, vaddr);
    printk("pmd_val = 0x%lx\n", pmd_val(*pmd));
    printk("pmd_index = %lu\n", pmd_index(vaddr));
    if (pmd_none(*pmd)) {
        goto out;
    }
 
    pte = pte_offset_map(pmd, vaddr);
    printk("pte_val = 0x%lx\n", pte_val(*pte));
    printk("pte_index = %lu\n", pte_index(vaddr));
    if (pte_none(*pte)) {
        goto out;
    }

	printk("PTE writeable?:%d", pte_write(*pte));

	pte_val(*pte) = pte_val(*pte) - PTE_RDONLY;
    pte_val(*pte) = pte_val(*pte) | PTE_WRITE;

	return pte;

    out:
    {
        printk("Translation not found!\n");
        return NULL;
    }
}


static int __init hack_init(void){
	syscall_table = (unsigned long*) kallsyms_lookup_name("sys_call_table");

	printk("syscall_table address:%px\n", syscall_table);

	old_syscall_clone = (sys_clone_type*)syscall_table[__NR_clone];

	// pte_t *pte = change_pte((unsigned long)(syscall_table + __NR_clone));
	pte_t *pte = change_pte((unsigned long)syscall_table+__NR_clone);

    if(!pte){
		printk("Wrong in change_pte!\n");
		return 0;
	}
	printk("pte_val = 0x%lx\n", pte_val(*pte));

	printk("PTE writeable?:%d", pte_write(*pte));

	syscall_table[__NR_clone] = (unsigned long)new_syscall_clone;

	return 0;
}

static void __exit hack_exit(void){
	syscall_table[__NR_clone] =  (unsigned long)old_syscall_clone;
	printk(KERN_INFO "Hacking module exit!\n");
}

module_init(hack_init);
module_exit(hack_exit);

MODULE_AUTHOR("Leox");
MODULE_LICENSE("GPL"); 