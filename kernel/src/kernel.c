#include "memory.h"
#include "idt.h"
#include "serial.h"
#include "paging.h"
#include "keyboard.h"
#include "irq.h"
#include "scheduler.h"
#include "vfs.h"
typedef unsigned long long u64;
__attribute__((used,section(".limine_requests"),aligned(8)))
volatile u64 limine_base_revision[3]={0xf9562b2d5c95a6c8ULL,0x6a7b384944536bdcULL,3};
static ed_u64 paging_alloc_page(void *opaque){(void)opaque;void *p=pmm_alloc_page();if(!p)return EDOS_PAGING_INVALID;return (ed_u64)(unsigned long long)p-memory_hhdm_offset();}
static ed_u64 *paging_phys_to_virt(ed_u64 physical,void *opaque){(void)opaque;if(physical>~0ULL-memory_hhdm_offset())return (ed_u64*)0;return (ed_u64*)(unsigned long long)(memory_hhdm_offset()+physical);}

static void serial_char(char c);
static void serial(const char *s);
static edos_ramfs_t boot_ramfs;
static edos_vfs_t boot_vfs;
static char boot_vfs_buffer[16];
static edos_scheduler_t boot_scheduler;
static edos_cpu_context_t boot_main_context;
static ed_u64 boot_task_a, boot_task_b;
static volatile int boot_task_stage;
static unsigned char boot_task_stack_a[8192] __attribute__((aligned(16)));
static unsigned char boot_task_stack_b[8192] __attribute__((aligned(16)));
static void boot_task_returned(void){for(;;)__asm__ volatile("cli; hlt");}
static void boot_task_a_entry(void){
    boot_task_stage=1;
    if(!edos_scheduler_switch_to(&boot_scheduler,boot_task_a,boot_task_b)){serial("SCHEDULER_SWITCH_A_ERROR\n");for(;;)__asm__ volatile("cli; hlt");}
    if(boot_task_stage!=2){serial("SCHEDULER_STAGE_ERROR\n");for(;;)__asm__ volatile("cli; hlt");}
    boot_task_stage=3;
    edos_context_switch(&boot_scheduler.tasks[0].context,&boot_main_context);
    for(;;)__asm__ volatile("cli; hlt");
}
static void boot_task_b_entry(void){
    if(boot_task_stage!=1){serial("SCHEDULER_STAGE_ERROR\n");for(;;)__asm__ volatile("cli; hlt");}
    boot_task_stage=2;
    if(!edos_scheduler_switch_to(&boot_scheduler,boot_task_b,boot_task_a)){serial("SCHEDULER_SWITCH_B_ERROR\n");for(;;)__asm__ volatile("cli; hlt");}
    for(;;)__asm__ volatile("cli; hlt");
}

static void serial_char(char c){(void)edos_serial_write_char(c);}
static void serial(const char*s){(void)edos_serial_write(s);}
static void hex(u64 v){static const char d[]="0123456789abcdef";serial("0x");for(int s=60;s>=0;s-=4)serial_char(d[(v>>s)&15]);}
__attribute__((noreturn)) void kernel_main(void){
if (!edos_serial_init()) { for(;;)__asm__ volatile("cli; hlt"); }
serial("\nE.D-OS kernel 0.0.5\nSERIAL_DRIVER_READY COM1\n");
if(limine_base_revision[2]!=0){serial("BOOT_ERROR\n");for(;;)__asm__ volatile("cli; hlt");}
serial("BOOT_PROTOCOL=Limine revision 3\n");
if(!memory_init() || !memory_validate()){serial("MEMORY_ERROR\n");for(;;)__asm__ volatile("cli; hlt");}
if(!idt_init()){serial("IDT_ERROR\n");for(;;)__asm__ volatile("cli; hlt");}
int keyboard_ready=edos_keyboard_available();
if(keyboard_ready)edos_keyboard_init();
if(!edos_irq_init(keyboard_ready)){serial("IRQ_INIT_ERROR\n");for(;;)__asm__ volatile("cli; hlt");}
edos_scheduler_init(&boot_scheduler);
boot_task_a=edos_task_create(&boot_scheduler,boot_task_a_entry,boot_task_stack_a);
boot_task_b=edos_task_create(&boot_scheduler,boot_task_b_entry,boot_task_stack_b);
if(!boot_task_a||!boot_task_b||
   !edos_task_prepare_context(&boot_scheduler,boot_task_a,boot_task_stack_a,sizeof(boot_task_stack_a),boot_task_returned)||
   !edos_task_prepare_context(&boot_scheduler,boot_task_b,boot_task_stack_b,sizeof(boot_task_stack_b),boot_task_returned)||
   !edos_task_set_state(&boot_scheduler,boot_task_a,EDOS_TASK_RUNNING)){
    serial("SCHEDULER_INIT_ERROR\n");for(;;)__asm__ volatile("cli; hlt");
}
edos_context_switch(&boot_main_context,&boot_scheduler.tasks[0].context);
if(boot_task_stage!=3||boot_scheduler.current_id!=boot_task_a||
   boot_scheduler.tasks[0].state!=EDOS_TASK_RUNNING||
   boot_scheduler.tasks[1].state!=EDOS_TASK_READY||
   !edos_scheduler_validate(&boot_scheduler)){
    serial("SCHEDULER_VERIFY_ERROR\n");for(;;)__asm__ volatile("cli; hlt");
}
serial("SCHEDULER_CONTEXT_SWITCH_OK\n");
edos_ramfs_init(&boot_ramfs);
if(!edos_vfs_mount_root(&boot_vfs,&boot_ramfs)){serial("VFS_MOUNT_ERROR\n");for(;;)__asm__ volatile("cli; hlt");}
int vfs_fd=edos_vfs_open(&boot_vfs,"/boot.txt",EDOS_VFS_CREATE|EDOS_VFS_READ|EDOS_VFS_WRITE);
if(vfs_fd<0||edos_vfs_write(&boot_vfs,vfs_fd,"E.D-OS VFS",10)!=10||
   !edos_vfs_seek(&boot_vfs,vfs_fd,0,EDOS_VFS_SEEK_SET)||
   edos_vfs_read(&boot_vfs,vfs_fd,boot_vfs_buffer,10)!=10||
   boot_vfs_buffer[0]!='E'||boot_vfs_buffer[9]!='S'){
    serial("VFS_RUNTIME_ERROR\n");for(;;)__asm__ volatile("cli; hlt");
}
edos_vfs_dirent_t vfs_info;
ed_u64 vfs_cursor=0;
if(!edos_vfs_stat(&boot_vfs,"/boot.txt",&vfs_info)||vfs_info.size!=10||
   !edos_vfs_readdir(&boot_vfs,&vfs_cursor,&vfs_info)||vfs_info.size!=10||
   !edos_vfs_close(&boot_vfs,vfs_fd)){
    serial("VFS_RUNTIME_ERROR\n");for(;;)__asm__ volatile("cli; hlt");
}
serial("VFS_RUNTIME_OK\n");
serial("IDT_READY entries=256 exceptions=32\nMEMORY_MAP_OK usable_bytes=");hex(memory_usable_bytes());serial(" hhdm=");hex(memory_hhdm_offset());serial("\n");
void *p=pmm_alloc_page();if(!p){serial("PMM_ALLOC_ERROR\n");for(;;)__asm__ volatile("cli; hlt");}
serial("PMM_ALLOC_ZEROED_OK\n");
if(!pmm_free_page(p)){serial("PMM_FREE_ERROR\n");for(;;)__asm__ volatile("cli; hlt");}
serial("PMM_FREE_OK\n");
if(!memory_validate()){serial("PMM_INVARIANTS_ERROR\n");for(;;)__asm__ volatile("cli; hlt");}
if(pmm_free_page(p)){serial("PMM_DOUBLE_FREE_GUARD_FAILED\n");for(;;)__asm__ volatile("cli; hlt");}
serial("PMM_DOUBLE_FREE_GUARD_OK\n");
void *q=pmm_alloc_page();if(q!=p){serial("PMM_REUSE_ERROR\n");for(;;)__asm__ volatile("cli; hlt");}
volatile u64 *w=(volatile u64*)q;for(u64 i=0;i<512;++i)if(w[i]!=0){serial("PMM_ZERO_ON_REUSE_ERROR\n");for(;;)__asm__ volatile("cli; hlt");}
serial("PMM_REUSE_AND_ZERO_OK\n");
if(!memory_validate()){serial("PMM_INVARIANTS_ERROR\n");for(;;)__asm__ volatile("cli; hlt");}
serial("PMM_INVARIANTS_OK\n");
edos_paging_space_t address_space;
if(!edos_paging_init(&address_space,paging_alloc_page,paging_phys_to_virt,(void*)0)){serial("PAGING_INIT_ERROR\n");for(;;)__asm__ volatile("cli; hlt");}
u64 test_va=0xffff900000000000ULL, test_pa=(u64)(unsigned long long)q-memory_hhdm_offset(), translated=0;
if(!edos_paging_map_4k(&address_space,test_va,test_pa,EDOS_PAGE_PRESENT|EDOS_PAGE_WRITE)||!edos_paging_translate(&address_space,test_va+0x42,&translated,(ed_u64*)0)||translated!=test_pa+0x42||!edos_paging_unmap_4k(&address_space,test_va,(ed_u64*)0)){serial("PAGING_TEST_ERROR\n");for(;;)__asm__ volatile("cli; hlt");}
serial("PAGING_TABLES_OK\n");
edos_paging_space_t active_space;u64 cr3=0,live_va=0,live_pa=0;
__asm__ volatile("mov %%cr3, %0":"=r"(cr3));
if(!edos_paging_adopt(&active_space,cr3&EDOS_PAGE_ADDRESS_MASK,paging_alloc_page,paging_phys_to_virt,(void*)0)||!edos_paging_find_unused_pml4(&active_space,&live_va)){serial("PAGING_ACTIVE_MAP_ERROR\n");for(;;)__asm__ volatile("cli; hlt");}
__asm__ volatile("invlpg (%0)"::"r"(live_va):"memory");
if(!edos_paging_map_4k(&active_space,live_va,test_pa,EDOS_PAGE_PRESENT|EDOS_PAGE_WRITE)){serial("PAGING_ACTIVE_MAP_ERROR\n");for(;;)__asm__ volatile("cli; hlt");}
volatile u64 *alias=(volatile u64*)(unsigned long long)live_va;*alias=0x45444f5350414745ULL;
if(*(volatile u64*)q!=0x45444f5350414745ULL||!edos_paging_unmap_4k(&active_space,live_va,&live_pa)||live_pa!=test_pa){serial("PAGING_ACTIVE_VERIFY_ERROR\n");for(;;)__asm__ volatile("cli; hlt");}
__asm__ volatile("invlpg (%0)"::"r"(live_va):"memory");
*(volatile u64*)q=0;
serial("PAGING_ACTIVE_MAP_OK\n");
if(keyboard_ready)serial("PS2_KEYBOARD_IRQ_READY\n");
u64 timer_start=edos_timer_ticks();
while(edos_timer_ticks()-timer_start<3ULL)__asm__ volatile("hlt");
serial("PIT_TIMER_IRQ_OK\nKERNEL_MEMORY_MILESTONE_OK\nTEST: triggering breakpoint exception (vector 3).\n");
__asm__ volatile("int3");for(;;)__asm__ volatile("cli; hlt");
}
__attribute__((noreturn)) void kernel_exception_panic(u64 vector,u64 error_code){
serial("EXCEPTION vector=");hex(vector);serial(" error=");hex(error_code);serial("\nKERNEL_HALTED_ON_EXCEPTION\n");for(;;)__asm__ volatile("cli; hlt");
}
