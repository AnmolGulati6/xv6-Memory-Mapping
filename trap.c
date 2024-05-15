#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"
#include "wmap.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"
#include "stat.h"
#include "fcntl.h"

#define min(a, b) ((a) < (b) ? (a) : (b))

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[]; // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

void tvinit(void)
{
  int i;

  for (i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE << 3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE << 3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
}

void idtinit(void)
{
  lidt(idt, sizeof(idt));
}

// PAGEBREAK: 41
void trap(struct trapframe *tf)
{
  if (tf->trapno == T_SYSCALL)
  {
    if (myproc()->killed)
      exit();
    myproc()->tf = tf;
    syscall();
    if (myproc()->killed)
      exit();
    return;
  }

  switch (tf->trapno)
  {
  case T_PGFLT:
  {   
    int i;
    struct proc *current_process = myproc();
    struct wmapping *mapping;
    uint fault_address = rcr2();
    for (i = 0; i < 16; i++)
    {
      mapping = current_process->wmappings[i];
      if (fault_address < mapping->address + mapping->len && mapping != 0 && fault_address >= mapping->address){
        uint number_of_pages = PGROUNDUP(mapping->len) / PGSIZE;
        for (uint j = 0; j < number_of_pages; j++){
          char *memory = kalloc();
          uint su = j * PGSIZE;
          uint page_address = PGROUNDDOWN(fault_address) + su;
          if (memory == 0){
            kill(myproc()->pid);
            return;
          }
          memset(memory, 0, PGSIZE);
          if (!(mapping->f & MAP_ANONYMOUS)){
            struct file *file = myproc()->ofile[mapping->fd];
            if (file == 0 || !(file->readable)){
              kill(myproc()->pid);
              return;
            }
            uint offs = page_address - mapping->address;
            if (offs >= file->ip->size){
              kill(myproc()->pid);
              return;
            }
            uint read_size = min(file->ip->size - offs, (uint)PGSIZE);
            ilock(file->ip);
            int read = readi(file->ip, memory, offs, read_size);
            iunlock(file->ip);

            if (read != read_size){
              kill(myproc()->pid);
              return;
            }
          }
          if (mappages(current_process->pgdir, (char *)page_address, PGSIZE, V2P(memory), PTE_W | PTE_U) < 0){
            kfree(memory);
            kill(myproc()->pid);
            return;
          }
          break;
        }
        return;
      }
    }
    myproc()->killed = 1;
    return;
  }
  case T_IRQ0 + IRQ_TIMER:
    if (cpuid() == 0)
    {
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE + 1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;

  // PAGEBREAK: 13
  default:
    if (myproc() == 0 || (tf->cs & 3) == 0)
    {
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if (myproc() && myproc()->killed && (tf->cs & 3) == DPL_USER)
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if (myproc() && myproc()->state == RUNNING &&
      tf->trapno == T_IRQ0 + IRQ_TIMER)
    yield();

  // Check if the process has been killed since we yielded
  if (myproc() && myproc()->killed && (tf->cs & 3) == DPL_USER)
    exit();
}
