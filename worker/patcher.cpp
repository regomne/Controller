#include "common.h"
#include "Communication.h"
#include <windows.h>
#include <ilhook.h>
#include <TlHelp32.h>
#include <vector>
#include "ConcurrentQueue.h"

#include "../gs/toolfun.h"
#include "patcher.h"


extern ConcurrentQueue<InstructionPack> SendingQueue;

using namespace std;

BOOL SuspendAllThreadExpectSelf(vector<int>& theadIdStack)
{
    HANDLE hThreadSnap = INVALID_HANDLE_VALUE;
    THREADENTRY32 te32;

    hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hThreadSnap == INVALID_HANDLE_VALUE)
        return(FALSE);

    te32.dwSize = sizeof(THREADENTRY32);
    if (!Thread32First(hThreadSnap, &te32))
    {
        DBGOUT(("Thread32First Failed"));  // Show cause of failure
        CloseHandle(hThreadSnap);     // Must clean up the snapshot object!
        return(FALSE);
    }

    int owner = GetCurrentProcessId();
    int selfId = GetCurrentThreadId();
    do
    {
        if (te32.th32OwnerProcessID == owner && te32.th32ThreadID != selfId)
        {
            auto ht = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID);
            if (ht == NULL)
            {
                //log
                DBGOUT(("%d thread cant be open.", te32.th32ThreadID));
                continue;
            }
            auto ret = SuspendThread(ht);
            if (ret != -1)
                theadIdStack.push_back(te32.th32ThreadID);
            else
            {
                DBGOUT(("%d thead cant be suspended.", te32.th32ThreadID));
            }

            CloseHandle(ht);
        }
    } while (Thread32Next(hThreadSnap, &te32));

    CloseHandle(hThreadSnap);
    return(TRUE);
}

BOOL ResumeAllThread(vector<int>& threadIdStack)
{
    for (int i = 0; i < threadIdStack.size(); i++)
    {
        auto ht = OpenThread(THREAD_SUSPEND_RESUME, FALSE, threadIdStack[i]);
        if (ht == NULL)
        {
            DBGOUT(("%d thread cant be open while resume.", threadIdStack[i]));
            continue;
        }
        auto ret = ResumeThread(ht);
        if (ret == -1)
        {
            DBGOUT(("%d thread can't be resumed.", threadIdStack[i]));
        }
        CloseHandle(ht);
    }
    return TRUE;
}
