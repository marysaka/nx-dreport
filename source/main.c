#include <switch.h>

#define ERPT_SAVE_ID 0x80000000000000D1

// we aren't an applet
u32 __nx_applet_type = AppletType_None;

// setup a fake heap (we don't need the heap anyway)
char   fake_heap[0x1000];

void fatalLater(Result err)
{
#ifdef DEBUG
    Handle srv;

    while (R_FAILED(smGetServiceOriginal(&srv, smEncodeName("fatal:u"))))
    {
        // wait one sec and retry
        svcSleepThread(1000000000L);
    }

    // fatal is here time, fatal like a boss    
    IpcCommand c;
    ipcInitialize(&c);
    ipcSendPid(&c);
    struct {
        u64 magic;
        u64 cmd_id;
        u64 result;
        u64 unknown;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;
    raw->result = err;
    raw->unknown = 0;

    ipcDispatch(srv);
    svcCloseHandle(srv);
#endif
}

// we override libnx internals to do a minimal init
void __libnx_initheap(void) {
    extern char* fake_heap_start;
    extern char* fake_heap_end;

    // setup newlib fake heap
    fake_heap_start = fake_heap;
    fake_heap_end   = fake_heap + 0x1000;
}

void __appInit(void) {
    Result rc;

    rc = smInitialize();
    if (R_FAILED(rc))
        fatalLater(rc);
    rc = fsInitialize();
    if (R_FAILED(rc))
        fatalLater(rc);
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

void registerFspLr()
{
    if (kernelAbove400())
        return;

    Result rc = fsprInitialize();
    if (R_FAILED(rc))
        fatalLater(rc);

    u64 pid;
    svcGetProcessId(&pid, 0xFFFF8001);

    rc = fsprRegisterProgram(pid, 0x4200000000000000, FsStorageId_NandSystem, NULL, 0, NULL, 0);
    if (R_FAILED(rc))
        fatalLater(rc);
    fsprExit();
}

int main(int argc, char **argv) {
    registerFspLr();
    // Delete erpt saves
    Result rc = fsDeleteSaveData(ERPT_SAVE_ID);

    if (R_FAILED(rc))
        fatalLater(rc);

    return 0;
}
