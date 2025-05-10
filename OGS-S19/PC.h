#pragma once
#include "framework.h"
#include "Looting.h"

namespace PC {
	void (*ServerAcknowledgePossessionOG)(AFortPlayerControllerAthena* PC, APawn* Pawn);
	inline void ServerAcknowledgePossession(AFortPlayerControllerAthena* PC, APawn* Pawn)
	{
		Log("ServerAck Called!");
		PC->AcknowledgedPawn = Pawn;
		return ServerAcknowledgePossessionOG(PC, Pawn);
	}

	void (*ServerReadyToStartMatchOG)(AFortPlayerControllerAthena* PC);
	void ServerReadyToStartMatch(AFortPlayerControllerAthena* PC)
	{
		if (!PC) {
			return;
		}
		AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
		
		static bool setupWorld = false;
		if (!setupWorld) {
			Looting::DestroyFloorLootSpawners();
		}

		return ServerReadyToStartMatchOG(PC);
	}

	inline void ServerAttemptAircraftJump(UFortControllerComponent_Aircraft* Comp, FRotator Rotation)
	{
		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

		auto PC = (AFortPlayerControllerAthena*)Comp->GetOwner();
		UWorld::GetWorld()->AuthorityGameMode->RestartPlayer(PC);

		if (PC->MyFortPawn)
		{
			PC->ClientSetRotation(Rotation, true);
			PC->MyFortPawn->BeginSkydiving(true);
			PC->MyFortPawn->SetHealth(100);
			PC->MyFortPawn->SetShield(0);
		}

		if (PC && PC->WorldInventory)
		{
			for (int i = PC->WorldInventory->Inventory.ReplicatedEntries.Num() - 1; i >= 0; i--)
			{
				if (((UFortWorldItemDefinition*)PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition)->bCanBeDropped)
				{
					int Count = PC->WorldInventory->Inventory.ReplicatedEntries[i].Count;
					Inventory::RemoveItem(PC, PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemGuid, Count);
				}
			}
		}

		PC->MyFortPawn->OnRep_IsInsideSafeZone();

		GameState->OnRep_SafeZoneDamage();
		GameState->OnRep_SafeZoneIndicator();
		GameState->OnRep_SafeZonePhase();
	}

	inline void (*ClientOnPawnDiedOG)(AFortPlayerControllerAthena*, FFortPlayerDeathReport);
	inline void ClientOnPawnDied(AFortPlayerControllerAthena* DeadPC, FFortPlayerDeathReport DeathReport)
	{
		DeadPC->bMarkedAlive = false;

		auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
		auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		AFortPlayerStateAthena* DeadState = (AFortPlayerStateAthena*)DeadPC->PlayerState;
		AFortPlayerPawnAthena* KillerPawn = (AFortPlayerPawnAthena*)DeathReport.KillerPawn;
		AFortPlayerStateAthena* KillerState = (AFortPlayerStateAthena*)DeathReport.KillerPlayerState;

		if (!GameState->IsRespawningAllowed(DeadState))
		{
			if (DeadPC && DeadPC->WorldInventory)
			{
				for (size_t i = 0; i < DeadPC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
				{
					if (((UFortWorldItemDefinition*)DeadPC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition)->bCanBeDropped)
					{
						SpawnPickup(DeadPC->WorldInventory->Inventory.ItemInstances[i]->ItemEntry.ItemDefinition, DeadPC->WorldInventory->Inventory.ItemInstances[i]->ItemEntry.Count, DeadPC->WorldInventory->Inventory.ItemInstances[i]->ItemEntry.LoadedAmmo, DeadPC->Pawn->K2_GetActorLocation(), EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::PlayerElimination, DeadPC->MyFortPawn);
					}
				}
			}
		}

		DeadState->PawnDeathLocation = DeadPC->Pawn->K2_GetActorLocation();
		FDeathInfo& DeathInfo = DeadState->DeathInfo;

		DeathInfo.bDBNO = DeadPC->MyFortPawn->bWasDBNOOnDeath;
		DeathInfo.bInitialized = true;
		DeathInfo.DeathLocation = DeadPC->Pawn->K2_GetActorLocation();
		DeathInfo.DeathTags = DeathReport.Tags;
		DeathInfo.Downer = KillerState;
		DeathInfo.Distance = (KillerPawn ? KillerPawn->GetDistanceTo(DeadPC->Pawn) : ((AFortPlayerPawnAthena*)DeadPC->Pawn)->LastFallDistance);
		DeathInfo.FinisherOrDowner = KillerState;
		DeathInfo.DeathCause = DeadState->ToDeathCause(DeathInfo.DeathTags, DeathInfo.bDBNO);
		DeadState->OnRep_DeathInfo();
		DeadPC->RespawnPlayerAfterDeath(true);

		return ClientOnPawnDiedOG(DeadPC, DeathReport);
	}

	void Hook() {
		HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x122, ServerAcknowledgePossession, (LPVOID*)&ServerAcknowledgePossessionOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x67804ac), ServerReadyToStartMatch, (LPVOID*)&ServerReadyToStartMatchOG);

		HookVTable(UFortControllerComponent_Aircraft::GetDefaultObj(), 0x9e, ServerAttemptAircraftJump, nullptr);

		MH_CreateHook((LPVOID)(ImageBase + 0x6c26888), ClientOnPawnDied, (LPVOID*)&ClientOnPawnDiedOG);

		Log("PC Hooked!");
	}
}