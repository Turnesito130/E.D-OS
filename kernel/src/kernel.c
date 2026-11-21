#include "memory.h"
#include "idt.h"
#include "serial.h"
#include "paging.h"
#include "keyboard.h"
typedef unsigned long long u64;
__attribute__((used,section(".limine_requests"),aligned(8)))
volatile u64 limine_base_revision[3]={0xf9562b2d5c95a6c8ULL,0x6a7b384944536bdcULL,3};
static ed_u64 paging_alloc_page(void *opaque){(void)opaque;void *p=pmm_alloc_page();if(!p)return EDOS_PAGING_INVALID;return (ed_u64)(unsigned long long)p-memory_hhdm_offset();}
static ed_u64 *paging_phys_to_virt(ed_u64 physical,void *opaque){(void)opaque;if(physical>~0ULL-memory_hhdm_offset())return (ed_u64*)0;return (ed_u64*)(unsigned long long)(memory_hhdm_offset()+physical);}
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
if(edos_keyboard_available()){edos_keyboard_init();serial("PS2_KEYBOARD_POLL_READY\n");}
serial("KERNEL_MEMORY_MILESTONE_OK\nTEST: triggering breakpoint exception (vector 3).\n");
__asm__ volatile("int3");for(;;)__asm__ volatile("cli; hlt");
}
__attribute__((noreturn)) void kernel_exception_panic(u64 vector,u64 error_code){
serial("EXCEPTION vector=");hex(vector);serial(" error=");hex(error_code);serial("\nKERNEL_HALTED_ON_EXCEPTION\n");for(;;)__asm__ volatile("cli; hlt");
}
