/*
ucshim.c: lets the Unicorn wheel (2.1.x) run on Apple Silicon macOS 26.

The wheel's QEMU code has two problems on this system (found with a SIGILL/SIGBUS probe):

1. init_cache_info() reads the cache type register (`mrs xN, ctr_el0`), and the kernel
   traps that read (SIGILL). The handler gives xN the value that the line sizes from
   sysctl hw.cachelinesize describe, and skips the instruction.
2. The JIT buffer is MAP_JIT, but the wheel does not switch the thread between "write"
   and "execute" (pthread_jit_write_protect_np). A write to the buffer, or a jump into it,
   then faults (SIGBUS). The handler switches the mode to the one the faulting access
   needs and retries. The same fault twice in a row is a real fault: the handler puts the
   old action back, so the retry crashes as it would without the shim.

run_diff.py builds this file with the host compiler and loads it (ctypes) before it
imports unicorn. Host tool only: it is not part of the game or the guest.
*/

#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <sys/sysctl.h>
#include <sys/ucontext.h>

static uint64_t ctr_value;
static struct sigaction old_bus, old_segv, old_ill;
static __thread uint64_t last_pc, last_addr;

static void on_ill(int sig, siginfo_t *info, void *context)
{
	ucontext_t *uc = context;
	uint32_t insn = *(uint32_t *)uc->uc_mcontext->__ss.__pc;
	(void)info;
	if ((insn & 0xffffffe0u) == 0xd53b0020u) /* mrs xN, ctr_el0 */
	{
		unsigned rt = insn & 31;
		if (rt != 31)
			uc->uc_mcontext->__ss.__x[rt] = ctr_value;
		uc->uc_mcontext->__ss.__pc += 4;
		return;
	}
	sigaction(sig, &old_ill, 0);
}

static void on_fault(int sig, siginfo_t *info, void *context)
{
	ucontext_t *uc = context;
	uint64_t pc = uc->uc_mcontext->__ss.__pc;
	uint64_t addr = (uint64_t)info->si_addr;
	if (pc != last_pc || addr != last_addr)
	{
		last_pc = pc;
		last_addr = addr;
		/* a jump into the buffer needs "execute", a store needs "write" */
		pthread_jit_write_protect_np(pc == addr ? 1 : 0);
		return;
	}
	sigaction(sig, sig == SIGBUS ? &old_bus : &old_segv, 0);
}

int ucshim_install(void)
{
	uint64_t line = 64;
	size_t size = sizeof(line);
	unsigned lg = 0;
	struct sigaction sa = {0};
	sysctlbyname("hw.cachelinesize", &line, &size, 0, 0);
	while ((4u << lg) < line)
		lg++;
	/* CTR_EL0: RES1 bit 31, L1Ip = PIPT (3), DminLine and IminLine = log2(words) */
	ctr_value = (1ull << 31) | ((uint64_t)lg << 16) | (3ull << 14) | lg;
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = on_ill;
	sigaction(SIGILL, &sa, &old_ill);
	sa.sa_sigaction = on_fault;
	sigaction(SIGBUS, &sa, &old_bus);
	sigaction(SIGSEGV, &sa, &old_segv);
	return 0;
}
