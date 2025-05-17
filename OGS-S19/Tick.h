#pragma once
#include "framework.h"
#include "Bots.h"

namespace Tick {
	void (*ServerReplicateActors)(void*) = decltype(ServerReplicateActors)(ImageBase + 0x55497b4);

	inline void (*TickFlushOG)(UNetDriver*, float);
	void TickFlush(UNetDriver* Driver, float DeltaTime)
	{
		if (!Driver)
			return;

		if (!Driver->ReplicationDriver)
		{
			Log("ReplicationDriver Doesent Exist!");
		}

		/*if (Driver && Driver->ClientConnections.Num() > 0 && Driver->ReplicationDriver)
			ServerReplicateActors(Driver->ReplicationDriver);*/

		ServerReplicateActors(Driver->ReplicationDriver);

		if (Globals::bBotsEnabled) {
			Bots::PlayerBotTick();
		}

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