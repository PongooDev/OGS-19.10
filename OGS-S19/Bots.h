#pragma once
#include "framework.h"
#include "Globals.h"
#include "Inventory.h"
#include "BotNames.h"

namespace Bots {
    static std::vector<UAthenaCharacterItemDefinition*> CIDs{};
    static std::vector<UAthenaPickaxeItemDefinition*> Pickaxes{};
    static std::vector<UAthenaBackpackItemDefinition*> Backpacks{};
    static std::vector<UAthenaGliderItemDefinition*> Gliders{};
    static std::vector<UAthenaSkyDiveContrailItemDefinition*> Contrails{};
    inline std::vector<UAthenaDanceItemDefinition*> Dances{};

    enum EBotState : uint8 {
        Warmup,
        PreBus, // Dont handle this, just there to stop bots from killing eachother before bus enters dropzone
        Bus,
        Skydiving,
        Gliding,
        Landed,
        Looting,
        LookingForPlayers
    };

    struct PlayerBot
    {
    public:
        uint64_t tick_counter = 0;
        AFortPlayerPawnAthena* Pawn = nullptr;
        AFortAthenaAIBotController* PC = nullptr;
        AFortPlayerStateAthena* PlayerState = nullptr;
        FString DisplayName = L"Bot";
        EBotState BotState = EBotState::Warmup;
        bool bIsDead = false;
        bool bHasThankedBusDriver = false;
        bool bIsMoving = false;
        bool bIsCurrentlyStrafing = false;
        bool bIsCrouching = false;
        bool bPickaxeEquiped = true;
        AActor* TargetActor = nullptr;
        float TimeToNextAction = 0.f;
        float LikelyHoodToDoAction = 0.f;

    public:
        void LookAt(AActor* Actor)
        {
            if (!Pawn || PC->GetFocusActor() == Actor)
                return;

            if (!Actor)
            {
                PC->K2_ClearFocus();
                return;
            }

            PC->K2_SetFocus(Actor);
        }

        // took these funcs from ero since they would basically be the same anyway
        void GiveItemBot(UFortItemDefinition* Def, int Count = 1, int LoadedAmmo = 0)
        {
            UFortWorldItem* Item = (UFortWorldItem*)Def->CreateTemporaryItemInstanceBP(Count, 0);
            Item->OwnerInventory = PC->Inventory;
            Item->ItemEntry.LoadedAmmo = LoadedAmmo;
            PC->Inventory->Inventory.ReplicatedEntries.Add(Item->ItemEntry);
            PC->Inventory->Inventory.ItemInstances.Add(Item);
            PC->Inventory->Inventory.MarkItemDirty(Item->ItemEntry);
            PC->Inventory->HandleInventoryLocalUpdate();
        }

        FFortItemEntry* GetEntry(UFortItemDefinition* Def)
        {
            for (size_t i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++)
            {
                if (PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition == Def)
                    return &PC->Inventory->Inventory.ReplicatedEntries[i];
            }

            return nullptr;
        }

        ABuildingSMActor* FindNearestBuildingSMActor()
        {
            static TArray<AActor*> Array;
            static bool PopulatedArray = false;
            if (!PopulatedArray)
            {
                PopulatedArray = true;
                UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), ABuildingSMActor::StaticClass(), &Array);
            }

            AActor* NearestPoi = nullptr;

            for (size_t i = 0; i < Array.Num(); i++)
            {
                if (!NearestPoi || (((ABuildingSMActor*)NearestPoi)->GetHealth() < 1500 && ((ABuildingSMActor*)NearestPoi)->GetHealth() > 1 && Array[i]->GetDistanceTo(Pawn) < NearestPoi->GetDistanceTo(Pawn)))
                {
                    NearestPoi = Array[i];
                }
            }

            return (ABuildingSMActor*)NearestPoi;
        }

        ABuildingActor* FindNearestChest()
        {
            // ill make support for finding the closest chest and rare chest if its even in this gs
            static auto ChestClass = StaticLoadObject<UClass>("/Game/Building/ActorBlueprints/Containers/Tiered_Chest_Athena.Tiered_Chest_Athena_C");
            static auto FactionChestClass = StaticLoadObject<UClass>("/Game/Building/ActorBlueprints/Containers/Tiered_Chest_Athena_FactionChest.Tiered_Chest_Athena_FactionChest_C");
            TArray<AActor*> Array;
            TArray<AActor*> FactionChests;
            UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), ChestClass, &Array);
            UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), FactionChestClass, &FactionChests);

            AActor* NearestChest = nullptr;

            for (size_t i = 0; i < Array.Num(); i++)
            {
                AActor* Chest = Array[i];
                if (Chest->bHidden)
                    continue;

                if (!NearestChest || Chest->GetDistanceTo(Pawn) < NearestChest->GetDistanceTo(Pawn))
                {
                    NearestChest = Array[i];
                }
            }

            for (size_t i = 0; i < FactionChests.Num(); i++)
            {
                AActor* Chest = FactionChests[i];
                if (Chest->bHidden)
                    continue;

                if (!NearestChest || Chest->GetDistanceTo(Pawn) < NearestChest->GetDistanceTo(Pawn))
                {
                    NearestChest = FactionChests[i];
                }
            }
            Array.Free();
            FactionChests.Free();
            return (ABuildingActor*)NearestChest;
        }

        AFortPickupAthena* FindNearestPickup()
        {
            static auto PickupClass = AFortPickupAthena::StaticClass();
            TArray<AActor*> Array;
            UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), PickupClass, &Array);
            AActor* NearestPickup = nullptr;

            for (size_t i = 0; i < Array.Num(); i++)
            {
                if (Array[i]->bHidden)
                    continue;

                if (!NearestPickup || Array[i]->GetDistanceTo(Pawn) < NearestPickup->GetDistanceTo(Pawn))
                {
                    NearestPickup = Array[i];
                }
            }

            Array.Free();
            return (AFortPickupAthena*)NearestPickup;
        }

        bool GetNearestLootable() {
            static auto ChestClass = StaticLoadObject<UClass>("/Game/Building/ActorBlueprints/Containers/Tiered_Chest_Athena.Tiered_Chest_Athena_C");
            TArray<AActor*> ChestArray;
            UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), ChestClass, &ChestArray);

            static auto PickupClass = AFortPickupAthena::StaticClass();
            TArray<AActor*> PickupArray;
            UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), PickupClass, &PickupArray);

            AActor* NearestPickup = nullptr;
            AActor* NearestChest = nullptr;

            for (size_t i = 0; i < ChestArray.Num(); i++)
            {
                AActor* Chest = ChestArray[i];
                if (Chest->bHidden)
                    continue;

                if (!NearestChest || Chest->GetDistanceTo(Pawn) < NearestChest->GetDistanceTo(Pawn))
                {
                    NearestChest = ChestArray[i];
                }
            }

            for (size_t i = 0; i < PickupArray.Num(); i++)
            {
                if (PickupArray[i]->bHidden)
                    continue;

                if (!NearestPickup || PickupArray[i]->GetDistanceTo(Pawn) < NearestPickup->GetDistanceTo(Pawn))
                {
                    NearestPickup = PickupArray[i];
                }
            }

            ChestArray.Free();
            PickupArray.Free();
            return NearestPickup->GetDistanceTo(Pawn) > NearestChest->GetDistanceTo(Pawn);
        }

        FVector FindNearestPlayerOrBot() {
            auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;

            AActor* NearestPlayer = nullptr;

            for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
            {
                if (!NearestPlayer || (GameMode->AlivePlayers[i]->Pawn && GameMode->AlivePlayers[i]->Pawn->GetDistanceTo(Pawn) < NearestPlayer->GetDistanceTo(Pawn)))
                {
                    NearestPlayer = GameMode->AlivePlayers[i]->Pawn;
                }
            }

            for (size_t i = 0; i < GameMode->AliveBots.Num(); i++)
            {
                if (GameMode->AliveBots[i]->Pawn != Pawn)
                {
                    if (!NearestPlayer || (GameMode->AliveBots[i]->Pawn && GameMode->AliveBots[i]->Pawn->GetDistanceTo(Pawn) < NearestPlayer->GetDistanceTo(Pawn)))
                    {
                        NearestPlayer = GameMode->AliveBots[i]->Pawn;
                    }
                }
            }

            return NearestPlayer->K2_GetActorLocation();
        }

        AActor* GetNearestPlayerActor() {
            auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;

            AActor* NearestPlayer = nullptr;

            for (size_t i = 0; i < GameMode->AlivePlayers.Num(); i++)
            {
                if (!NearestPlayer || (GameMode->AlivePlayers[i]->Pawn && GameMode->AlivePlayers[i]->Pawn->GetDistanceTo(Pawn) < NearestPlayer->GetDistanceTo(Pawn)))
                {
                    NearestPlayer = GameMode->AlivePlayers[i]->Pawn;
                }
            }

            for (size_t i = 0; i < GameMode->AliveBots.Num(); i++)
            {
                if (GameMode->AliveBots[i]->Pawn != Pawn)
                {
                    if (!NearestPlayer || (GameMode->AliveBots[i]->Pawn && GameMode->AliveBots[i]->Pawn->GetDistanceTo(Pawn) < NearestPlayer->GetDistanceTo(Pawn)))
                    {
                        NearestPlayer = GameMode->AliveBots[i]->Pawn;
                    }
                }
            }

            return NearestPlayer;
        }

        bool IsInventoryFull()
        {
            int ItemNb = 0;
            auto InstancesPtr = &PC->Inventory->Inventory.ItemInstances;
            for (int i = 0; i < InstancesPtr->Num(); i++)
            {
                if (InstancesPtr->operator[](i))
                {
                    if (Inventory::GetQuickBars(InstancesPtr->operator[](i)->ItemEntry.ItemDefinition) == EFortQuickBars::Primary)
                    {
                        ItemNb++;

                        if (ItemNb >= 5)
                        {
                            break;
                        }
                    }
                }
            }

            return ItemNb >= 5;
        }

        void Pickup(AFortPickup* Pickup) {
            GiveItemBot(Pickup->PrimaryPickupItemEntry.ItemDefinition, Pickup->PrimaryPickupItemEntry.Count, Pickup->PrimaryPickupItemEntry.LoadedAmmo);
            if (((UFortWeaponItemDefinition*)Pickup->PrimaryPickupItemEntry.ItemDefinition)->GetAmmoWorldItemDefinition_BP() && ((UFortWeaponItemDefinition*)Pickup->PrimaryPickupItemEntry.ItemDefinition)->GetAmmoWorldItemDefinition_BP() != Pickup->PrimaryPickupItemEntry.ItemDefinition)
            {
                GiveItemBot(((UFortWeaponItemDefinition*)Pickup->PrimaryPickupItemEntry.ItemDefinition)->GetAmmoWorldItemDefinition_BP(), 60);
            }

            Pickup->PickupLocationData.bPlayPickupSound = true;
            Pickup->PickupLocationData.FlyTime = 0.3f;
            Pickup->PickupLocationData.ItemOwner = Pawn;
            Pickup->PickupLocationData.PickupGuid = Pickup->PrimaryPickupItemEntry.ItemGuid;
            Pickup->PickupLocationData.PickupTarget = Pawn;
            Pickup->OnRep_PickupLocationData();

            Pickup->bPickedUp = true;
            Pickup->OnRep_bPickedUp();
        }

        void PickupAllItemsInRange(float Range = 320.f) {
            static auto PickupClass = AFortPickupAthena::StaticClass();
            TArray<AActor*> Array;
            UGameplayStatics::GetDefaultObj()->GetAllActorsOfClass(UWorld::GetWorld(), PickupClass, &Array);

            for (size_t i = 0; i < Array.Num(); i++)
            {
                if (Array[i]->GetDistanceTo(Pawn) < Range)
                {
                    Pickup((AFortPickupAthena*)Array[i]);
                }
            }

            Array.Free();
        }

        void EquipPickaxe()
        {
            if (!Pawn || !Pawn->CurrentWeapon)
                return;

            if (!Pawn->CurrentWeapon->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
            {
                for (size_t i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++)
                {
                    if (PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
                    {
                        Pawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)PC->Inventory->Inventory.ReplicatedEntries[i].ItemDefinition,
                            PC->Inventory->Inventory.ReplicatedEntries[i].ItemGuid, PC->Inventory->Inventory.ReplicatedEntries[i].TrackerGuid, false);
                        break;
                    }
                }
            }
        }

        bool IsPickaxeEquiped() {
            if (!Pawn || !Pawn->CurrentWeapon)
                return false;

            if (Pawn->CurrentWeapon->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
            {
                return true;
            }
            return false;
        }

        bool HasGun()
        {
            for (size_t i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++)
            {
                auto& Entry = PC->Inventory->Inventory.ReplicatedEntries[i];
                if (Entry.ItemDefinition) {
                    std::string ItemName = Entry.ItemDefinition->Name.ToString();
                    if (ItemName.contains("Shotgun") || ItemName.contains("SMG") || ItemName.contains("Assault")
                        || ItemName.contains("Grenade") || ItemName.contains("Sniper") || ItemName.contains("Rocket") || ItemName.contains("Pistol")) {
                        return true;
                        break;
                    }
                }
            }
            return false;
        }

        void SimpleSwitchToWeapon() {
            if (!HasGun()) {
                return;
            }

            if (!Pawn || !Pawn->CurrentWeapon || !Pawn->CurrentWeapon->WeaponData || !PC || !PC->Inventory || bIsDead)
                return;

            if (!Pawn->CurrentWeapon->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass())) {
                return;
            }

            if (Pawn->CurrentWeapon->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
            {
                for (size_t i = 0; i < PC->Inventory->Inventory.ReplicatedEntries.Num(); i++)
                {
                    auto& Entry = PC->Inventory->Inventory.ReplicatedEntries[i];
                    if (Entry.ItemDefinition) {
                        std::string ItemName = Entry.ItemDefinition->Name.ToString();
                        if (ItemName.contains("Shotgun") || ItemName.contains("SMG") || ItemName.contains("Assault")
                            || ItemName.contains("Grenade") || ItemName.contains("Sniper") || ItemName.contains("Rocket") || ItemName.contains("Pistol")) {
                            Pawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)Entry.ItemDefinition, Entry.ItemGuid, Entry.TrackerGuid, false);
                            break;
                        }
                    }
                }
            }
        }
    };
    std::vector<class PlayerBot*> PlayerBotArray{};

    void FreeDeadBots() {
        for (size_t i = 0; i < PlayerBotArray.size();)
        {
            if (PlayerBotArray[i]->bIsDead) {
                delete PlayerBotArray[i];
                PlayerBotArray.erase(PlayerBotArray.begin() + i);
                Log("Freed a dead bot from the array!");
            }
            else {
                ++i;
            }
        }
    }

    void InitPlayerBot(AFortPlayerPawnAthena* Pawn, AFortAthenaAIBotController* PC, AFortPlayerStateAthena* PlayerState) {
        static auto BotBP = StaticLoadObject<UClass>("/Game/Athena/AI/Phoebe/BP_PlayerPawn_Athena_Phoebe.BP_PlayerPawn_Athena_Phoebe_C");
        static UBehaviorTree* BehaviorTree = StaticLoadObject<UBehaviorTree>("/Game/Athena/AI/Phoebe/BehaviorTrees/BT_Phoebe.BT_Phoebe");

        if (!BotBP || !BehaviorTree)
            return;

        PlayerBot* bot = new PlayerBot{};

        AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;

        bot->Pawn = Pawn;

        bot->PC = PC;
        bot->PlayerState = PlayerState;

        if (!bot->Pawn || !bot->PC || !bot->PlayerState)
            return;

        if (!CIDs.empty() && UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(1)) { // use the random bool so that some bots will be defaults (more realistic)
            // as you could probably tell, i did not write this!
            auto CID = CIDs[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, CIDs.size() - 1)];
            if (CID->HeroDefinition)
            {
                if (CID->HeroDefinition->Specializations.IsValid())
                {
                    for (size_t i = 0; i < CID->HeroDefinition->Specializations.Num(); i++)
                    {
                        UFortHeroSpecialization* Spec = StaticLoadObject<UFortHeroSpecialization>(UKismetStringLibrary::GetDefaultObj()->Conv_NameToString(CID->HeroDefinition->Specializations[i].ObjectID.AssetPathName).ToString());
                        if (Spec)
                        {
                            for (size_t j = 0; j < Spec->CharacterParts.Num(); j++)
                            {
                                UCustomCharacterPart* Part = StaticLoadObject<UCustomCharacterPart>(UKismetStringLibrary::GetDefaultObj()->Conv_NameToString(Spec->CharacterParts[j].ObjectID.AssetPathName).ToString());
                                if (Part)
                                {
                                    bot->PlayerState->CharacterData.Parts[(uintptr_t)Part->CharacterPartType] = Part;
                                }
                            }
                        }
                    }
                }
            }
            bot->PC->CosmeticLoadoutBC.Character = CID;
        }

        if (!Backpacks.empty() && UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.5)) { // less likely to equip than skin cause lots of ppl prefer not to use backpack
            auto Backpack = Backpacks[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, Backpacks.size() - 1)];
            for (size_t j = 0; j < Backpack->CharacterParts.Num(); j++)
            {
                UCustomCharacterPart* Part = Backpack->CharacterParts[j];
                if (Part)
                {
                    bot->PlayerState->CharacterData.Parts[(uintptr_t)Part->CharacterPartType] = Part;
                }
            }
        }
        if (!Gliders.empty()) {
            auto Glider = Gliders[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, Gliders.size() - 1)];
            bot->PC->CosmeticLoadoutBC.Glider = Glider;
        }
        if (!Contrails.empty() && UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.95)) {
            auto Contrail = Contrails[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, Contrails.size() - 1)];
            bot->PC->CosmeticLoadoutBC.SkyDiveContrail = Contrail;
        }

        for (size_t i = 0; i < Dances.size(); i++)
        {
            bot->PC->CosmeticLoadoutBC.Dances.Add(Dances.at(i));
        }

        bot->Pawn->CosmeticLoadout = bot->PC->CosmeticLoadoutBC;
        bot->Pawn->ApplyCosmeticLoadout();
        bot->Pawn->OnRep_BaseCosmeticLoadout();
        bot->PlayerState->OnRep_CharacterData();

        if (!bot->PC->Inventory)
            bot->PC->Inventory = SpawnActor<AFortInventory>({}, {}, bot->PC);

        if (Pickaxes.empty()) {
            Log("Pickaxes array is empty!");
            UFortWeaponMeleeItemDefinition* PickDef = StaticLoadObject<UFortWeaponMeleeItemDefinition>("/Game/Athena/Items/Weapons/WID_Harvest_Pickaxe_Athena_C_T01.WID_Harvest_Pickaxe_Athena_C_T01");
            if (PickDef) {
                bot->GiveItemBot(PickDef);
                auto Entry = bot->GetEntry(PickDef);
                bot->Pawn->EquipWeaponDefinition((UFortWeaponMeleeItemDefinition*)Entry->ItemDefinition, Entry->ItemGuid, Entry->TrackerGuid, false);
            }
            else {
                Log("Default Pickaxe dont exist!");
            }
        }
        else {
            auto PickDef = Pickaxes[UKismetMathLibrary::GetDefaultObj()->RandomIntegerInRange(0, Pickaxes.size() - 1)];
            if (!PickDef)
            {
                Log("Cooked!");
                UFortWeaponMeleeItemDefinition* PickDef = StaticLoadObject<UFortWeaponMeleeItemDefinition>("/Game/Athena/Items/Weapons/WID_Harvest_Pickaxe_Athena_C_T01.WID_Harvest_Pickaxe_Athena_C_T01");
                if (PickDef) {
                    bot->GiveItemBot(PickDef);
                    auto Entry = bot->GetEntry(PickDef);
                    bot->Pawn->EquipWeaponDefinition((UFortWeaponMeleeItemDefinition*)Entry->ItemDefinition, Entry->ItemGuid, Entry->TrackerGuid, false);
                }
                else {
                    Log("Default Pickaxe dont exist!");
                }
            }
            else {
                if (PickDef && PickDef->WeaponDefinition)
                {
                    bot->GiveItemBot(PickDef->WeaponDefinition);
                }

                auto Entry = bot->GetEntry(PickDef->WeaponDefinition);
                bot->Pawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)Entry->ItemDefinition, Entry->ItemGuid, Entry->TrackerGuid, false);
            }
        }

        if (BotDisplayNames.size() != 0) {
            std::srand(static_cast<unsigned int>(std::time(0)));
            int randomIndex = std::rand() % BotDisplayNames.size();
            std::string rdName = BotDisplayNames[randomIndex];
            BotDisplayNames.erase(BotDisplayNames.begin() + randomIndex);

            int size_needed = MultiByteToWideChar(CP_UTF8, 0, rdName.c_str(), (int)rdName.size(), NULL, 0);
            std::wstring wideString(size_needed, 0);
            MultiByteToWideChar(CP_UTF8, 0, rdName.c_str(), (int)rdName.size(), &wideString[0], size_needed);


            FString CVName = FString(wideString.c_str());
            GameMode->ChangeName(bot->PC, CVName, true);
            bot->DisplayName = CVName;


            bot->PlayerState->OnRep_PlayerName();
        }

        static UBlackboardData* BlackboardData = StaticLoadObject<UBlackboardData>("/Game/Athena/AI/Phoebe/BehaviorTrees/BB_Phoebe.BB_Phoebe");

        for (auto SkillSet : bot->PC->BotSkillSetClasses) // are they even needed
        {
            if (!SkillSet)
                continue;

            if (auto AimingSkill = Cast<UFortAthenaAIBotAimingDigestedSkillSet>(SkillSet))
                bot->PC->CacheAimingDigestedSkillSet = AimingSkill;

            if (auto HarvestSkill = Cast<UFortAthenaAIBotHarvestDigestedSkillSet>(SkillSet))
                bot->PC->CacheHarvestDigestedSkillSet = HarvestSkill;

            if (auto InventorySkill = Cast<UFortAthenaAIBotInventoryDigestedSkillSet>(SkillSet))
                bot->PC->CacheInventoryDigestedSkillSet = InventorySkill;

            if (auto LootingSkill = Cast<UFortAthenaAIBotLootingDigestedSkillSet>(SkillSet))
                bot->PC->CacheLootingSkillSet = LootingSkill;

            if (auto MovementSkill = Cast<UFortAthenaAIBotMovementDigestedSkillSet>(SkillSet))
                bot->PC->CacheMovementSkillSet = MovementSkill;

            if (auto PerceptionSkill = Cast<UFortAthenaAIBotPerceptionDigestedSkillSet>(SkillSet))
                bot->PC->CachePerceptionDigestedSkillSet = PerceptionSkill;

            if (auto PlayStyleSkill = Cast<UFortAthenaAIBotPlayStyleDigestedSkillSet>(SkillSet))
                bot->PC->CachePlayStyleSkillSet = PlayStyleSkill;
        }

        bot->PC->BehaviorTree = BehaviorTree;
        bot->PC->RunBehaviorTree(BehaviorTree);
        bot->PC->UseBlackboard(BehaviorTree->BlackboardAsset, &bot->PC->Blackboard);

        bot->Pawn->SetMaxHealth(100);
        bot->Pawn->SetHealth(100);
        bot->Pawn->SetMaxShield(100);
        bot->Pawn->SetShield(0);

        PlayerBotArray.push_back(bot); // gotta do this so we can tick them all
        Log("Bot Spawned With DisplayName: " + bot->DisplayName.ToString());
    }

    void PlayerBotTick() {
        if (!PlayerBotArray.empty()) {
            if (UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.001f))
            {
                FreeDeadBots();
            }
        }
        else {
            return;
        }

        auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
        auto Math = (UKismetMathLibrary*)UKismetMathLibrary::StaticClass()->DefaultObject;
        auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
        auto Statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;

        for (PlayerBot* bot : PlayerBotArray) {
            if (!bot || !bot->Pawn || !bot->PC || !bot->PlayerState || bot->bIsDead)
                continue;

            if (bot->Pawn->IsDead()) {
                bot->bIsDead = true;
                for (size_t i = 0; i < GameMode->AliveBots.Num(); i++)
                {
                    if (GameMode->AliveBots[i]->Pawn == bot->Pawn) {
                        GameMode->AliveBots.Remove(i);
                    }
                }
                GameState->PlayersLeft--;
                GameState->OnRep_PlayerBotsLeft();
                FreeDeadBots();
            }

            if (bot->tick_counter <= 150) {
                bot->tick_counter++;
                continue;
            }

            if (bot->bIsMoving) {
                FVector Vel = bot->Pawn->GetVelocity();
                float Speed = sqrtf(Vel.X * Vel.X + Vel.Y * Vel.Y);

                if (Speed < 0.5f) {
                    if (!bot->IsPickaxeEquiped()) {
                        bot->EquipPickaxe();
                        bot->bPickaxeEquiped = true;
                    }
                    bot->Pawn->PawnStartFire(0);
                }
                else {
                    bot->Pawn->PawnStopFire(0);
                }
            }

            if (bot->bIsMoving && UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.001)) {
                bot->Pawn->Jump();
            }

            if (bot->BotState == EBotState::Warmup) {
                bot->bIsMoving = true;
                bot->Pawn->AddMovementInput(bot->Pawn->GetActorForwardVector(), 1, true);
            }
            else if (bot->BotState == EBotState::PreBus) {
                bot->Pawn->SetHealth(100);
                bot->Pawn->SetShield(100);
                if (bot->bIsMoving) {
                    bot->Pawn->PawnStopFire(0);
                    bot->bIsMoving = false;
                }
                if (!bot->bHasThankedBusDriver && UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.0005f))
                {
                    bot->bHasThankedBusDriver = true;
                    bot->PC->ThankBusDriver();
                }
            }
            else if (bot->BotState == EBotState::Bus) {
                bot->Pawn->SetShield(0);
                if (bot->bIsMoving) {
                    bot->Pawn->PawnStopFire(0);
                    bot->bIsMoving = false;
                }
                if (!bot->bHasThankedBusDriver && UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.0005f))
                {
                    bot->bHasThankedBusDriver = true;
                    bot->PC->ThankBusDriver();
                }

                if (!bot->TimeToNextAction) {
                    bot->TimeToNextAction = UGameplayStatics::GetDefaultObj()->GetTimeSeconds(UWorld::GetWorld());
                    bot->LikelyHoodToDoAction = 0.0008f;
                }
                
                if (UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.0025f)) {
                    if (!bot->bHasThankedBusDriver && UKismetMathLibrary::GetDefaultObj()->RandomBoolWithWeight(0.5f)) {
                        bot->bHasThankedBusDriver = true;
                        bot->PC->ThankBusDriver();
                    }
                    bot->Pawn->K2_TeleportTo(GameState->GetAircraft(0)->K2_GetActorLocation(), {});
                    bot->Pawn->BeginSkydiving(true);
                    bot->TimeToNextAction = 0;
                    bot->LikelyHoodToDoAction = 0;
                    bot->BotState = EBotState::Skydiving;
                }
            }
            else if (bot->BotState == EBotState::Skydiving) {
                if (bot->bIsMoving) {
                    bot->Pawn->PawnStopFire(0);
                    bot->bIsMoving = false;
                }
                if (!bot->Pawn->bIsSkydiving) {
                    bot->Pawn->bInGliderRedeploy = true;
                    bot->BotState = EBotState::Gliding;
                    Log("oh well");
                }
                else {
                    auto BotPos = bot->Pawn->K2_GetActorLocation();
                    FVector Nearest = bot->FindNearestPlayerOrBot();
                    AActor* NearestChest = bot->FindNearestChest();
                    AActor* NearestPickup = (AActor*)bot->FindNearestPickup();
                    if (!NearestChest || !NearestPickup) {
                        continue;
                    }
                    AActor* NearestLootable = NearestChest;
                    if (NearestChest->GetDistanceTo(bot->Pawn) > NearestPickup->GetDistanceTo(bot->Pawn)) {
                        NearestLootable = NearestPickup;
                    }
                    if (!Nearest.IsZero()) {
                        float Dist = Math->Vector_Distance(BotPos, Nearest);
                        if (Dist < 3000.f) {
                            auto TestRot = Math->FindLookAtRotation(Nearest, BotPos);

                            bot->PC->SetControlRotation(TestRot);
                            bot->PC->K2_SetActorRotation(TestRot, true);
                            bot->Pawn->AddMovementInput(bot->Pawn->GetActorForwardVector(), 1.2f, true);
                        }
                        else {
                            auto TestRot = Math->FindLookAtRotation(BotPos, NearestLootable->K2_GetActorLocation());

                            bot->PC->SetControlRotation(TestRot);
                            bot->PC->K2_SetActorRotation(TestRot, true);
                            bot->LookAt(NearestLootable);

                            bot->PC->MoveToActor(NearestLootable, 0.f, true, false, true, nullptr, true);
                            bot->Pawn->AddMovementInput(UKismetMathLibrary::GetDefaultObj()->NegateVector(bot->Pawn->GetActorUpVector()), 1, true);
                            //bot->Pawn->AddMovementInput(bot->Pawn->GetActorForwardVector(), 1.2f, true);
                        }
                    }
                    else {
                        auto TestRot = Math->FindLookAtRotation(BotPos, NearestLootable->K2_GetActorLocation());

                        bot->PC->SetControlRotation(TestRot);
                        bot->PC->K2_SetActorRotation(TestRot, true);
                        bot->LookAt(NearestLootable);

                        bot->PC->MoveToActor(NearestLootable, 0.f, true, false, true, nullptr, true);
                        bot->Pawn->AddMovementInput(UKismetMathLibrary::GetDefaultObj()->NegateVector(bot->Pawn->GetActorUpVector()), 1, true);
                        //bot->Pawn->AddMovementInput(bot->Pawn->GetActorForwardVector(), 1.2f, true);
                    }
                }
            }
            else if (bot->BotState == EBotState::Gliding) {
                FVector Vel = bot->Pawn->GetVelocity();
                float Speed = Vel.Z;
                if (Speed > 0.025f) {
                    bot->BotState = EBotState::Landed;
                }

                auto BotPos = bot->Pawn->K2_GetActorLocation();
                FVector Nearest = bot->FindNearestPlayerOrBot();
                AActor* NearestChest = bot->FindNearestChest();
                AActor* NearestPickup = (AActor*)bot->FindNearestPickup();
                if (!NearestChest || !NearestPickup) {
                    continue;
                }
                AActor* NearestLootable = NearestChest;
                if (NearestChest->GetDistanceTo(bot->Pawn) > NearestPickup->GetDistanceTo(bot->Pawn)) {
                    NearestLootable = NearestPickup;
                }
                float Dist = Math->Vector_Distance(BotPos, Nearest);
                if (!Nearest.IsZero()) {
                    if (Dist < 3500.f) {
                        auto TestRot = Math->FindLookAtRotation(Nearest, BotPos);

                        bot->PC->SetControlRotation(TestRot);
                        bot->PC->K2_SetActorRotation(TestRot, true);
                        bot->Pawn->AddMovementInput(bot->Pawn->GetActorForwardVector(), 1.2f, true);
                    }
                    else {
                        auto TestRot = Math->FindLookAtRotation(BotPos, NearestLootable->K2_GetActorLocation());

                        bot->PC->SetControlRotation(TestRot);
                        bot->PC->K2_SetActorRotation(TestRot, true);
                        bot->LookAt(NearestLootable);

                        bot->PC->MoveToActor(NearestLootable, 0.f, true, false, true, nullptr, true);
                        //bot->Pawn->AddMovementInput(bot->Pawn->GetActorForwardVector(), 1.2f, true);
                    }
                }
                else {
                    auto TestRot = Math->FindLookAtRotation(BotPos, NearestLootable->K2_GetActorLocation());

                    bot->PC->SetControlRotation(TestRot);
                    bot->PC->K2_SetActorRotation(TestRot, true);
                    bot->LookAt(NearestLootable);

                    bot->PC->MoveToActor(NearestLootable, 0.f, true, false, true, nullptr, true);
                    //bot->Pawn->AddMovementInput(bot->Pawn->GetActorForwardVector(), 1.2f, true);
                }
            }

            bot->tick_counter++;
        }
    }

    inline void (*OnPawnAISpawnedOG)(AActor* Controller, AFortPlayerPawnAthena* Pawn);
    void OnPawnAISpawned(AActor* Controller, AFortPlayerPawnAthena* Pawn) {
        //Log("OnPawnAISpawned Called!");

        if (Controller || !Pawn || !Pawn->Controller) {
            //Log("Controller dont exist!");
        }
        if (!Pawn || !Pawn->Controller) {
            Log("Pawn/Controller dont exist!");
            return;
        }
        if (Pawn) {
            //Log("Pawn Exists");
            //Log("PawnClass: " + Pawn->Class->GetFullName());
        }
        if (Controller) {}
        else {
            Log("nuh uh");
        }

        if (Pawn && Pawn->Controller && Pawn->Controller->Class) {
            //Log("Pawn Class: " + Pawn->Controller->Class->GetFullName());
        }
        else {
            Log("dont exist!");
        }

        if (Pawn->Controller->Class->GetFullName().contains("Phoebe")) {
            OnPawnAISpawnedOG(Controller, Pawn);
            AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
            AFortAthenaAIBotController* BotPC = (AFortAthenaAIBotController*)Pawn->Controller;
            if (BotPC) {
                auto BotPlayerState = (AFortPlayerStateAthena*)Pawn->PlayerState;
                ((AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode)->AliveBots.Add(BotPC);
                GameState->PlayersLeft++;
                GameState->OnRep_PlayerBotsLeft();
                return InitPlayerBot(Pawn, BotPC, BotPlayerState);
                
                for (auto& Item : ((AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode)->StartingItems)
                {
                    UFortWorldItem* WorldItem = Cast<UFortWorldItem>(Item.Item->CreateTemporaryItemInstanceBP(Item.Count, 0));
                    WorldItem->OwnerInventory = BotPC->Inventory;
                    FFortItemEntry& Entry = WorldItem->ItemEntry;
                    BotPC->Inventory->Inventory.ReplicatedEntries.Add(Entry);
                    BotPC->Inventory->Inventory.ItemInstances.Add(WorldItem);
                    BotPC->Inventory->Inventory.MarkItemDirty(Entry);
                    BotPC->Inventory->HandleInventoryLocalUpdate();
                }
            }
            else {
                Log("No BotPC!");
            }
            return;
        }

        OnPawnAISpawnedOG(Controller, Pawn);
    }

    void Hook() {
        MH_CreateHook((LPVOID)(ImageBase + 0x5CED164), OnPawnAISpawned, (LPVOID*)&OnPawnAISpawnedOG);
        //HookVTable(AFortAthenaAIBotController::StaticClass(), 263, OnPawnAISpawned, (LPVOID*)&OnPawnAISpawnedOG);

        Log("Bots Hooked!");
    }
}