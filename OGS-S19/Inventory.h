#pragma once
#include "framework.h"

// thx timeless for most of the stuff here!
namespace Inventory {
	bool CompareGuids(FGuid Guid1, FGuid Guid2) {
		if (Guid1.A == Guid2.A
			&& Guid1.B == Guid2.B
			&& Guid1.C == Guid2.C
			&& Guid1.D == Guid2.D) {
			return true;
		}
		else {
			return false;
		}
	}

	inline void GiveItem(AFortPlayerController* PC, UFortItemDefinition* Def, int Count, int LoadedAmmo)
	{
		UFortWorldItem* Item = Cast<UFortWorldItem>(Def->CreateTemporaryItemInstanceBP(Count, 0));
		Item->SetOwningControllerForTemporaryItem(PC);
		Item->OwnerInventory = PC->WorldInventory;
		Item->ItemEntry.LoadedAmmo = LoadedAmmo;

		PC->WorldInventory->Inventory.ReplicatedEntries.Add(Item->ItemEntry);
		PC->WorldInventory->Inventory.ItemInstances.Add(Item);
		PC->WorldInventory->Inventory.MarkItemDirty(Item->ItemEntry);
		PC->WorldInventory->HandleInventoryLocalUpdate();
	}

	inline void UpdateInventory(AFortPlayerController* PC, FFortItemEntry& Entry)
	{
		for (size_t i = 0; i < PC->WorldInventory->Inventory.ItemInstances.Num(); i++)
		{
			if (CompareGuids(PC->WorldInventory->Inventory.ItemInstances[i]->ItemEntry.ItemGuid, Entry.ItemGuid))
			{
				PC->WorldInventory->Inventory.ItemInstances[i]->ItemEntry = Entry;
				PC->WorldInventory->Inventory.MarkItemDirty(PC->WorldInventory->Inventory.ReplicatedEntries[i]);
				break;
			}
		}
	}

	void RemoveItem(AFortPlayerController* PC, UFortItemDefinition* Def, int Count)
	{
		bool Remove = false;
		FGuid guid;
		for (size_t i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			auto& Entry = PC->WorldInventory->Inventory.ReplicatedEntries[i];
			if (Entry.ItemDefinition == Def)
			{
				Entry.Count -= Count;
				if (Entry.Count <= 0)
				{
					PC->WorldInventory->Inventory.ReplicatedEntries[i].StateValues.Free();
					PC->WorldInventory->Inventory.ReplicatedEntries.RemoveSingle(i);
					Remove = true;
					guid = Entry.ItemGuid;
				}
				else
				{
					PC->WorldInventory->Inventory.MarkItemDirty(PC->WorldInventory->Inventory.ReplicatedEntries[i]);
					UpdateInventory(PC, Entry);
					PC->WorldInventory->HandleInventoryLocalUpdate();
					return;
				}
				break;
			}
		}

		if (Remove)
		{
			for (size_t i = 0; i < PC->WorldInventory->Inventory.ItemInstances.Num(); i++)
			{
				if (CompareGuids(PC->WorldInventory->Inventory.ItemInstances[i]->GetItemGuid(), guid))
				{
					PC->WorldInventory->Inventory.ItemInstances.RemoveSingle(i);
					break;
				}
			}
		}

		PC->WorldInventory->Inventory.MarkArrayDirty();
		PC->WorldInventory->HandleInventoryLocalUpdate();
	}

	inline void RemoveItem(AFortPlayerController* PC, FGuid Guid, int Count)
	{
		for (auto& Entry : PC->WorldInventory->Inventory.ReplicatedEntries)
		{
			if (CompareGuids(Guid, Entry.ItemGuid))
			{
				RemoveItem(PC, Entry.ItemDefinition, Count);
				break;
			}
		}
	}

	static void ServerExecuteInventoryItem(AFortPlayerControllerAthena* PC, FGuid Guid)
	{
		Log("ServerExecuteInventoryItem Called!");
		if (!PC || !PC->MyFortPawn)
			return;

		for (size_t i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			if (!PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition)
				continue;

			FGuid ItemGuid = PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemGuid;
			if (CompareGuids(Guid, ItemGuid)) {
				UFortWeaponItemDefinition* ItemDef = (UFortWeaponItemDefinition*)PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition;
				FGuid TrackerGuid = PC->WorldInventory->Inventory.ReplicatedEntries[i].TrackerGuid;

				PC->MyFortPawn->EquipWeaponDefinition(ItemDef, Guid, TrackerGuid, false);
			}
		}

		return;
	}

	void Hook() {
		HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x22c, ServerExecuteInventoryItem, nullptr);

		Log("Inventory Hooked!");
	}
}