#include "memory.h"
#include "idt.h"
typedef unsigned char u8; typedef unsigned short u16; typedef unsigned long long u64;
__attribute__((used,section(".limine_requests"),aligned(8)))
volatile u64 limine_base_revision[3]={0xf9562b2d5c95a6c8ULL,0x6a7b384944536bdcULL,3};
static inline void outb(u16 p,u8 v){__asm__ volatile("outb %0, %1"::"a"(v),"Nd"(p));}
static inline u8 inb(u16 p){u8 v;__asm__ volatile("inb %1, %0":"=a"(v):"Nd"(p));return v;}
static void serial_init(void){outb(0x3F9,0);outb(0x3FB,0x80);outb(0x3F8,1);outb(0x3F9,0);outb(0x3FB,3);outb(0x3FA,0xC7);outb(0x3FC,0x0B);}
static void serial_char(char c){unsigned int t=1000000;while(!(inb(0x3FD)&0x20)&&t)--t;if(t)outb(0x3F8,(u8)c);}
static void serial(const char*s){while(*s){if(*s=='\n')serial_char('\r');serial_char(*s++);}}
static void hex(u64 v){static const char d[]="0123456789abcdef";serial("0x");for(int s=60;s>=0;s-=4)serial_char(d[(v>>s)&15]);}
__attribute__((noreturn)) void kernel_main(void){
serial_init();serial("\nE.D-OS kernel 0.0.4\n");
if(limine_base_revision[2]!=0){serial("BOOT_ERROR\n");for(;;)__asm__ volatile("cli; hlt");}
serial("BOOT_PROTOCOL=Limine revision 3\n");
if(!memory_init()){serial("MEMORY_ERROR\n");for(;;)__asm__ volatile("cli; hlt");}
if(!idt_init()){serial("IDT_ERROR\n");for(;;)__asm__ volatile("cli; hlt");}
serial("IDT_READY entries=256 exceptions=32\nMEMORY_MAP_OK usable_bytes=");hex(memory_usable_bytes());serial(" hhdm=");hex(memory_hhdm_offset());serial("\n");
void *p=pmm_alloc_page();if(!p){serial("PMM_ALLOC_ERROR\n");for(;;)__asm__ volatile("cli; hlt");}
serial("PMM_ALLOC_ZEROED_OK\n");
if(!pmm_free_page(p)){serial("PMM_FREE_ERROR\n");for(;;)__asm__ volatile("cli; hlt");}
serial("PMM_FREE_OK\n");
if(pmm_free_page(p)){serial("PMM_DOUBLE_FREE_GUARD_FAILED\n");for(;;)__asm__ volatile("cli; hlt");}
serial("PMM_DOUBLE_FREE_GUARD_OK\n");
void *q=pmm_alloc_page();if(q!=p){serial("PMM_REUSE_ERROR\n");for(;;)__asm__ volatile("cli; hlt");}
volatile u64 *w=(volatile u64*)q;for(u64 i=0;i<512;++i)if(w[i]!=0){serial("PMM_ZERO_ON_REUSE_ERROR\n");for(;;)__asm__ volatile("cli; hlt");}
serial("PMM_REUSE_AND_ZERO_OK\nKERNEL_MEMORY_MILESTONE_OK\nTEST: triggering breakpoint exception (vector 3).\n");
__asm__ volatile("int3");for(;;)__asm__ volatile("cli; hlt");
}
__attribute__((noreturn)) void kernel_exception_panic(u64 vector,u64 error_code){
serial("EXCEPTION vector=");hex(vector);serial(" error=");hex(error_code);serial("\nKERNEL_HALTED_ON_EXCEPTION\n");for(;;)__asm__ volatile("cli; hlt");
}
