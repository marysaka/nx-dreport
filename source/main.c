#include <switch.h>

// no doc about svcBreak, I guess it's the same as the 3ds?
#define BREAK_PANIC 0x0

// we aren't an applet
u32 __nx_applet_type = AppletType_None;

// setup a fake heap (we don't need the heap anyway)
char   fake_heap[0x100];

// we override libnx internals to do a minimal init
void __libnx_initheap(void) {
    extern char* fake_heap_start;
    extern char* fake_heap_end;

    // setup newlib fake heap
    fake_heap_start = fake_heap;
    fake_heap_end   = fake_heap + 100;
}

void __appInit(void) {
    Result rc;

    rc = smInitialize();
    if (R_FAILED(rc))
        svcBreak(BREAK_PANIC, rc, 0);
    rc = fsInitialize();
    if (R_FAILED(rc))
        svcBreak(BREAK_PANIC, rc, 0);
}

void __appExit(void) {
    fsdevUnmountAll();
    fsExit();
    smExit();
}

Result fsDeleteSaveData(u64 saveID) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 saveID;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 21;
    raw->saveID = saveID;

    Result rc = serviceIpcDispatch(fsGetServiceSession());
    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }
    return rc;
}

int main(int argc, char **argv) {
    // Delete erpt saves
    Result rc = fsDeleteSaveData(0x80000000000000D1);
    if (R_FAILED(rc))
        svcBreak(BREAK_PANIC, rc, 0);
    return 0;
}
