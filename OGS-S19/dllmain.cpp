#include "framework.h"
#include "GameMode.h"
#include "PC.h"
#include "Inventory.h"
#include "Abilities.h"
#include "Net.h"
#include "Looting.h"
#include "Misc.h"
#include "Tick.h"
#include "Globals.h"

void InitConsole() {
    AllocConsole();
    FILE* fptr;
    freopen_s(&fptr, "CONOUT$", "w+", stdout);
    SetConsoleTitleA("OGS 19.10 | Starting...");
    Log("Welcome to OGS, Made with love by ObsessedTech!");
}

void LoadWorld() {
    if (!Globals::bCreativeEnabled && !Globals::bSTWEnabled) {
        UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), L"open Artemis_Terrain", nullptr);
    }
    else if (Globals::bCreativeEnabled) {
        UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), L"open Creative_NoApollo_Terrain", nullptr);
    }
    UWorld::GetWorld()->OwningGameInstance->LocalPlayers.Remove(0);
}

void Hook() {
    GameMode::Hook();
    PC::Hook();
    Abilities::Hook();
    Inventory::Hook();
    Looting::Hook();

    Misc::Hook();
    Net::Hook();
    Tick::Hook();

    Sleep(1000);
    MH_EnableHook(MH_ALL_HOOKS);
}

DWORD Main(LPVOID) {
    InitConsole();
    MH_Initialize();
    Log("MinHook Initialised!");

    Hook();

    *(bool*)(InSDKUtils::GetImageBase() + 0xb30cf9f) = false; //GIsClient

    Sleep(1000);
    LoadWorld();

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        CreateThread(0, 0, Main, 0, 0, 0);
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}