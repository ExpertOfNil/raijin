#define NOB_IMPLEMENTATION
#include "nob.h"

#define COMMON_CFLAGS \
    "-std=c99", "-Wall", "-Wextra", "-pedantic", "-ggdb"
#define BUILD_DIR "build/"
#define SRC_DIR "src/"

int main(int argc, char** argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);
    Nob_Cmd cmd = {0};
    if (!nob_mkdir_if_not_exists(BUILD_DIR)) return 1;

    nob_cmd_append(&cmd, "clang", COMMON_CFLAGS);
    nob_cmd_append(&cmd, "-Iinclude", "-Ilib/wgpu/include", "-Ilib/cglm/include");
    nob_cmd_append(&cmd, SRC_DIR "main.c");
    nob_cmd_append(&cmd, "-o", BUILD_DIR "raijin");
    nob_cmd_append(&cmd, "-lm", "-Llib/wgpu", "-lwgpu_native", "-Llib/cglm", "-lcglm", "-lSDL3");
    if (!nob_cmd_run_sync(cmd)) return 1;
    return 0;
}
