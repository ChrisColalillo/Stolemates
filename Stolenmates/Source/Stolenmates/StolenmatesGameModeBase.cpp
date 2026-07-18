// Fill out your copyright notice in the Description page of Project Settings.


#include "StolenmatesGameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Components/SkeletalMeshComponent.h"
#include "StolenmatesPlayer.h"
#include "Camera.h"
#include "Materials/Material.h"
#include "TimerManager.h"
#include "Components/DecalComponent.h"
#include "Kismet/GameplayStatics.h"
#include "StolematesGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "StolematesPlayerController.h"
#include "EngineUtils.h"
#include "StolenmatesPlayer.h"


AStolenmatesGameModeBase::AStolenmatesGameModeBase()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.0f;
}

void AStolenmatesGameModeBase::StartPlay()
{
	Super::StartPlay();

	UStolematesGameInstance* StolematesGI =
		Cast<UStolematesGameInstance>(GetGameInstance());

	if (StolematesGI)
	{
		UE_LOG(LogTemp, Warning, TEXT("Final_Level MatchType: %s"),
			StolematesGI->MatchType == EMatchType::Online ? TEXT("Online") : TEXT("Local"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Final_Level MatchType: GameInstance missing"));
	}

	if (StolematesGI && StolematesGI->MatchType == EMatchType::Online)
	{
		GetWorldTimerManager().SetTimerForNextTick(
			this,
			&AStolenmatesGameModeBase::TryStartOnlineGame
		);
	}
	else
	{
		StartLocalGame();
	}
}

static void SortSpawnLocationsByPlayerTag(TArray<AActor*>& Locations)
{
	Locations.Sort([](const AActor& A, const AActor& B)
	{
		auto GetSpawnIndex = [](const AActor& Actor)
		{
			if (Actor.ActorHasTag("PlayerSpawn0")) return 0;
			if (Actor.ActorHasTag("PlayerSpawn1")) return 1;
			if (Actor.ActorHasTag("PlayerSpawn2")) return 2;
			if (Actor.ActorHasTag("PlayerSpawn3")) return 3;

			// Untagged spawns go last.
			return 999;
		};

		return GetSpawnIndex(A) < GetSpawnIndex(B);
	});
}

void AStolenmatesGameModeBase::StartLocalGame()
{
	SetAllPlayersInputLocked(false);

	camera = Cast<ACamera>(GetWorld()->SpawnActor<AActor>(BPCamera, FVector(0, 0, cameraStartHeight), FRotator::ZeroRotator));

	TArray<AActor*> Locations;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), PlayerSpawners, Locations);

	SortSpawnLocationsByPlayerTag(Locations);

	UStolematesGameInstance* StolematesGI =
		Cast<UStolematesGameInstance>(GetGameInstance());

	int32 NumPlayersToSpawn = 4;

	if (StolematesGI)
	{
		NumPlayersToSpawn = StolematesGI->LocalPlayerCount;
	}

	NumPlayersToSpawn = FMath::Clamp(NumPlayersToSpawn, 1, Locations.Num());

	for (int32 i = 0; i < NumPlayersToSpawn; i++)
	{
		AStolenmatesPlayer* player = GetWorld()->SpawnActor<AStolenmatesPlayer>(BPPlayer, Locations[i]->GetActorLocation(), Locations[i]->GetActorRotation());
		player->PlayerIndex = i;
		//player->GetMesh()->SetMaterial(0, PlayerMaterials[i]);
		//UGameplayStatics::SpawnDecalAttached(PlayerDecals[i], FVector(256, 256 * player->DecalScale, 256 * player->DecalScale), player->GetRootComponent(), FName("Decal"), FVector(0, 0, 0), FRotator(90, 0, 0), EAttachLocation::SnapToTargetIncludingScale, 0);
		APlayerController* PlayerController = nullptr;

		if (i == 0)
		{
			PlayerController = UGameplayStatics::GetPlayerController(this, 0);
		}
		else
		{
			PlayerController = UGameplayStatics::CreatePlayer(this, i, true);
		}

		if (PlayerController)
		{
			PlayerController->Possess(player);
			PlayerController->SetViewTargetWithBlend(camera);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to get/create local PlayerController index: %d"), i);
		}

		players.Push(player);
	}

	AStolematesPlayerController* StolematesPC =
		Cast<AStolematesPlayerController>(UGameplayStatics::GetPlayerController(this, 0));

	if (StolematesPC)
	{
		StolematesPC->ClientShowInGameHUD(players.Num(), TimeToWin);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to show local HUD: PlayerController was not StolematesPlayerController"));
	}

	//GetWorldTimerManager().SetTimer(powerUpSpawnTimerHandle, this, &AStolenmatesGameModeBase::SpawnPowerUp, 5.0f, true);
	gameOver = false;

	SetAllPlayersInputLocked(false);
	UE_LOG(LogTemp, Warning, TEXT("TEMP: Local players unlocked immediately."));
}



void AStolenmatesGameModeBase::TryStartOnlineGame()
{
	if (bOnlineGameStarted)
	{
		return;
	}

	UStolematesGameInstance* StolematesGI =
		Cast<UStolematesGameInstance>(GetGameInstance());

	int32 ExpectedCount = 1;

	if (StolematesGI)
	{
		ExpectedCount = FMath::Max(StolematesGI->ExpectedOnlinePlayerCount, 1);
	}

	int32 CurrentControllerCount = 0;

	if (GetWorld())
	{
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			CurrentControllerCount++;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("TryStartOnlineGame: Controllers %d / Expected %d"),
		CurrentControllerCount,
		ExpectedCount
	);

	if (CurrentControllerCount >= ExpectedCount)
	{
		bOnlineGameStarted = true;
		StartOnlineGame();
		return;
	}

	FTimerHandle RetryHandle;

	GetWorldTimerManager().SetTimer(
		RetryHandle,
		this,
		&AStolenmatesGameModeBase::TryStartOnlineGame,
		0.25f,
		false
	);
}



void AStolenmatesGameModeBase::StartOnlineGame()
{
	UE_LOG(LogTemp, Warning, TEXT("Starting Online Game"));

	if (!BPCamera)
	{
		UE_LOG(LogTemp, Error, TEXT("BPCamera is null"));
	}

	if (!BPPlayer)
	{
		UE_LOG(LogTemp, Error, TEXT("BPPlayer is null"));
	}

	if (!PlayerSpawners)
	{
		UE_LOG(LogTemp, Error, TEXT("PlayerSpawners class is null"));
	}

	camera = Cast<ACamera>(
		GetWorld()->SpawnActor<AActor>(
			BPCamera,
			FVector(0, 0, cameraStartHeight),
			FRotator::ZeroRotator
			)
		);

	if (!camera)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to spawn online camera"));
	}

	TArray<AActor*> Locations;
	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(),
		PlayerSpawners,
		Locations
	);

	SortSpawnLocationsByPlayerTag(Locations);

	UE_LOG(LogTemp, Warning, TEXT("Online spawn locations found: %d"), Locations.Num());

	int32 ControllerCount = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ControllerCount++;
	}

	UE_LOG(LogTemp, Warning, TEXT("Online PlayerControllers found: %d"), ControllerCount);

	int32 PlayerIndex = 0;

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();

		if (!PlayerController)
		{
			UE_LOG(LogTemp, Warning, TEXT("PlayerController was null"));
			continue;
		}

		if (!Locations.IsValidIndex(PlayerIndex))
		{
			UE_LOG(LogTemp, Warning, TEXT("Not enough player spawners for online players"));
			break;
		}

		UE_LOG(LogTemp, Warning, TEXT("Spawning online character index: %d"), PlayerIndex);

		AStolenmatesPlayer* Player = GetWorld()->SpawnActor<AStolenmatesPlayer>(
			BPPlayer,
			Locations[PlayerIndex]->GetActorLocation(),
			Locations[PlayerIndex]->GetActorRotation()
			);

		if (!Player)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to spawn online player pawn"));
			continue;
		}

		Player->PlayerIndex = PlayerIndex;

		UE_LOG(LogTemp, Warning, TEXT("Assigned replicated PlayerIndex %d to pawn %s"),
			PlayerIndex,
			*Player->GetName()
		);

		//if (PlayerMaterials.IsValidIndex(PlayerIndex))
		//{
		//	Player->GetMesh()->SetMaterial(0, PlayerMaterials[PlayerIndex]);
		//}
		//else
		//{
		//	UE_LOG(LogTemp, Warning, TEXT("No material found for online player index: %d"), PlayerIndex);
		//}

		//if (PlayerDecals.IsValidIndex(PlayerIndex))
		//{
		//	UGameplayStatics::SpawnDecalAttached(
		//		PlayerDecals[PlayerIndex],
		//		FVector(256, 256 * Player->DecalScale, 256 * Player->DecalScale),
		//		Player->GetRootComponent(),
		//		FName("Decal"),
		//		FVector(0, 0, 0),
		//		FRotator(90, 0, 0),
		//		EAttachLocation::SnapToTargetIncludingScale,
		//		0
		//	);
		//}
		//else
		//{
		//	UE_LOG(LogTemp, Warning, TEXT("No decal found for online player index: %d"), PlayerIndex);
		//}

		UE_LOG(LogTemp, Warning,
			TEXT("Assigning PlayerController %s to spawned pawn index %d"),
			*PlayerController->GetName(),
			PlayerIndex
		);

		PlayerController->Possess(Player);

		if (camera)
		{
			AStolematesPlayerController* StolematesPC =
				Cast<AStolematesPlayerController>(PlayerController);

			if (StolematesPC)
			{
				StolematesPC->ClientSetSharedCamera(camera);

				UE_LOG(LogTemp, Warning,
					TEXT("Sent shared camera to controller %s"),
					*PlayerController->GetName()
				);
			}
		}

		players.Push(Player);

		UE_LOG(LogTemp, Warning, TEXT("Online character spawned and possessed"));

		PlayerIndex++;
	}

	UE_LOG(LogTemp, Warning, TEXT("Online players spawned: %d"), players.Num());
	UE_LOG(LogTemp, Warning, TEXT("Shared camera will focus player count: %d"), players.Num());

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AStolematesPlayerController* StolematesPC = Cast<AStolematesPlayerController>(It->Get());

		if (StolematesPC)
		{
			StolematesPC->ClientShowInGameHUD(players.Num(), TimeToWin);
		}
	}

	//GetWorldTimerManager().SetTimer(
	//	powerUpSpawnTimerHandle,
	//	this,
	//	&AStolenmatesGameModeBase::SpawnPowerUp,
	//	5.0f,
	//	true
	//);

	gameOver = false;

	SetAllPlayersInputLocked(true);
	UE_LOG(LogTemp, Warning, TEXT("ONLINE MATCH START: Players locked. Waiting for all clients to be ready."));

	ExpectedMatchStartPlayerCount = 0;

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (It->Get())
		{
			ExpectedMatchStartPlayerCount++;
		}
	}

	ExpectedMatchStartPlayerCount = FMath::Max(ExpectedMatchStartPlayerCount, 1);

	MatchStartReadyControllers.Empty();
	bWaitingForMatchStartReady = true;

	BroadcastMatchStartMessage(TEXT("Connecting..."), true);
}

void AStolenmatesGameModeBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (gameOver)
		return;

	if (!camera)
	{
		return;
	}

	camera->Focus(players);
	for (int i = 0; i < players.Num(); i++)
	{
		if (getTimer(i) >= TimeToWin)
		{
			int32 WinnerIndex = getWinner();

			for (int j = 0; j < players.Num(); j++)
			{
				Cast<AStolenmatesPlayer>(players[j])->gameOver = true;
			}

			for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
			{
				AStolematesPlayerController* StolematesPC = Cast<AStolematesPlayerController>(It->Get());

				if (StolematesPC)
				{
					StolematesPC->ClientShowGameOverHUD(WinnerIndex);
				}
			}

			gameOver = true;
			return;
		}
	}
}

void AStolenmatesGameModeBase::playAgain()
{
	UStolematesGameInstance* StolematesGI =
		Cast<UStolematesGameInstance>(GetGameInstance());

	// Online match restart.
	// The host/server should move everyone back into Final_Level together.
	if (StolematesGI && StolematesGI->MatchType == EMatchType::Online)
	{
		if (GetWorld())
		{
			GetWorld()->ServerTravel(TEXT("/Game/Levels/Final_Level"));
		}

		return;
	}

	// Local match restart.
	// Reload the level and let StartLocalGame() rebuild the match cleanly.
	UGameplayStatics::OpenLevel(
		this,
		FName("/Game/Levels/Final_Level")
	);
}

int AStolenmatesGameModeBase::getWinner()
{
	int winnerIndex =0;
	for (int i = 0; i < players.Num(); i++)
	{
		if (getTimer(i) > getTimer(winnerIndex))
			winnerIndex = i;
	}
	return winnerIndex;
}

float AStolenmatesGameModeBase::getTimer(int i)
{
	if (i >= players.Num())
		return 0;
	return Cast<AStolenmatesPlayer>(players[i])->timeHoldingHeart;
}

TArray<ACharacter*> AStolenmatesGameModeBase::getPlayers()
{
	return  players;
}



bool AStolenmatesGameModeBase::RequestOnlinePause(APlayerController* RequestingPlayer)
{
	if (!RequestingPlayer)
	{
		UE_LOG(LogTemp, Warning, TEXT("Pause failed: RequestingPlayer was null"));
		return false;
	}

	if (PlayerWhoPaused)
	{
		UE_LOG(LogTemp, Warning, TEXT("Pause denied: game is already paused by another player"));
		return false;
	}

	bool bPauseSucceeded = SetPause(RequestingPlayer);

	UE_LOG(LogTemp, Warning,
		TEXT("SetPause result: %s | IsPaused: %s"),
		bPauseSucceeded ? TEXT("true") : TEXT("false"),
		UGameplayStatics::IsGamePaused(GetWorld()) ? TEXT("true") : TEXT("false")
	);

	if (bPauseSucceeded)
	{
		PlayerWhoPaused = RequestingPlayer;
		BroadcastOnlinePauseMenu(PlayerWhoPaused);

		UE_LOG(LogTemp, Warning, TEXT("Game paused by player controller"));
		return true;
	}

	return false;
}



bool AStolenmatesGameModeBase::RequestOnlineResume(APlayerController* RequestingPlayer)
{
	if (!RequestingPlayer)
	{
		UE_LOG(LogTemp, Warning, TEXT("Resume failed: RequestingPlayer was null"));
		return false;
	}

	if (PlayerWhoPaused != RequestingPlayer)
	{
		UE_LOG(LogTemp, Warning, TEXT("Resume denied: player did not initiate pause"));
		return false;
	}

	bool bResumeSucceeded = ClearPause();

	UE_LOG(LogTemp, Warning,
		TEXT("ClearPause result: %s | IsPaused: %s"),
		bResumeSucceeded ? TEXT("true") : TEXT("false"),
		UGameplayStatics::IsGamePaused(GetWorld()) ? TEXT("true") : TEXT("false")
	);

	if (bResumeSucceeded)
	{
		PlayerWhoPaused = nullptr;
		BroadcastOnlineResume();

		UE_LOG(LogTemp, Warning, TEXT("Game resumed by pause owner"));
		return true;
	}

	return false;
}



void AStolenmatesGameModeBase::BroadcastPowerUpHUDUpdate(int32 PlayerIndex, int32 PowerUpIndex)
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AStolematesPlayerController* StolematesPC = Cast<AStolematesPlayerController>(It->Get());

		if (StolematesPC)
		{
			StolematesPC->ClientUpdatePowerUpHUD(PlayerIndex, PowerUpIndex);
		}
	}
}



void AStolenmatesGameModeBase::HandlePlayerLeavingMatch(APlayerController* LeavingPlayer)
{
	if (!LeavingPlayer)
	{
		return;
	}

	if (PlayerWhoPaused == LeavingPlayer)
	{
		ClearPause();
		PlayerWhoPaused = nullptr;
		BroadcastOnlineResume();

		UE_LOG(LogTemp, Warning, TEXT("Leaving player owned pause. Cleared pause before return to menu."));
		return;
	}

	if (UGameplayStatics::IsGamePaused(GetWorld()) && PlayerWhoPaused == nullptr)
	{
		ClearPause();
		BroadcastOnlineResume();

		UE_LOG(LogTemp, Warning, TEXT("Game was paused with no pause owner. Cleared pause before return to menu."));
	}
}

void AStolenmatesGameModeBase::BroadcastOnlinePauseMenu(APlayerController* PauseOwner)
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AStolematesPlayerController* StolematesPC =
			Cast<AStolematesPlayerController>(It->Get());

		if (StolematesPC)
		{
			const bool bCanResume = StolematesPC == PauseOwner;
			StolematesPC->ClientShowOnlinePauseMenu(bCanResume);
		}
	}
}

void AStolenmatesGameModeBase::BroadcastOnlineResume()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AStolematesPlayerController* StolematesPC =
			Cast<AStolematesPlayerController>(It->Get());

		if (StolematesPC)
		{
			StolematesPC->ClientHideOnlinePauseMenu();
		}
	}
}



void AStolenmatesGameModeBase::SetAllPlayersInputLocked(bool bLocked)
{
	for (TActorIterator<AStolenmatesPlayer> It(GetWorld()); It; ++It)
	{
		AStolenmatesPlayer* Player = *It;

		if (Player)
		{
			Player->bInputLocked = bLocked;

			UE_LOG(LogTemp, Warning,
				TEXT("Set bInputLocked=%s for PlayerIndex %d"),
				bLocked ? TEXT("true") : TEXT("false"),
				Player->PlayerIndex
			);
		}
	}
}

void AStolenmatesGameModeBase::UnlockPlayersForMatchStart()
{
	SetAllPlayersInputLocked(false);

	BroadcastMatchStartMessage(TEXT(""), false);

	UE_LOG(LogTemp, Warning, TEXT("MATCH START: All players unlocked."));
}

void AStolenmatesGameModeBase::BroadcastMatchStartMessage(const FString& Message, bool bVisible)
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AStolematesPlayerController* PC = Cast<AStolematesPlayerController>(It->Get());

		if (PC)
		{
			PC->ClientSetMatchStartMessage(Message, bVisible);
		}
	}
}

void AStolenmatesGameModeBase::NotifyPlayerReadyForMatchStart(APlayerController* ReadyPlayerController)
{
	if (!bWaitingForMatchStartReady)
	{
		return;
	}

	if (!ReadyPlayerController)
	{
		return;
	}

	if (!MatchStartReadyControllers.Contains(ReadyPlayerController))
	{
		MatchStartReadyControllers.Add(ReadyPlayerController);

		UE_LOG(LogTemp, Warning,
			TEXT("MATCH START READY: PlayerController ready. Ready %d / Expected %d"),
			MatchStartReadyControllers.Num(),
			ExpectedMatchStartPlayerCount
		);
	}

	CheckAllPlayersReadyForMatchStart();
}

void AStolenmatesGameModeBase::CheckAllPlayersReadyForMatchStart()
{
	if (!bWaitingForMatchStartReady)
	{
		return;
	}

	if (MatchStartReadyControllers.Num() < ExpectedMatchStartPlayerCount)
	{
		return;
	}

	bWaitingForMatchStartReady = false;

	UE_LOG(LogTemp, Warning, TEXT("MATCH START READY: All expected players ready. Starting countdown."));

	BroadcastMatchStartMessage(TEXT("Starting..."), true);

	GetWorldTimerManager().ClearTimer(MatchStartUnlockTimerHandle);
	GetWorldTimerManager().SetTimer(
		MatchStartUnlockTimerHandle,
		this,
		&AStolenmatesGameModeBase::UnlockPlayersForMatchStart,
		MatchStartDelay,
		false
	);
}