#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"
#include <stdlib.h>
#include <string.h>

// Callback function to safely gather correct source paths
bool gather_sources_callback(Walk_Entry entry) {
    size_t len = strlen(entry.path);
    if (len > 2 && strcmp(&entry.path[len - 2], ".c") == 0) {
        
        // Skip specialized hardware graphic accelerator folders we don't need on PC
        if (strstr(entry.path, "/draw/nxp/") || 
            strstr(entry.path, "/draw/renesas/") || 
            strstr(entry.path, "/draw/vg_lite/") ||
            strstr(entry.path, "/draw/g2d/")) {
            return true;
        }

        // Skip non-SDL standalone driver subdirectories
        if (strstr(entry.path, "/drivers/") && !strstr(entry.path, "/drivers/sdl")) {
            return true;
        }

        File_Paths *sources = (File_Paths *)entry.data;
        da_append(sources, temp_sprintf("%s", entry.path));
    }
    return true;
}

int main(int argc, char **argv) {
    // Ensure local compilation artifact directory exists cleanly
    if (!mkdir_if_not_exists("build")) return 1;

    File_Paths lvgl_sources = {0};
    if (!walk_dir("lvgl/src", gather_sources_callback, .data = &lvgl_sources)) {
        nob_log(NOB_ERROR, "Failed to scan the 'lvgl/src' directory tree structure.");
        return 1;
    }

    File_Paths obj_files = {0};
    Cmd cmd = {0};

    nob_log(NOB_INFO, "--- Phase 1: Compiling 268 LVGL Modules to Object Files ---");

    for (size_t i = 0; i < lvgl_sources.count; ++i) {
        const char *src = lvgl_sources.items[i];
        
        // Flatten the file path name to store it safely inside our build directory
        // e.g., "lvgl/src/core/lv_obj.c" becomes "build/lv_obj.o"
        const char *flat_name = strrchr(src, '/');
        flat_name = flat_name ? flat_name + 1 : src;
        const char *obj = temp_sprintf("build/%.*s.o", (int)(strlen(flat_name) - 2), flat_name);
        da_append(&obj_files, obj);

        // Check if the object file already exists to avoid redundant rebuilding (Incremental Build)
        if (file_exists(obj)) continue;

        cmd.count = 0;
        cmd_append(&cmd, "gcc", "-Wall", "-Wextra", "-O2", "-I.", "-Ilvgl");
        cmd_append(&cmd, "-c", src, "-o", obj);
        if (!cmd_run_sync(cmd)) return 1;
    }

    nob_log(NOB_INFO, "--- Phase 2: Compiling Local Application ---");
    cmd.count = 0;
    cmd_append(&cmd, "gcc", "-Wall", "-Wextra", "-O2", "-I.", "-Ilvgl", "-c", "main.c", "-o", "build/main.o");
    if (!cmd_run_sync(cmd)) return 1;
    da_append(&obj_files, "build/main.o");

    nob_log(NOB_INFO, "--- Phase 3: Linking Binary Executable Sandbox ---");
    cmd.count = 0;
    cmd_append(&cmd, "gcc");
    for (size_t i = 0; i < obj_files.count; ++i) {
        cmd_append(&cmd, obj_files.items[i]);
    }
    cmd_append(&cmd, "-lSDL2", "-lm", "-o", "sandbox");
    if (!cmd_run_sync(cmd)) return 1;

    // Execute window upon run command parameter
    if (argc > 1 && strcmp(argv[1], "run") == 0) {
        cmd.count = 0;
        cmd_append(&cmd, "./sandbox");
        if (!cmd_run_sync(cmd)) return 1;
    }

    return 0;
}

