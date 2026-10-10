/*
CRT0.C

Minimal guest runtime for the build skeleton: the image header and __guest_start, without
musl. It proves that the toolchain, the of-rebase pass, the linker script and the host
loader work together. The game image uses the full runtime (guest musl) instead.

Based on ref/OpenCE-mac-zimm3rmann port/android/guest/runtime/guest_start.c (CC0).
*/

#include "openfrenzy_guest_abi.h"

extern char __guest_image_end[];
extern unsigned long long __host_import_table[];
extern const char __host_import_names[];
extern const unsigned int __host_import_count;
extern void (*__guest_init_start[])(void);
extern void (*__guest_init_end[])(void);

/* host imports (port/macos/host_imports.list) */
void host_exit(int code) __attribute__((noreturn));
void host_abort(const char *reason) __attribute__((noreturn));

void __guest_start(const struct of_guest_boot *boot);
void __guest_thread_start(unsigned int thread);
unsigned int __guest_thread_attach(void);

__attribute__((section("__TEXT,__guest_header"), used))
const struct of_guest_header __guest_header =
{
	OPENFRENZY_GUEST_MAGIC,
	OPENFRENZY_GUEST_ABI_VERSION,
	(uint32_t)__guest_image_end,
	(uint32_t)__host_import_table,
	(uint32_t)__host_import_names,
	(uint32_t)&__host_import_count,
	(uint32_t)__guest_start,
	(uint32_t)__guest_thread_start,
	(uint32_t)__guest_thread_attach,
	(uint32_t)__guest_init_start,
	(uint32_t)__guest_init_end,
};

extern int main(int argc, char **argv);

void __guest_start(const struct of_guest_boot *boot)
{
	void (**constructor)(void);

	for (constructor = __guest_init_start; constructor < __guest_init_end; constructor++)
		(*constructor)();
	host_exit(main((int)boot->argc, (char **)boot->argv));
}

void __guest_thread_start(unsigned int thread)
{
	(void)thread;
	host_abort("skeleton guest: threads are not supported");
}

unsigned int __guest_thread_attach(void)
{
	host_abort("skeleton guest: threads are not supported");
}
