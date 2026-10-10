/*
PORT_CONFIG.H

The platform layer's settings (interface from OpenCE; openfrenzy implements it
in of_config.c without OpenCE's config.toml). A setting is named
"section.key" ("display.vsync") and has a built-in default; the environment
variable OPENFRENZY_<SECTION>_<KEY> (OPENFRENZY_DISPLAY_VSYNC=false) overrides
it for the run, and config_write overrides both until the game exits.
paths.data and paths.saves default to the host's OPENFRENZY_DATA_ROOT and
OPENFRENZY_SAVE_ROOT.
*/

#ifndef PORT_CONFIG_H
#define PORT_CONFIG_H

#include <stddef.h>

int config_boolean(const char *name);
long config_integer(const char *name);
double config_real(const char *name);
/* never NULL; "" when unset */
const char *config_string(const char *name);
/* sets a setting from its value as text ("true", "60", "1.5", "all") for this
run (nothing is written to disk); 1 on success, 0 for an unknown setting */
int config_write(const char *name, const char *value);
int config_write_boolean(const char *name, int value);
/* a setting's value as text ("true", "60", "1.5", "all"); 0 if there is no
such setting */
int config_text(const char *name, char *text, size_t size);
/* the save root (platform_save_root), with its separator */
void config_folder(char *path, size_t size);
/* a whole file (a UTF-8 path), NUL terminated, or NULL; free() it (the Android
guest has no SDL_LoadFile) */
char *config_file_read(const char *path, size_t *size);
/* a setting's default, as text; 0 if there is no such setting */
int config_default(const char *name, char *text, size_t size);
/* how many times config_write has changed a setting: what keeps one reads
it again when this moves */
unsigned long config_changes(void);

#endif
