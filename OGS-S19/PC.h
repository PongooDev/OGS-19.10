#pragma once
#include "framework.h"

namespace PC {
	inline void ServerAttemptAircraftJump(UFortControllerComponent_Aircraft* Comp, FRotator Rotation)
	{
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
	}

	void Hook() {
		HookVTable(UFortControllerComponent_Aircraft::GetDefaultObj(), 0x9e, ServerAttemptAircraftJump, nullptr);

		Log("PC Hooked!");
	}
}