/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>



###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */
TRAPHANDLER_NOEC(traph_divide, T_DIVIDE)
TRAPHANDLER_NOEC(traph_debug,  T_DEBUG )
TRAPHANDLER_NOEC(traph_nmi,    T_NMI   )
TRAPHANDLER_NOEC(traph_brkpt,  T_BRKPT )
TRAPHANDLER_NOEC(traph_oflow,  T_OFLOW )
TRAPHANDLER_NOEC(traph_bound,  T_BOUND )
TRAPHANDLER_NOEC(traph_illop,  T_ILLOP )
TRAPHANDLER_NOEC(traph_device, T_DEVICE)
TRAPHANDLER(traph_dblflt, T_DBLFLT)
TRAPHANDLER(traph_tss, T_TSS)
TRAPHANDLER(traph_segnp, T_SEGNP)
TRAPHANDLER(traph_stack, T_STACK)
TRAPHANDLER(traph_gpflt, T_GPFLT)
TRAPHANDLER(traph_pgflt, T_PGFLT)
TRAPHANDLER_NOEC(traph_fperr, T_FPERR)
TRAPHANDLER(traph_align, T_ALIGN)
TRAPHANDLER_NOEC(traph_mchk,  T_MCHK)
TRAPHANDLER_NOEC(traph_simderr, T_SIMDERR)
TRAPHANDLER_NOEC(traph_syscall, T_SYSCALL)


/*
 * Lab 3: Your code here for _alltraps
 */
_alltraps:
	# 补齐Trapframe结构体
	pushl %ds;
	pushl %es;
	pushal;
	
	# 将内核数据段选择子GD_KD加载到ds\es寄存器
	movw $GD_KD, %ax;
	movw %ax, %ds;
	movw %ax, %es;

	# 将trapfram指针值作为参数传递给trap函数
	pushl %esp;
	call trap;
