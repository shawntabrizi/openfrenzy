// Adapted from ref/OpenCE-mac-zimm3rmann (github.com/zimm3rmann/halo-ce-ios-macos @ e55684a) port/macos/host/host_main.c (CC0). Renamed HALO_* to OPENFRENZY_*.
/* Native macOS entry point. SDL video remains on the real main thread. */
#include "host.h"
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <ctype.h>
#include <errno.h>
#include <mach-o/dyld.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
static char data_root[4096], save_root[4096];
static int create_directories(const char *path) {
    char buffer[4096];
    if (strlen(path) >= sizeof(buffer)) {
        errno = ENAMETOOLONG;
        return -1;
    }
    strcpy(buffer, path);
    for (char *p = buffer + 1; *p; p++)
        if (*p == '/') {
            *p = 0;
            if (mkdir(buffer, 0755) && errno != EEXIST)
                return -1;
            *p = '/';
        }
    return mkdir(buffer, 0755) && errno != EEXIST ? -1 : 0;
}
void host_logf(int priority, const char *format, ...) {
    (void)priority;
    va_list a;
    va_start(a, format);
    vfprintf(stderr, format, a);
    fputc('\n', stderr);
    va_end(a);
}
void host_log(int priority, const char *text) { host_logf(priority, "%s", text); }
void host_fatal(const char *format, ...) {
    va_list a;
    va_start(a, format);
    vfprintf(stderr, format, a);
    fputc('\n', stderr);
    va_end(a);
    exit(1);
}
void host_abort(const char *reason) { host_fatal("guest abort: %s", reason); }
void host_exit(int code) {
    host_logf(HOST_LOG_INFO, "Game exited (%d)", code);
    exit(code);
}
int host_errno(void) { return host_linux_errno(errno); }
void host_android_path(int which, char *buffer, uint32_t size) {
    snprintf(buffer, size, "%s", which ? save_root : data_root);
}
extern char **environ;
static uint32_t make_boot(void) {
    size_t size = 0x20000;
    char *memory = host_low_map(size, PROT_READ | PROT_WRITE);
    if (!memory)
        host_fatal("No guest environment memory");
    struct of_guest_boot *boot = (void *)memory;
    uint32_t *args = (void *)(memory + sizeof(*boot));
    uint32_t *env = args + 2;
    char *strings = (void *)(env + 256);
    unsigned count = 0;
    strcpy(strings, "openfrenzy");
    args[0] = (uint32_t)(uintptr_t)strings;
    args[1] = 0;
    strings += sizeof("openfrenzy");
    for (char **p = environ; *p && count < 255; p++) {
        size_t n = strlen(*p) + 1;
        if (strings + n > memory + size)
            break;
        memcpy(strings, *p, n);
        env[count++] = (uint32_t)(uintptr_t)strings;
        strings += n;
    }
    env[count] = 0;
    *boot = (struct of_guest_boot){1, (uint32_t)(uintptr_t)args, (uint32_t)(uintptr_t)env,
                                     OPENFRENZY_MACOS_PAGE};
    return (uint32_t)(uintptr_t)boot;
}
/* ---------- launch without a shell (OpenFrenzy.app; docs/BUILD.md "Run the app") */

/* <save root>/settings.ini: "name = value" lines; each sets OPENFRENZY_<NAME> ('.' as '_',
   upper case) unless that variable is already set, so the environment still wins.
   true/yes and false/no become 1 and 0 (OPENFRENZY_WINDOWED reads a number). */
static const char settings_template[] =
    "# OpenFrenzy settings. One \"name = value\" a line; # starts a comment.\n"
    "# Each line sets OPENFRENZY_<NAME> (docs/BUILD.md); an environment variable overrides it.\n"
    "# windowed = true\n"
    "# display.vsync = true\n"
    "# audio.volume = 1.0\n"
    "# display.anti_aliasing = off\n";
static char *trim(char *text) {
    while (*text == ' ' || *text == '\t')
        text++;
    char *end = text + strlen(text);
    while (end > text && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' || end[-1] == '\n'))
        *--end = 0;
    return text;
}
static void apply_settings_file(const char *root) {
    char path[4096], line[1024];
    snprintf(path, sizeof(path), "%s/settings.ini", root);
    FILE *file = fopen(path, "r");
    if (!file) {
        if ((file = fopen(path, "w"))) {
            fputs(settings_template, file);
            fclose(file);
        }
        return;
    }
    while (fgets(line, sizeof(line), file)) {
        char *equals = strchr(line, '=');
        char *name = trim(line);
        if (!*name || *name == '#' || !equals)
            continue;
        *equals = 0;
        name = trim(name);
        char *value = trim(equals + 1);
        char variable[160] = "OPENFRENZY_";
        size_t length = strlen(variable);
        for (char *c = name; *c && length + 1 < sizeof(variable); c++)
            variable[length++] = *c == '.' ? '_' : (char)toupper((unsigned char)*c);
        variable[length] = 0;
        if (!strcasecmp(value, "true") || !strcasecmp(value, "yes"))
            value = "1";
        else if (!strcasecmp(value, "false") || !strcasecmp(value, "no"))
            value = "0";
        if (getenv(variable))
            host_logf(HOST_LOG_INFO, "settings.ini: %s = %s ignored (environment has %s)", name,
                      value, variable);
        else {
            setenv(variable, value, 0);
            host_logf(HOST_LOG_INFO, "settings.ini: %s=%s", variable, value);
        }
    }
    fclose(file);
}

/* nonzero if path is an Xbox disc image: "MICROSOFT*XBOX*MEDIA" at 0x10000 of the game
   partition, at the offsets that src/vfs/xiso_fs.c tries */
static int is_disc_image(const char *path) {
    static const long long partitions[] = {0, 0x18300000, 0x0FD90000, 0x02080000};
    char magic[20];
    FILE *file = fopen(path, "rb");
    int found = 0;
    if (!file)
        return 0;
    for (unsigned i = 0; !found && i < sizeof(partitions) / sizeof(partitions[0]); i++)
        found = !fseeko(file, partitions[i] + 0x10000, SEEK_SET) &&
                fread(magic, 1, sizeof(magic), file) == sizeof(magic) &&
                !memcmp(magic, "MICROSOFT*XBOX*MEDIA", sizeof(magic));
    fclose(file);
    return found;
}

struct dialog_state {
    volatile int done;
    char path[4096];
};
static void SDLCALL dialog_chosen(void *userdata, const char *const *files, int filter) {
    (void)filter;
    struct dialog_state *state = userdata;
    if (!files)
        host_logf(HOST_LOG_WARN, "File dialog failed: %s", SDL_GetError());
    else if (files[0])
        snprintf(state->path, sizeof(state->path), "%s", files[0]);
    state->done = 1;
}

/* Asks for the disc image until the player picks a valid one or quits (exit 1). */
static void ask_for_disc(char *path, size_t size) {
    static const SDL_DialogFileFilter filters[] = {{"Xbox disc image (ISO)", "iso;xiso"},
                                                   {"All files", "*"}};
    const char *message =
        "OpenFrenzy needs your own copy of the game as an Xbox disc image (XDVDFS .iso, "
        "for example made with extract-xiso).\n\nChoose the disc image file. OpenFrenzy "
        "remembers it. It does not copy it.";
    for (;;) {
        const SDL_MessageBoxButtonData buttons[] = {
            {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Choose Disc Image..."},
            {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Quit"},
        };
        const SDL_MessageBoxData box = {SDL_MESSAGEBOX_INFORMATION, NULL, "OpenFrenzy: game disc",
                                        message, 2, buttons, NULL};
        int button = 0;
        if (!SDL_ShowMessageBox(&box, &button))
            host_fatal("No disc image, and no dialog (%s). Put game.xiso.iso in %s or set "
                       "OPENFRENZY_ISO.", SDL_GetError(), data_root);
        if (button != 1)
            host_exit(1);
        struct dialog_state state = {0};
        SDL_ShowOpenFileDialog(dialog_chosen, &state, NULL, filters, 2, NULL, false);
        while (!state.done) {
            SDL_PumpEvents();
            SDL_Delay(10);
        }
        if (state.path[0] && is_disc_image(state.path)) {
            snprintf(path, size, "%s", state.path);
            return;
        }
        if (state.path[0])
            message = "That file is not an Xbox disc image (no XDVDFS volume found).\n\n"
                      "Choose the game's disc image (.iso).";
    }
}

/* Bundle launch only: finds the disc image when OPENFRENZY_ISO is not set. Order:
   <data root>/game.xiso.iso, the path in <save root>/disc_path.txt (the last choice),
   else a dialog; the choice is stored and passed to the guest as OPENFRENZY_ISO. */
static void find_disc(void) {
    char path[4096], stored[4096];
    const char *environment = getenv("OPENFRENZY_ISO");
    if (environment && *environment)
        return;
    snprintf(path, sizeof(path), "%s/game.xiso.iso", data_root);
    if (!access(path, F_OK))
        return;
    snprintf(stored, sizeof(stored), "%s/disc_path.txt", save_root);
    FILE *file = fopen(stored, "r");
    path[0] = 0;
    if (file) {
        if (!fgets(path, sizeof(path), file))
            path[0] = 0;
        path[strcspn(path, "\r\n")] = 0;
        fclose(file);
    }
    if (!path[0] || !is_disc_image(path)) {
        if (path[0])
            host_logf(HOST_LOG_WARN, "Stored disc image is not usable: %s", path);
        ask_for_disc(path, sizeof(path));
        if ((file = fopen(stored, "w"))) {
            fprintf(file, "%s\n", path);
            fclose(file);
        } else
            host_logf(HOST_LOG_WARN, "Cannot store the disc path in %s", stored);
    }
    setenv("OPENFRENZY_ISO", path, 1);
    host_logf(HOST_LOG_INFO, "Disc image: %s", path);
}

extern void macos_enter_guest_stack(void *top, uint32_t boot) __attribute__((noreturn));
int main(int argc, char **argv) {
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [openfrenzy_guest.elf]\n", argv[0]);
        return 2;
    }
    setbuf(stderr, NULL);
    host_install_signal_handlers();
    /* No argument: launched as OpenFrenzy.app (or the bare host). The guest image, data root
       and save root then have defaults, settings.ini applies and the disc is found or asked for. */
    int bundle = argc == 1;
    char executable[4096], default_image[4096], default_saves[4096];
    uint32_t executable_size = sizeof(executable);
    if (_NSGetExecutablePath(executable, &executable_size))
        host_fatal("Executable path is too long");
    char *slash = strrchr(executable, '/');
    if (!slash)
        host_fatal("Cannot locate application resources");
    *slash = 0;
    /* the bundle's Contents/Resources, else next to the host (a build directory) */
    snprintf(default_image, sizeof(default_image), "%s/../Resources/openfrenzy_guest.elf", executable);
    if (access(default_image, F_OK))
        snprintf(default_image, sizeof(default_image), "%s/openfrenzy_guest.elf", executable);
    const char *home = getenv("HOME");
    if (!home)
        host_fatal("HOME is not set");
    snprintf(default_saves, sizeof(default_saves),
             "%s/Library/Application Support/OpenFrenzy", home);
    const char *saves = getenv("OPENFRENZY_SAVE_ROOT");
    if (!saves)
        saves = bundle ? default_saves : "build/macos/saves";
    if (create_directories(saves) || !realpath(saves, save_root))
        host_fatal("Cannot open saves folder: %s", saves);
    if (bundle) {
        char log_path[4096];
        snprintf(log_path, sizeof(log_path), "%s/openfrenzy.log", save_root);
        freopen(log_path, "w", stderr);
        setbuf(stderr, NULL);
        apply_settings_file(save_root);
    }
    /* the data root holds game.xiso.iso (and screenshots); the app uses the save root */
    const char *root = getenv("OPENFRENZY_DATA_ROOT");
    if (!root)
        root = bundle ? save_root : "assets";
    if (!realpath(root, data_root))
        host_fatal("Game data folder is missing: %s", root);
    setenv("OPENFRENZY_DATA_ROOT", data_root, 1);
    setenv("OPENFRENZY_SAVE_ROOT", save_root, 1);
    SDL_SetMainReady();
    SDL_SetHint(SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES, "0");
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
        host_fatal("Cannot initialize display: %s", SDL_GetError());
    if (bundle)
        find_disc();
    const SDL_DisplayMode *display = SDL_GetDesktopDisplayMode(SDL_GetPrimaryDisplay());
    if (display && display->h > 0) {
        char width[32];
        snprintf(width, sizeof(width), "%d", (480 * display->w / display->h) & ~1);
        setenv("OPENFRENZY_DISPLAY_WIDTH", width, 0);
        host_logf(HOST_LOG_INFO, "Display %dx%d; game aspect %sx480", display->w,
                  display->h, getenv("OPENFRENZY_DISPLAY_WIDTH"));
    }
    const char *image_path = argc == 2 ? argv[1] : default_image;
    FILE *file = fopen(image_path, "rb");
    if (!file)
        host_fatal("Cannot open guest image: %s", image_path);
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);
    if (size <= 0 || size > 256 * 1024 * 1024)
        host_fatal("Invalid guest image size");
    void *image = malloc((size_t)size);
    if (!image || fread(image, 1, (size_t)size, file) != (size_t)size)
        host_fatal("Cannot read guest image");
    fclose(file);
    if (host_load_image(image, (size_t)size))
        host_fatal("Cannot load rebased game image");
    free(image);
    uint32_t boot = make_boot();
    size_t stack_size = 16 * 1024 * 1024;
    char *stack = host_low_map(stack_size + OPENFRENZY_MACOS_PAGE, PROT_READ | PROT_WRITE);
    if (!stack)
        host_fatal("Cannot allocate main stack");
    mprotect(stack, OPENFRENZY_MACOS_PAGE, PROT_NONE);
    host_logf(HOST_LOG_INFO, "OpenFrenzy ARM64 starting: data %s; saves %s", data_root, save_root);
    macos_enter_guest_stack(stack + stack_size + OPENFRENZY_MACOS_PAGE, boot);
}
