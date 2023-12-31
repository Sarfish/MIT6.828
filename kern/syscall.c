/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/syscall.h>
#include <kern/console.h>
#include <kern/sched.h>

// Print a string to the system console.
// The string is exactly 'len' characters long.
// Destroys the environment on memory errors.
static void
sys_cputs(const char *s, size_t len)
{
	// Check that the user has permission to read memory [s, s+len).
	// Destroy the environment if not.

	// LAB 3: Your code here.
	user_mem_assert(curenv, s, len, PTE_U);
	// Print the string supplied by the user.
	cprintf("%.*s", len, s);
}

// Read a character from the system console without blocking.
// Returns the character, or 0 if there is no input waiting.
static int
sys_cgetc(void)
{
	return cons_getc();
}

// Returns the current environment's envid.
static envid_t
sys_getenvid(void)
{
	return curenv->env_id;
}

// Destroy a given environment (possibly the currently running environment).
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_destroy(envid_t envid)
{
	int r;
	struct Env *e;

	if ((r = envid2env(envid, &e, 1)) < 0)
		return r;
	if (e == curenv)
		cprintf("[%08x] exiting gracefully\n", curenv->env_id);
	else
		cprintf("[%08x] destroying %08x\n", curenv->env_id, e->env_id);
	env_destroy(e);
	return 0;
}

// Deschedule current environment and pick a different one to run.
static void
sys_yield(void)
{
	sched_yield();
}

// Allocate a new environment.
// Returns envid of new environment, or < 0 on error.  Errors are:
//	-E_NO_FREE_ENV if no free environment is available.
//	-E_NO_MEM on memory exhaustion.
static envid_t
sys_exofork(void)
{
	// Create the new environment with env_alloc(), from kern/env.c.
	// It should be left as env_alloc created it, except that
	// status is set to ENV_NOT_RUNNABLE, and the register set is copied
	// from the current environment -- but tweaked so sys_exofork
	// will appear to return 0.

	// LAB 4: Your code here.
	//panic("sys_exofork not implemented");
	struct Env * new_subenv = NULL;
	int error_code = env_alloc(&new_subenv, curenv->env_id);
	if(error_code < 0)return error_code;
	new_subenv->env_status = ENV_NOT_RUNNABLE;
	new_subenv->env_tf = curenv->env_tf;
	new_subenv->env_tf.tf_regs.reg_eax = 0;
	return new_subenv->env_id;
}

// Set envid's env_status to status, which must be ENV_RUNNABLE
// or ENV_NOT_RUNNABLE.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if status is not a valid status for an environment.
static int
sys_env_set_status(envid_t envid, int status)
{
	// Hint: Use the 'envid2env' function from kern/env.c to translate an
	// envid to a struct Env.
	// You should set envid2env's third argument to 1, which will
	// check whether the current environment has permission to set
	// envid's status.

	// LAB 4: Your code here.
	//panic("sys_env_set_status not implemented");
	struct Env * targ_env;
	int errcode = envid2env(envid, &targ_env, true);
	if(errcode != 0)return -E_INVAL;
	targ_env->env_status = status;
	return 0;
}

// Set the page fault upcall for 'envid' by modifying the corresponding struct
// Env's 'env_pgfault_upcall' field.  When 'envid' causes a page fault, the
// kernel will push a fault record onto the exception stack, then branch to
// 'func'.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_set_pgfault_upcall(envid_t envid, void *func)
{
	// LAB 4: Your code here.
	// panic("sys_env_set_pgfault_upcall not implemented");
	cprintf("env %d calling sys_env_set_pgfault_upcall\n", envid);
	struct Env * p_env = NULL;
	int errc = envid2env(envid, &p_env, true);
	if(errc == -E_BAD_ENV){
		return -E_BAD_ENV;
	}
	
	p_env->env_pgfault_upcall = func;
	cprintf("set env-%d's pgfault_upcall to [0x%p]\n", envid, p_env->env_pgfault_upcall);
	return 0;
}

// Allocate a page of memory and map it at 'va' with permission
// 'perm' in the address space of 'envid'.
// The page's contents are set to 0.
// If a page is already mapped at 'va', that page is unmapped as a
// side effect.
//
// perm -- PTE_U | PTE_P must be set, PTE_AVAIL | PTE_W may or may not be set,
//         but no other bits may be set.  See PTE_SYSCALL in inc/mmu.h.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if va >= UTOP, or va is not page-aligned.
//	-E_INVAL if perm is inappropriate (see above).
//	-E_NO_MEM if there's no memory to allocate the new page,
//		or to allocate any necessary page tables.
static int
sys_page_alloc(envid_t envid, void *va, int perm)
{
	// Hint: This function is a wrapper around page_alloc() and
	//   page_insert() from kern/pmap.c.
	//   Most of the new code you write should be to check the
	//   parameters for correctness.
	//   If page_insert() fails, remember to free the page you
	//   allocated!

	// LAB 4: Your code here.
	// panic("sys_page_alloc not implemented");
	
	struct Env* penv = NULL;
	int errc = envid2env(envid, &penv, false);
	if(errc != 0)return errc;

	struct PageInfo * p_newphypg = page_alloc(ALLOC_ZERO);
	if(NULL == p_newphypg){
		return -E_NO_MEM;
	}

	errc = page_insert(penv->env_pgdir, p_newphypg, va, perm);
	if(errc != 0){
		page_free(p_newphypg);
		return -E_NO_MEM;
	}
	return 0;
	
}

// Map the page of memory at 'srcva' in srcenvid's address space
// at 'dstva' in dstenvid's address space with permission 'perm'.
// Perm has the same restrictions as in sys_page_alloc, except
// that it also must not grant write access to a read-only
// page.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if srcenvid and/or dstenvid doesn't currently exist,
//		or the caller doesn't have permission to change one of them.
//	-E_INVAL if srcva >= UTOP or srcva is not page-aligned,
//		or dstva >= UTOP or dstva is not page-aligned.
//	-E_INVAL is srcva is not mapped in srcenvid's address space.
//	-E_INVAL if perm is inappropriate (see sys_page_alloc).
//	-E_INVAL if (perm & PTE_W), but srcva is read-only in srcenvid's
//		address space.
//	-E_NO_MEM if there's no memory to allocate any necessary page tables.
static int
sys_page_map(envid_t srcenvid, void *srcva,
	     envid_t dstenvid, void *dstva, int perm)
{
	// Hint: This function is a wrapper around page_lookup() and
	//   page_insert() from kern/pmap.c.
	//   Again, most of the new code you write should be to check the
	//   parameters for correctness.
	//   Use the third argument to page_lookup() to
	//   check the current permissions on the page.

	// LAB 4: Your code here.
	// panic("sys_page_map not implemented");

	int errc;
	struct Env *penv_s = NULL, *penv_d = NULL;
	errc = envid2env(srcenvid, &penv_s, true);
	if(-E_BAD_ENV == errc){
		return -E_BAD_ENV;
	}
	errc = envid2env(dstenvid, &penv_d, true);
	if(-E_BAD_ENV == errc){
		return -E_BAD_ENV;
	}
	
	if((physaddr_t)(srcva) >= UTOP || PGOFF(srcva) ||
	   (physaddr_t)(dstva) >= UTOP || PGOFF(dstva))return -E_INVAL;
	if ( (perm & PTE_U) == 0 || (perm & PTE_P) == 0 || (perm & ~PTE_SYSCALL))
		return -E_INVAL;
	pte_t *pte = NULL;
	struct PageInfo * spage = page_lookup(penv_s->env_pgdir, srcva, &pte);
	
	if(NULL == spage){return -E_INVAL;}
	if(0 == ((*pte) & perm)){return -E_INVAL;}
	
	if((perm & PTE_W) && !((*pte)&PTE_W))return -E_INVAL;

	errc = page_insert(penv_d->env_pgdir, spage, dstva, perm);
	if(-E_NO_MEM == errc)return -E_NO_MEM;

	return 0;
}

// Unmap the page of memory at 'va' in the address space of 'envid'.
// If no page is mapped, the function silently succeeds.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if va >= UTOP, or va is not page-aligned.
static int
sys_page_unmap(envid_t envid, void *va)
{
	// Hint: This function is a wrapper around page_remove().

	// LAB 4: Your code here.
	// panic("sys_page_unmap not implemented");
	int errc;
	struct Env *penv = NULL;
	errc = envid2env(envid, &penv, true);
	if(errc)return errc;

	if((uint32_t)va >= UTOP || PGOFF((uint32_t)va))return -E_INVAL;
	page_remove(penv->env_pgdir, va);
	return 0;
}

// Try to send 'value' to the target env 'envid'.
// If srcva < UTOP, then also send page currently mapped at 'srcva',
// so that receiver gets a duplicate mapping of the same page.
//
// The send fails with a return value of -E_IPC_NOT_RECV if the
// target is not blocked, waiting for an IPC.
//
// The send also can fail for the other reasons listed below.
//
// Otherwise, the send succeeds, and the target's ipc fields are
// updated as follows:
//    env_ipc_recving is set to 0 to block future sends;
//    env_ipc_from is set to the sending envid;
//    env_ipc_value is set to the 'value' parameter;
//    env_ipc_perm is set to 'perm' if a page was transferred, 0 otherwise.
// The target environment is marked runnable again, returning 0
// from the paused sys_ipc_recv system call.  (Hint: does the
// sys_ipc_recv function ever actually return?)
//
// If the sender wants to send a page but the receiver isn't asking for one,
// then no page mapping is transferred, but no error occurs.
// The ipc only happens when no errors occur.
//
// Returns 0 on success, < 0 on error.
// Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist.
//		(No need to check permissions.)
//	-E_IPC_NOT_RECV if envid is not currently blocked in sys_ipc_recv,
//		or another environment managed to send first.
//	-E_INVAL if srcva < UTOP but srcva is not page-aligned.
//	-E_INVAL if srcva < UTOP and perm is inappropriate
//		(see sys_page_alloc).
//	-E_INVAL if srcva < UTOP but srcva is not mapped in the caller's
//		address space.
//	-E_INVAL if (perm & PTE_W), but srcva is read-only in the
//		current environment's address space.
//	-E_NO_MEM if there's not enough memory to map srcva in envid's
//		address space.
static int
sys_ipc_try_send(envid_t envid, uint32_t value, void *srcva, unsigned perm)
{
	// LAB 4: Your code here.
	panic("sys_ipc_try_send not implemented");
}

// Block until a value is ready.  Record that you want to receive
// using the env_ipc_recving and env_ipc_dstva fields of struct Env,
// mark yourself not runnable, and then give up the CPU.
//
// If 'dstva' is < UTOP, then you are willing to receive a page of data.
// 'dstva' is the virtual address at which the sent page should be mapped.
//
// This function only returns on error, but the system call will eventually
// return 0 on success.
// Return < 0 on error.  Errors are:
//	-E_INVAL if dstva < UTOP but dstva is not page-aligned.
static int
sys_ipc_recv(void *dstva)
{
	// LAB 4: Your code here.
	panic("sys_ipc_recv not implemented");
	return 0;
}

// Dispatches to the correct kernel function, passing the arguments.
int32_t
syscall(uint32_t syscallno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
	// Call the function corresponding to the 'syscallno' parameter.
	// Return any appropriate return value.
	// LAB 3: Your code here.

	//panic("syscall not implemented");
	uint32_t sysret = 0;
	switch (syscallno) {
	case SYS_cputs:
		sys_cputs((const char*)a1, a2);
		return sysret;
		break;
	case SYS_cgetc:
		sysret = sys_cgetc();
		return sysret;
		break;
	case SYS_getenvid:
		sysret = sys_getenvid();
		return sysret;
		break;
	case SYS_env_destroy:
		sysret = sys_env_destroy(a1);
		return sysret;
		break;
	case SYS_yield:
		sys_yield();
		return 0;
		break;
	case SYS_exofork:
		sysret = sys_exofork();
		return sysret;
	case SYS_env_set_status:
		sysret = sys_env_set_status((envid_t)a1, (int)a2);
		return sysret;
	case SYS_page_alloc:
		sysret = sys_page_alloc((envid_t)a1, (void*)a2, (int)a3);
		return sysret;
	case SYS_page_map:
		sysret = sys_page_map((envid_t)a1, (void*)a2, (envid_t)a3, (void*)a4, (int)a5);
		return sysret;
	case SYS_page_unmap:
		sysret = sys_page_unmap((envid_t)a1, (void*)a2);
		return sysret;
	case SYS_env_set_pgfault_upcall:
		sysret = sys_env_set_pgfault_upcall((envid_t)a1, (void*)a2);
		return sysret;
	case NSYSCALLS:
		panic("unimplemented syscall: NSYSCALLS");
		break;
	default:
		return -E_INVAL;
	}
}

