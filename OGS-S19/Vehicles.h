#pragma once
#include "framework.h"

namespace Vehicles {
	static void SpawnVehicles()
	{
		auto GameState = AFortGameStateAthena::GetDefaultObj();
		auto Spawners = GetAllObjectsOfClass<AAthena_VehicleSpawner_C>();
		xmap<UClass*, int> EvaledVehicles;
		xmap<UClass*, int> InoperableVehicles;
		xmap<UClass*, int> VehicleCounts;
		xmap<UClass*, TArray<AAthena_VehicleSpawner_C*>> Vehicles;

		for (auto& Spawner : Spawners)
		{
			auto VC = Spawner->GetVehicleClass();
			if (VehicleCounts[VC])
				VehicleCounts[VC]++;
			else
				VehicleCounts[VC] = 1;
		}

		for (auto& Spawner : Spawners)
		{
			auto VC = Spawner->GetVehicleClass();
			Vehicles[VC].Add(Spawner);
		}

		for (auto& [VC, VehicleArray] : Vehicles)
		{
			for (auto& vehicle : VehicleArray)
			{
				auto Vehicle = SpawnActor<AFortAthenaVehicle>(vehicle->K2_GetActorLocation(), vehicle->K2_GetActorRotation(), nullptr, VC);
				vehicle->SpawnedVehicle = Vehicle;
			}
		}
		Spawners.clear();
		Log("Spawned vehicles");
	}
}