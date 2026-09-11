#include <cstdio>
#include <emscripten.h>
int main() {
    EM_ASM(
        try { FS.mkdir('/working'); } catch(e) {}
        FS.mount(IDBFS, {}, '/working');
        FS.chdir('/working');
        FS.syncfs(true, function(err) {
            console.log('[probe] syncfs err:', err);
            Module.ccall('probe_fs', null, null, null, { async: true });
        });
    );
    return 0;
}
extern "C" void EMSCRIPTEN_KEEPALIVE probe_fs() {
    FILE* f1 = fopen("/fonts/font.ttf", "rb");
    printf("[probe] font.ttf fopen: %s\n", f1 ? "OK" : "FAIL");
    if (f1) { fseek(f1,0,SEEK_END); printf("[probe] font.ttf size: %d\n", (int)ftell(f1)); fclose(f1); }
    FILE* f2 = fopen("/fonts/font_title.ttf", "rb");
    printf("[probe] font_title.ttf fopen: %s\n", f2 ? "OK" : "FAIL");
    if (f2) { fseek(f2,0,SEEK_END); printf("[probe] title size: %d\n", (int)ftell(f2)); fclose(f2); }
    FILE* f3 = fopen("qgarden_record.txt", "r");
    printf("[probe] record in cwd: %s\n", f3 ? "exists" : "absent");
    if (f3) fclose(f3);
    emscripten_cancel_main_loop();
}
