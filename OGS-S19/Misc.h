#pragma once
#include "framework.h"

namespace Misc {
    void nullFunc() {}

    int True() {
        return 1;
    }

    int False() {
        return 0;
    }

    static inline void (*KickPlayerOG)(AGameSession*, AController*);
    static void KickPlayer(AGameSession*, AController*) {
        Log("KickPlayer Called!");
        return;
    }

    void (*DispatchRequestOG)(__int64 a1, unsigned __int64* a2, int a3);
    void DispatchRequest(__int64 a1, unsigned __int64* a2, int a3)
    {
        return DispatchRequestOG(a1, a2, 3);
    }

    void Hook() {
        //MH_CreateHook((LPVOID)(ImageBase + 0xd6b454), True, nullptr); // collectgarbage
        MH_CreateHook((LPVOID)(ImageBase + 0x65c4390), KickPlayer, (LPVOID*)&KickPlayerOG); // Kickplayer
        MH_CreateHook((LPVOID)(ImageBase + 0x1674270), DispatchRequest, (LPVOID*)&DispatchRequestOG);

        MH_CreateHook((LPVOID)(ImageBase + 0x65cde18), True, nullptr); // reservation kick

        MH_CreateHook((LPVOID)(ImageBase + 0x44a9b00), nullFunc, nullptr);
        MH_CreateHook((LPVOID)(ImageBase + 0x1f901ac), nullFunc, nullptr);
        MH_CreateHook((LPVOID)(ImageBase + 0x258d0dc), nullFunc, nullptr);
        MH_CreateHook((LPVOID)(ImageBase + 0x677f0e4), nullFunc, nullptr);

        // No Localplayer logs fix
        MH_CreateHook((LPVOID)(ImageBase + 0x257CC1C), nullFunc, nullptr);
        //MH_CreateHook((LPVOID)(ImageBase + 0xD34D3C), nullFunc, nullptr);

        Log("Misc Hooked!");
    }
}