#include <windows.h>
#include <tlhelp32.h>
#include <limits.h>

#include "../include/MinHook.h"
#include "buffer.h"
#include "trampoline.h"

#ifndef ARRAYSIZE
    #define ARRAYSIZE(A) (sizeof(A)/sizeof((A)[0]))
#endif

#define INITIAL_HOOK_CAPACITY   32
#define INITIAL_THREAD_CAPACITY 128
#define INVALID_HOOK_POS UINT_MAX
#define ALL_HOOKS_POS    UINT_MAX
#define ACTION_DISABLE      0
#define ACTION_ENABLE       1
#define ACTION_APPLY_QUEUED 2
#define THREAD_ACCESS (THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION | THREAD_SET_CONTEXT)

typedef struct _HOOK_ENTRY
{
    LPVOID pTarget;
    LPVOID pDetour;
    LPVOID pTrampoline;
    UINT8  backup[8];
    UINT8  patchAbove  : 1;
    UINT8  isEnabled   : 1;
    UINT8  queueEnable : 1;
    UINT   nIP : 4;
    UINT8  oldIPs[8];
    UINT8  newIPs[8];
} HOOK_ENTRY, *PHOOK_ENTRY;

typedef struct _FROZEN_THREADS
{
    LPDWORD pItems;
    UINT    capacity;
    UINT    size;
} FROZEN_THREADS, *PFROZEN_THREADS;

static volatile LONG g_isLocked = FALSE;
static HANDLE g_hHeap = NULL;

static struct
{
    PHOOK_ENTRY pItems;
    UINT        capacity;
    UINT        size;
} g_hooks;

static UINT FindHookEntry(LPVOID pTarget)
{
    for (UINT i = 0; i < g_hooks.size; ++i)
        if ((ULONG_PTR)pTarget == (ULONG_PTR)g_hooks.pItems[i].pTarget) return i;
    return INVALID_HOOK_POS;
}

static PHOOK_ENTRY AddHookEntry()
{
    if (g_hooks.pItems == NULL) {
        g_hooks.capacity = INITIAL_HOOK_CAPACITY;
        g_hooks.pItems = (PHOOK_ENTRY)HeapAlloc(g_hHeap, 0, g_hooks.capacity * sizeof(HOOK_ENTRY));
        if (g_hooks.pItems == NULL) return NULL;
    } else if (g_hooks.size >= g_hooks.capacity) {
        PHOOK_ENTRY p = (PHOOK_ENTRY)HeapReAlloc(g_hHeap, 0, g_hooks.pItems, (g_hooks.capacity * 2) * sizeof(HOOK_ENTRY));
        if (p == NULL) return NULL;
        g_hooks.capacity *= 2;
        g_hooks.pItems = p;
    }
    return &g_hooks.pItems[g_hooks.size++];
}

static VOID DeleteHookEntry(UINT pos)
{
    if (pos < g_hooks.size - 1) g_hooks.pItems[pos] = g_hooks.pItems[g_hooks.size - 1];
    g_hooks.size--;
    if (g_hooks.capacity / 2 >= INITIAL_HOOK_CAPACITY && g_hooks.capacity / 2 >= g_hooks.size) {
        PHOOK_ENTRY p = (PHOOK_ENTRY)HeapReAlloc(g_hHeap, 0, g_hooks.pItems, (g_hooks.capacity / 2) * sizeof(HOOK_ENTRY));
        if (p) { g_hooks.capacity /= 2; g_hooks.pItems = p; }
    }
}

static DWORD_PTR FindOldIP(PHOOK_ENTRY pHook, DWORD_PTR ip)
{
    if (pHook->patchAbove && ip == ((DWORD_PTR)pHook->pTarget - sizeof(JMP_REL)))
        return (DWORD_PTR)pHook->pTarget;
    for (UINT i = 0; i < pHook->nIP; ++i)
        if (ip == ((DWORD_PTR)pHook->pTrampoline + pHook->newIPs[i]))
            return (DWORD_PTR)pHook->pTarget + pHook->oldIPs[i];
#if defined(_M_X64) || defined(__x86_64__)
    if (ip == (DWORD_PTR)pHook->pDetour) return (DWORD_PTR)pHook->pTarget;
#endif
    return 0;
}

static DWORD_PTR FindNewIP(PHOOK_ENTRY pHook, DWORD_PTR ip)
{
    for (UINT i = 0; i < pHook->nIP; ++i)
        if (ip == ((DWORD_PTR)pHook->pTarget + pHook->oldIPs[i]))
            return (DWORD_PTR)pHook->pTrampoline + pHook->newIPs[i];
    return 0;
}

static VOID ProcessThreadIPs(HANDLE hThread, UINT pos, UINT action)
{
    CONTEXT c;
#if defined(_M_X64) || defined(__x86_64__)
    DWORD64 *pIP = &c.Rip;
#else
    DWORD *pIP = &c.Eip;
#endif
    c.ContextFlags = CONTEXT_CONTROL;
    if (!GetThreadContext(hThread, &c)) return;

    UINT count = (pos == ALL_HOOKS_POS) ? g_hooks.size : (pos + 1);
    for (UINT i = (pos == ALL_HOOKS_POS) ? 0 : pos; i < count; ++i) {
        PHOOK_ENTRY pHook = &g_hooks.pItems[i];
        BOOL enable;
        switch (action) {
            case ACTION_DISABLE: enable = FALSE; break;
            case ACTION_ENABLE: enable = TRUE; break;
            default: enable = pHook->queueEnable; break;
        }
        if (pHook->isEnabled == enable) continue;
        DWORD_PTR ip = enable ? FindNewIP(pHook, *pIP) : FindOldIP(pHook, *pIP);
        if (ip != 0) { *pIP = ip; SetThreadContext(hThread, &c); }
    }
}

static BOOL EnumerateThreads(PFROZEN_THREADS pThreads)
{
    BOOL succeeded = FALSE;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        THREADENTRY32 te; te.dwSize = sizeof(THREADENTRY32);
        if (Thread32First(hSnapshot, &te)) {
            succeeded = TRUE;
            do {
                if (te.dwSize >= (FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(DWORD))
                    && te.th32OwnerProcessID == GetCurrentProcessId()
                    && te.th32ThreadID != GetCurrentThreadId()) {
                    if (pThreads->pItems == NULL) {
                        pThreads->capacity = INITIAL_THREAD_CAPACITY;
                        pThreads->pItems = (LPDWORD)HeapAlloc(g_hHeap, 0, pThreads->capacity * sizeof(DWORD));
                        if (!pThreads->pItems) { succeeded = FALSE; break; }
                    } else if (pThreads->size >= pThreads->capacity) {
                        pThreads->capacity *= 2;
                        LPDWORD p = (LPDWORD)HeapReAlloc(g_hHeap, 0, pThreads->pItems, pThreads->capacity * sizeof(DWORD));
                        if (!p) { succeeded = FALSE; break; }
                        pThreads->pItems = p;
                    }
                    pThreads->pItems[pThreads->size++] = te.th32ThreadID;
                }
                te.dwSize = sizeof(THREADENTRY32);
            } while (Thread32Next(hSnapshot, &te));
            if (succeeded && GetLastError() != ERROR_NO_MORE_FILES) succeeded = FALSE;
            if (!succeeded && pThreads->pItems) { HeapFree(g_hHeap, 0, pThreads->pItems); pThreads->pItems = NULL; }
        }
        CloseHandle(hSnapshot);
    }
    return succeeded;
}

static MH_STATUS Freeze(PFROZEN_THREADS pThreads, UINT pos, UINT action)
{
    pThreads->pItems = NULL; pThreads->capacity = 0; pThreads->size = 0;
    if (!EnumerateThreads(pThreads)) return MH_ERROR_MEMORY_ALLOC;
    if (pThreads->pItems) {
        for (UINT i = 0; i < pThreads->size; ++i) {
            HANDLE hThread = OpenThread(THREAD_ACCESS, FALSE, pThreads->pItems[i]);
            if (hThread) {
                if (SuspendThread(hThread) != 0xFFFFFFFF) {
                    ProcessThreadIPs(hThread, pos, action);
                }
                CloseHandle(hThread);
            }
        }
    }
    return MH_OK;
}

static VOID Unfreeze(PFROZEN_THREADS pThreads)
{
    if (pThreads->pItems) {
        for (UINT i = 0; i < pThreads->size; ++i) {
            HANDLE hThread = OpenThread(THREAD_ACCESS, FALSE, pThreads->pItems[i]);
            if (hThread) { ResumeThread(hThread); CloseHandle(hThread); }
        }
        HeapFree(g_hHeap, 0, pThreads->pItems);
    }
}

static MH_STATUS EnableHookLL(UINT pos, BOOL enable)
{
    PHOOK_ENTRY pHook = &g_hooks.pItems[pos];
    DWORD oldProtect;
    SIZE_T patchSize = sizeof(JMP_REL);
    LPBYTE pPatchTarget = (LPBYTE)pHook->pTarget;
    if (pHook->patchAbove) { pPatchTarget -= sizeof(JMP_REL); patchSize += sizeof(JMP_REL_SHORT); }
    if (!VirtualProtect(pPatchTarget, patchSize, PAGE_EXECUTE_READWRITE, &oldProtect))
        return MH_ERROR_MEMORY_PROTECT;
    if (enable) {
        PJMP_REL pJmp = (PJMP_REL)pPatchTarget;
        pJmp->opcode = 0xE9;
        pJmp->operand = (INT32)((LPBYTE)pHook->pDetour - (pPatchTarget + sizeof(JMP_REL)));
        if (pHook->patchAbove) {
            PJMP_REL_SHORT pShortJmp = (PJMP_REL_SHORT)pHook->pTarget;
            pShortJmp->opcode = 0xEB;
            pShortJmp->operand = (INT8)(0 - (sizeof(JMP_REL_SHORT) + sizeof(JMP_REL)));
        }
    } else {
        memcpy(pPatchTarget, pHook->backup, pHook->patchAbove ? (sizeof(JMP_REL) + sizeof(JMP_REL_SHORT)) : sizeof(JMP_REL));
    }
    VirtualProtect(pPatchTarget, patchSize, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), pPatchTarget, patchSize);
    pHook->isEnabled = enable;
    pHook->queueEnable = enable;
    return MH_OK;
}

static MH_STATUS EnableAllHooksLL(BOOL enable)
{
    UINT first = INVALID_HOOK_POS;
    for (UINT i = 0; i < g_hooks.size; ++i)
        if (g_hooks.pItems[i].isEnabled != enable) { first = i; break; }
    if (first != INVALID_HOOK_POS) {
        FROZEN_THREADS threads;
        MH_STATUS status = Freeze(&threads, ALL_HOOKS_POS, enable ? ACTION_ENABLE : ACTION_DISABLE);
        if (status == MH_OK) {
            for (UINT i = first; i < g_hooks.size; ++i)
                if (g_hooks.pItems[i].isEnabled != enable)
                    if ((status = EnableHookLL(i, enable)) != MH_OK) break;
            Unfreeze(&threads);
        }
        return status;
    }
    return MH_OK;
}

static VOID EnterSpinLock(VOID)
{
    while (InterlockedCompareExchange(&g_isLocked, TRUE, FALSE) != FALSE) Sleep(1);
}

static VOID LeaveSpinLock(VOID)
{
    InterlockedExchange(&g_isLocked, FALSE);
}

MH_STATUS WINAPI MH_Initialize(VOID)
{
    EnterSpinLock();
    MH_STATUS status = MH_OK;
    if (g_hHeap == NULL) {
        g_hHeap = HeapCreate(0, 0, 0);
        if (g_hHeap) InitializeBuffer();
        else status = MH_ERROR_MEMORY_ALLOC;
    } else status = MH_ERROR_ALREADY_INITIALIZED;
    LeaveSpinLock();
    return status;
}

MH_STATUS WINAPI MH_Uninitialize(VOID)
{
    EnterSpinLock();
    MH_STATUS status = MH_OK;
    if (g_hHeap) {
        status = EnableAllHooksLL(FALSE);
        if (status == MH_OK) {
            UninitializeBuffer();
            HeapFree(g_hHeap, 0, g_hooks.pItems);
            HeapDestroy(g_hHeap);
            g_hHeap = NULL;
            g_hooks.pItems = NULL; g_hooks.capacity = 0; g_hooks.size = 0;
        }
    } else status = MH_ERROR_NOT_INITIALIZED;
    LeaveSpinLock();
    return status;
}

MH_STATUS WINAPI MH_CreateHook(LPVOID pTarget, LPVOID pDetour, LPVOID *ppOriginal)
{
    EnterSpinLock();
    MH_STATUS status = MH_OK;
    if (g_hHeap) {
        if (IsExecutableAddress(pTarget) && IsExecutableAddress(pDetour)) {
            UINT pos = FindHookEntry(pTarget);
            if (pos == INVALID_HOOK_POS) {
                LPVOID pBuffer = AllocateBuffer(pTarget);
                if (pBuffer) {
                    TRAMPOLINE ct;
                    ct.pTarget = pTarget; ct.pDetour = pDetour; ct.pTrampoline = pBuffer;
                    if (CreateTrampolineFunction(&ct)) {
                        PHOOK_ENTRY pHook = AddHookEntry();
                        if (pHook) {
                            pHook->pTarget = ct.pTarget;
#if defined(_M_X64) || defined(__x86_64__)
                            pHook->pDetour = ct.pRelay;
#else
                            pHook->pDetour = ct.pDetour;
#endif
                            pHook->pTrampoline = ct.pTrampoline;
                            pHook->patchAbove = ct.patchAbove;
                            pHook->isEnabled = FALSE; pHook->queueEnable = FALSE;
                            pHook->nIP = ct.nIP;
                            memcpy(pHook->oldIPs, ct.oldIPs, sizeof(ct.oldIPs));
                            memcpy(pHook->newIPs, ct.newIPs, sizeof(ct.newIPs));
                            if (ct.patchAbove)
                                memcpy(pHook->backup, (LPBYTE)pTarget - sizeof(JMP_REL), sizeof(JMP_REL) + sizeof(JMP_REL_SHORT));
                            else
                                memcpy(pHook->backup, pTarget, sizeof(JMP_REL));
                            if (ppOriginal) *ppOriginal = pHook->pTrampoline;
                        } else status = MH_ERROR_MEMORY_ALLOC;
                    } else status = MH_ERROR_UNSUPPORTED_FUNCTION;
                    if (status != MH_OK) FreeBuffer(pBuffer);
                } else status = MH_ERROR_MEMORY_ALLOC;
            } else status = MH_ERROR_ALREADY_CREATED;
        } else status = MH_ERROR_NOT_EXECUTABLE;
    } else status = MH_ERROR_NOT_INITIALIZED;
    LeaveSpinLock();
    return status;
}

MH_STATUS WINAPI MH_RemoveHook(LPVOID pTarget)
{
    EnterSpinLock();
    MH_STATUS status = MH_OK;
    if (g_hHeap) {
        UINT pos = FindHookEntry(pTarget);
        if (pos != INVALID_HOOK_POS) {
            if (g_hooks.pItems[pos].isEnabled) {
                FROZEN_THREADS threads;
                status = Freeze(&threads, pos, ACTION_DISABLE);
                if (status == MH_OK) { status = EnableHookLL(pos, FALSE); Unfreeze(&threads); }
            }
            if (status == MH_OK) { FreeBuffer(g_hooks.pItems[pos].pTrampoline); DeleteHookEntry(pos); }
        } else status = MH_ERROR_NOT_CREATED;
    } else status = MH_ERROR_NOT_INITIALIZED;
    LeaveSpinLock();
    return status;
}

static MH_STATUS EnableHook(LPVOID pTarget, BOOL enable)
{
    EnterSpinLock();
    MH_STATUS status = MH_OK;
    if (g_hHeap) {
        if (pTarget == MH_ALL_HOOKS) status = EnableAllHooksLL(enable);
        else {
            UINT pos = FindHookEntry(pTarget);
            if (pos != INVALID_HOOK_POS) {
                if (g_hooks.pItems[pos].isEnabled != enable) {
                    FROZEN_THREADS threads;
                    status = Freeze(&threads, pos, ACTION_ENABLE);
                    if (status == MH_OK) { status = EnableHookLL(pos, enable); Unfreeze(&threads); }
                } else status = enable ? MH_ERROR_ENABLED : MH_ERROR_DISABLED;
            } else status = MH_ERROR_NOT_CREATED;
        }
    } else status = MH_ERROR_NOT_INITIALIZED;
    LeaveSpinLock();
    return status;
}

MH_STATUS WINAPI MH_EnableHook(LPVOID pTarget) { return EnableHook(pTarget, TRUE); }
MH_STATUS WINAPI MH_DisableHook(LPVOID pTarget) { return EnableHook(pTarget, FALSE); }

MH_STATUS WINAPI MH_CreateHookApi(LPCWSTR pszModule, LPCSTR pszProcName, LPVOID pDetour, LPVOID *ppOriginal)
{
    HMODULE hModule = GetModuleHandleW(pszModule);
    if (!hModule) return MH_ERROR_MODULE_NOT_FOUND;
    LPVOID pTarget = (LPVOID)GetProcAddress(hModule, pszProcName);
    if (!pTarget) return MH_ERROR_FUNCTION_NOT_FOUND;
    return MH_CreateHook(pTarget, pDetour, ppOriginal);
}

const char *WINAPI MH_StatusToString(MH_STATUS status)
{
    switch (status) {
        case MH_UNKNOWN: return "MH_UNKNOWN";
        case MH_OK: return "MH_OK";
        case MH_ERROR_ALREADY_INITIALIZED: return "MH_ERROR_ALREADY_INITIALIZED";
        case MH_ERROR_NOT_INITIALIZED: return "MH_ERROR_NOT_INITIALIZED";
        case MH_ERROR_ALREADY_CREATED: return "MH_ERROR_ALREADY_CREATED";
        case MH_ERROR_NOT_CREATED: return "MH_ERROR_NOT_CREATED";
        case MH_ERROR_ENABLED: return "MH_ERROR_ENABLED";
        case MH_ERROR_DISABLED: return "MH_ERROR_DISABLED";
        case MH_ERROR_NOT_EXECUTABLE: return "MH_ERROR_NOT_EXECUTABLE";
        case MH_ERROR_UNSUPPORTED_FUNCTION: return "MH_ERROR_UNSUPPORTED_FUNCTION";
        case MH_ERROR_MEMORY_ALLOC: return "MH_ERROR_MEMORY_ALLOC";
        case MH_ERROR_MEMORY_PROTECT: return "MH_ERROR_MEMORY_PROTECT";
        case MH_ERROR_MODULE_NOT_FOUND: return "MH_ERROR_MODULE_NOT_FOUND";
        case MH_ERROR_FUNCTION_NOT_FOUND: return "MH_ERROR_FUNCTION_NOT_FOUND";
    }
    return "(unknown)";
}
