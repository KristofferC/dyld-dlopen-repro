// dlopen() gets slower with every library already loaded from outside the dyld shared cache.
//
//   repro <dir> <n>        dlopen <dir>/lib0.dylib ... lib<n-1>.dylib, one call each
//   repro <dir> umbrella   one dlopen of <dir>/umbrella.dylib, which links all of them
#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static double now_ms(void) { return clock_gettime_nsec_np(CLOCK_UPTIME_RAW) / 1e6; }

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s <dir> <n>|umbrella\n", argv[0]);
        return 2;
    }
    char path[4096];
    uint32_t before = _dyld_image_count();
    if (strcmp(argv[2], "umbrella") == 0) {
        snprintf(path, sizeof path, "%s/umbrella.dylib", argv[1]);
        double t0 = now_ms();
        if (!dlopen(path, RTLD_NOW | RTLD_LOCAL)) {
            fprintf(stderr, "%s\n", dlerror());
            return 1;
        }
        printf("1 dlopen call loading %u images: %.1f ms\n", _dyld_image_count() - before, now_ms() - t0);
        return 0;
    }
    int n = atoi(argv[2]), bucket = n >= 20 ? n / 10 : 1;
    double total = 0, acc = 0;
    printf("%10s  %16s\n", "call", "ms per dlopen");
    for (int i = 0; i < n; i++) {
        snprintf(path, sizeof path, "%s/lib%d.dylib", argv[1], i);
        double t0 = now_ms();
        void *h = dlopen(path, RTLD_NOW | RTLD_LOCAL);
        double dt = now_ms() - t0;
        if (!h) {
            fprintf(stderr, "%s\n", dlerror());
            return 1;
        }
        total += dt;
        acc += dt;
        if ((i + 1) % bucket == 0) {
            printf("%4d..%-4d  %16.3f\n", i + 2 - bucket, i + 1, acc / bucket);
            acc = 0;
        }
    }
    printf("%d dlopen calls loading %u images: %.1f ms\n", n, _dyld_image_count() - before, total);
    return 0;
}
