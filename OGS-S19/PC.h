#pragma once
#include "framework.h"
#include "Looting.h"
#include "Vehicles.h"
#include "Bots.h"

namespace PC {
	void (*ServerAcknowledgePossessionOG)(AFortPlayerControllerAthena* PC, APawn* Pawn);
	inline void ServerAcknowledgePossession(AFortPlayerControllerAthena* PC, APawn* Pawn)
	{
		Log("ServerAck Called!");
		PC->AcknowledgedPawn = Pawn;

		return ServerAcknowledgePossessionOG(PC, Pawn);
	}

	inline void (*ServerLoadingScreenDroppedOG)(AFortPlayerControllerAthena* PC);
	inline void ServerLoadingScreenDropped(AFortPlayerControllerAthena* PC)
	{
		Log("ServerLoadingScreenDropped Called!");
		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
		AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		auto Pawn = (AFortPlayerPawn*)PC->Pawn;

		return ServerLoadingScreenDroppedOG(PC);
	}

	void (*ServerReadyToStartMatchOG)(AFortPlayerControllerAthena* PC);
	void ServerReadyToStartMatch(AFortPlayerControllerAthena* PC)
	{
		if (!PC) {
			return;
		}
		AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

		UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), AFortPlayerStartWarmup::StaticClass(), &PlayerStarts);
		
		static bool setupWorld = false;
		if (!setupWorld) {
			setupWorld = true;
			Looting::DestroyFloorLootSpawners();

			Vehicles::SpawnVehicles();
		}

		for (int i = 0; i < 99; i++) {
			if (PlayerStarts.Num() <= 0)
			{
				Log("NO PLAYER STARTS!!");
				continue;
			}
			AActor* SpawnLocator = PlayerStarts[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, PlayerStarts.Num() - 1)];

			static auto PhoebeSpawnerData = StaticLoadObject<UClass>("/Game/Athena/AI/Phoebe/BP_AISpawnerData_Phoebe.BP_AISpawnerData_Phoebe_C");
			if (SpawnLocator && PhoebeSpawnerData)
			{
				//Bots::SpawnPlayerBots(SpawnLocator);

				FTransform Transform{};
				Transform.Translation = SpawnLocator->K2_GetActorLocation();
				Transform.Rotation = FQuat();
				Transform.Scale3D = FVector{ 1,1,1 };

				((UAthenaAISystem*)UWorld::GetWorld()->AISystem)->AISpawner->RequestSpawn(UFortAthenaAIBotSpawnerData::CreateComponentListFromClass(PhoebeSpawnerData, UWorld::GetWorld()), Transform);
			}
			else {
				Log("not so great...");
			}
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
		Log("Pawn died");
		DeadPC->bMarkedAlive = false;

		auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
		auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		AFortPlayerStateAthena* DeadState = (AFortPlayerStateAthena*)DeadPC->PlayerState;
		AFortPlayerPawnAthena* KillerPawn = (AFortPlayerPawnAthena*)DeathReport.KillerPawn;
		AFortPlayerStateAthena* KillerState = (AFortPlayerStateAthena*)DeathReport.KillerPlayerState;

		static bool Won = false;

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

		if (!Won && DeadPC && DeadState)
		{
			if (KillerState && KillerState != DeadState)
			{
				KillerState->KillScore++;

				for (size_t i = 0; i < KillerState->PlayerTeam->TeamMembers.Num(); i++)
				{
					((AFortPlayerStateAthena*)KillerState->PlayerTeam->TeamMembers[i]->PlayerState)->TeamKillScore++;
					((AFortPlayerStateAthena*)KillerState->PlayerTeam->TeamMembers[i]->PlayerState)->OnRep_TeamKillScore();
				}

				KillerState->ClientReportKill(DeadState);
				KillerState->OnRep_Kills();
				
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
			}

			if (Won || !GameState->IsRespawningAllowed(DeadState))
			{
				FAthenaRewardResult Result;
				UFortPlayerControllerAthenaXPComponent* XPComponent = DeadPC->XPComponent;
				Result.TotalBookXpGained = XPComponent->TotalXpEarned;
				Result.TotalSeasonXpGained = XPComponent->TotalXpEarned;

				DeadPC->ClientSendEndBattleRoyaleMatchForPlayer(true, Result);

				FAthenaMatchStats Stats;
				FAthenaMatchTeamStats TeamStats;

				if (DeadState)
				{
					DeadState->Place = GameMode->AliveBots.Num() + GameMode->AlivePlayers.Num();
					DeadState->OnRep_Place();
				}

				for (size_t i = 0; i < 20; i++)
				{
					Stats.Stats[i] = 0;
				}

				Stats.Stats[3] = DeadState->KillScore;

				TeamStats.Place = DeadState->Place;
				TeamStats.TotalPlayers = GameState->TotalPlayers;

				DeadPC->ClientSendMatchStatsForPlayer(Stats);
				DeadPC->ClientSendTeamStatsForPlayer(TeamStats);
				FDeathInfo& DeathInfo = DeadState->DeathInfo;

				//RemoveFromAlivePlayers(GameMode, DeadPC, (KillerState == DeadState ? nullptr : KillerState), KillerPawn, DeathReport.KillerWeapon ? DeathReport.KillerWeapon : nullptr, DeadState ? DeathInfo.DeathCause : EDeathCause::Rifle, 0);

				if (KillerState)
				{
					if (KillerState->Place == 1)
					{
						if (DeathReport.KillerWeapon)
						{
							((AFortPlayerControllerAthena*)KillerState->Owner)->PlayWinEffects(KillerPawn, DeathReport.KillerWeapon, EDeathCause::Rifle, false);
							((AFortPlayerControllerAthena*)KillerState->Owner)->ClientNotifyWon(KillerPawn, DeathReport.KillerWeapon, EDeathCause::Rifle);
						}

						FAthenaRewardResult Result;
						AFortPlayerControllerAthena* KillerPC = (AFortPlayerControllerAthena*)KillerState->GetOwner();
						KillerPC->ClientSendEndBattleRoyaleMatchForPlayer(true, Result);

						FAthenaMatchStats Stats;
						FAthenaMatchTeamStats TeamStats;

						for (size_t i = 0; i < 20; i++)
						{
							Stats.Stats[i] = 0;
						}

						Stats.Stats[3] = KillerState->KillScore;

						TeamStats.Place = 1;
						TeamStats.TotalPlayers = GameState->TotalPlayers;

						KillerPC->ClientSendMatchStatsForPlayer(Stats);
						KillerPC->ClientSendTeamStatsForPlayer(TeamStats);

						GameState->WinningPlayerState = KillerState;
						GameState->WinningTeam = KillerState->TeamIndex;
						GameState->OnRep_WinningPlayerState();
						GameState->OnRep_WinningTeam();
					}
				}
			}
		}

		return ClientOnPawnDiedOG(DeadPC, DeathReport);
	}

	static void ServerSendZiplineState(AFortPlayerPawnAthena* Pawn, FZiplinePawnState State)
	{
		Log("ServerSendZiplineState Called!");
		if (!Pawn)
			return;

		if (State.bIsZiplining)
			Pawn->ZiplineState = State;

		if (State.bJumped)
		{
			auto Velocity = Pawn->CharacterMovement->Velocity;
			auto VelocityX = Velocity.X * -0.5f;
			auto VelocityY = Velocity.Y * -0.5f;
			Pawn->LaunchCharacterJump({ VelocityX >= -750 ? fminf(VelocityX, 750) : -750, VelocityY >= -750 ? fminf(VelocityY, 750) : -750, 1200 }, false, false, true, true);
		}
	}

	inline void ServerCreateBuildingActor(AFortPlayerControllerAthena* PC, FCreateBuildingActorData CreateBuildingData)
	{
		Log("ServerCreateBuildingActor Called!");
		if (!PC || PC->IsInAircraft())
			return;

		UClass* BuildingClass = PC->BroadcastRemoteClientInfo->RemoteBuildableClass.Get();
		TArray<AActor*> BuildingsToRemove;

		auto ResDef = UFortKismetLibrary::GetDefaultObj()->K2_GetResourceItemDefinition(((ABuildingSMActor*)BuildingClass->DefaultObject)->ResourceType);
		Inventory::RemoveItem(PC, ResDef, 10);

		ABuildingSMActor* NewBuilding = SpawnActor<ABuildingSMActor>(CreateBuildingData.BuildLoc, CreateBuildingData.BuildRot, PC, BuildingClass);

		NewBuilding->bPlayerPlaced = true;
		NewBuilding->InitializeKismetSpawnedBuildingActor(NewBuilding, PC, true, nullptr);
		NewBuilding->TeamIndex = ((AFortPlayerStateAthena*)PC->PlayerState)->TeamIndex;
		NewBuilding->Team = EFortTeam(NewBuilding->TeamIndex);

		for (size_t i = 0; i < BuildingsToRemove.Num(); i++)
		{
			BuildingsToRemove[i]->K2_DestroyActor();
		}
		UKismetArrayLibrary::Array_Clear((TArray<int32>&)BuildingsToRemove);
	}

	void ServerPlayEmoteItem(AFortPlayerControllerAthena* PC, UFortMontageItemDefinitionBase* EmoteAsset, float EmoteRandomNumber) {
		Log("ServerPlayEmoteItem Called!");

		if (EmoteAsset->IsA(UAthenaDanceItemDefinition::StaticClass()))
		{
			if (auto Pawn = PC->MyFortPawn)
			{
				if (auto DanceItemDefinition = reinterpret_cast<UAthenaDanceItemDefinition*>(EmoteAsset))
				{
					Pawn->bMovingEmote = DanceItemDefinition->bMovingEmote;
					Pawn->bMovingEmoteForwardOnly = DanceItemDefinition->bMoveForwardOnly;
					Pawn->EmoteWalkSpeed = DanceItemDefinition->WalkForwardSpeed;
				}
			}
		}

		static UClass* EmoteAbilityClass = StaticLoadObject<UClass>("/Game/Abilities/Emotes/GAB_Emote_Generic.GAB_Emote_Generic_C");

		FGameplayAbilitySpec Spec{};
		AbilitySpecConstructor(&Spec, reinterpret_cast<UGameplayAbility*>(EmoteAbilityClass->DefaultObject), 1, -1, EmoteAsset);
		GiveAbilityAndActivateOnce(reinterpret_cast<AFortPlayerStateAthena*>(PC->PlayerState)->AbilitySystemComponent, &Spec.Handle, Spec);
	}

	void ServerPlaySquadQuickChatMessage(AFortPlayerControllerAthena* PC, FAthenaQuickChatActiveEntry ChatEntry, __int64) {
		Log("ServerPlaySquadQuickChatMessage Called!");

		static ETeamMemberState MemberStates[10] = {
			ETeamMemberState::ChatBubble,
			ETeamMemberState::EnemySpotted,
			ETeamMemberState::NeedMaterials,
			ETeamMemberState::NeedBandages,
			ETeamMemberState::NeedShields,
			ETeamMemberState::NeedAmmoHeavy,
			ETeamMemberState::NeedAmmoLight,
			ETeamMemberState::FIRST_CHAT_MESSAGE,
			ETeamMemberState::NeedAmmoMedium,
			ETeamMemberState::NeedAmmoShells
		};

		auto PlayerState = reinterpret_cast<AFortPlayerStateAthena*>(PC->PlayerState);

		PlayerState->ReplicatedTeamMemberState = MemberStates[ChatEntry.Index];

		PlayerState->OnRep_ReplicatedTeamMemberState();

		static auto EmojiComm = StaticFindObject<UAthenaEmojiItemDefinition>(L"/Game/Athena/Items/Cosmetics/Dances/Emoji/Emoji_Comm.Emoji_Comm");

		if (EmojiComm)
		{
			PC->ServerPlayEmoteItem(EmojiComm, 0);
		}
	}

	void ServerReturnToMainMenu(AFortPlayerControllerAthena* PC)
	{
		PC->ClientReturnToMainMenu(L"Thanks For Playing OGS-S19!");
	}

	void Hook() {
		HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x122, ServerAcknowledgePossession, (LPVOID*)&ServerAcknowledgePossessionOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x1547ff0), ServerLoadingScreenDropped, (LPVOID*)&ServerLoadingScreenDroppedOG);

		MH_CreateHook((LPVOID)(ImageBase + 0x67804ac), ServerReadyToStartMatch, (LPVOID*)&ServerReadyToStartMatchOG);

		HookVTable(UFortControllerComponent_Aircraft::GetDefaultObj(), 0x9e, ServerAttemptAircraftJump, nullptr);

		MH_CreateHook((LPVOID)(ImageBase + 0x6c26888), ClientOnPawnDied, (LPVOID*)&ClientOnPawnDiedOG);

		//MH_CreateHook((LPVOID)(ImageBase + 0x673363c), ServerSendZiplineState, nullptr);
		//HookVTable(AFortPlayerPawnAthena::StaticClass(), 558, ServerSendZiplineState, nullptr);

		HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 589, ServerCreateBuildingActor, nullptr);

		//HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 487, ServerPlayEmoteItem, nullptr);

		HookVTable(AFortPlayerControllerAthena::StaticClass(), 647, ServerReturnToMainMenu, nullptr);

		// Offsets:
		// OnCapsuleBeginOverlap: 0x115A604

		Log("PC Hooked!");
	}
}