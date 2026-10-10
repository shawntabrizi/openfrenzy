/* openfrenzy: stands in for Halo's source/main/console.h, which OpenCE's
d3d8_gl.c includes for console_printf (screenshot messages). Implemented in
compat/halo_hooks.c. */
#ifndef OPENFRENZY_COMPAT_CONSOLE_H
#define OPENFRENZY_COMPAT_CONSOLE_H

unsigned char console_is_active(void);
void console_printf(unsigned char clear, const char *format, ...);

#endif
