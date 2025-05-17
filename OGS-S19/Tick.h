#pragma once
#include "framework.h"
#include "Bots.h"
#include "Globals.h"

namespace Tick {
	void (*ServerReplicateActors)(void*) = decltype(ServerReplicateActors)(ImageBase + 0x55497b4);

	inline void (*TickFlushOG)(UNetDriver*, float);
	void TickFlush(UNetDriver* Driver, float DeltaTime)
	{
		if (!Driver)
			return;

		AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

		if (!Driver->ReplicationDriver)
		{
			Log("ReplicationDriver Doesent Exist!");
		}

		/*if (Driver && Driver->ClientConnections.Num() > 0 && Driver->ReplicationDriver)
			ServerReplicateActors(Driver->ReplicationDriver);*/

		ServerReplicateActors(Driver->ReplicationDriver);

		if (GameState->GamePhase == EAthenaGamePhase::Warmup
			&& (GameMode->NumPlayers + GameMode->NumBots) >= Globals::MinPlayersForEarlyStart
			&& GameState->WarmupCountdownEndTime > UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) + 10.f) {

			auto TS = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
			auto DR = 10.f;

			GameState->WarmupCountdownEndTime = TS + DR;
			GameMode->WarmupCountdownDuration = DR;
			GameState->WarmupCountdownStartTime = TS;
			GameMode->WarmupEarlyCountdownDuration = DR;
		}

		/*if (Globals::bBotsEnabled) {
			Bots::PlayerBotTick();
		}*/

		return TickFlushOG(Driver, DeltaTime);
	}


	inline float GetMaxTickRate()
	{
		return 30.f;
	}

	void Hook() {
		MH_CreateHook((LPVOID)(ImageBase + 0xaed938), GetMaxTickRate, nullptr);
		MH_CreateHook((LPVOID)(ImageBase + 0xbc72c0), TickFlush, (LPVOID*)&TickFlushOG);

		Log("Tick Hooked!");
	}
}