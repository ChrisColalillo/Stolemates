// Fill out your copyright notice in the Description page of Project Settings.


#include "StolenmatesGameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "StolenmatesPlayer.h"
#include "Camera.h"
#include "TimerManager.h"
#include "StolematesGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "StolematesPlayerController.h"
#include "EngineUtils.h"


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

	if (StolematesGI &&
		StolematesGI->MatchType == EMatchType::Online)
	{
		GetWorldTimerManager().SetTimerForNextTick(
			this,
			&AStolenmatesGameModeBase::TryStartOnlineGame
		);

		return;
	}

	StartLocalGame();
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

	if (!GetWorld() ||
		!BPCamera ||
		!BPPlayer ||
		!PlayerSpawners)
	{
		return;
	}

	camera = Cast<ACamera>(
		GetWorld()->SpawnActor<AActor>(
			BPCamera,
			FVector(0.0f, 0.0f, cameraStartHeight),
			FRotator::ZeroRotator
			)
		);

	if (!camera)
	{
		return;
	}

	TArray<AActor*> Locations;

	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(),
		PlayerSpawners,
		Locations
	);

	SortSpawnLocationsByPlayerTag(Locations);

	if (Locations.Num() == 0)
	{
		return;
	}

	UStolematesGameInstance* StolematesGI =
		Cast<UStolematesGameInstance>(GetGameInstance());

	int32 NumPlayersToSpawn = 4;

	if (StolematesGI)
	{
		NumPlayersToSpawn = StolematesGI->LocalPlayerCount;
	}

	NumPlayersToSpawn = FMath::Clamp(
		NumPlayersToSpawn,
		1,
		Locations.Num()
	);

	for (int32 i = 0; i < NumPlayersToSpawn; i++)
	{
		AStolenmatesPlayer* Player =
			GetWorld()->SpawnActor<AStolenmatesPlayer>(
				BPPlayer,
				Locations[i]->GetActorLocation(),
				Locations[i]->GetActorRotation()
				);

		if (!Player)
		{
			continue;
		}

		Player->PlayerIndex = i;

		APlayerController* PlayerController = nullptr;

		if (i == 0)
		{
			PlayerController =
				UGameplayStatics::GetPlayerController(this, 0);
		}
		else
		{
			PlayerController =
				UGameplayStatics::CreatePlayer(this, i, true);
		}

		if (PlayerController)
		{
			PlayerController->Possess(Player);
			PlayerController->SetViewTargetWithBlend(camera);
		}

		players.Push(Player);
	}

	AStolematesPlayerController* StolematesPC =
		Cast<AStolematesPlayerController>(
			UGameplayStatics::GetPlayerController(this, 0)
			);

	if (StolematesPC)
	{
		StolematesPC->ClientShowInGameHUD(
			players.Num(),
			TimeToWin
		);
	}

	gameOver = false;
	SetAllPlayersInputLocked(false);
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
		ExpectedCount = FMath::Max(
			StolematesGI->ExpectedOnlinePlayerCount,
			1
		);
	}

	int32 CurrentControllerCount = 0;

	if (GetWorld())
	{
		for (FConstPlayerControllerIterator It =
			GetWorld()->GetPlayerControllerIterator();
			It;
			++It)
		{
			CurrentControllerCount++;
		}
	}

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
	if (!GetWorld() ||
		!BPCamera ||
		!BPPlayer ||
		!PlayerSpawners)
	{
		return;
	}

	camera = Cast<ACamera>(
		GetWorld()->SpawnActor<AActor>(
			BPCamera,
			FVector(0.0f, 0.0f, cameraStartHeight),
			FRotator::ZeroRotator
			)
		);

	if (!camera)
	{
		return;
	}

	TArray<AActor*> Locations;

	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(),
		PlayerSpawners,
		Locations
	);

	SortSpawnLocationsByPlayerTag(Locations);

	if (Locations.Num() == 0)
	{
		return;
	}

	int32 PlayerIndex = 0;

	for (FConstPlayerControllerIterator It =
		GetWorld()->GetPlayerControllerIterator();
		It;
		++It)
	{
		APlayerController* PlayerController = It->Get();

		if (!PlayerController)
		{
			continue;
		}

		if (!Locations.IsValidIndex(PlayerIndex))
		{
			break;
		}

		AStolenmatesPlayer* Player =
			GetWorld()->SpawnActor<AStolenmatesPlayer>(
				BPPlayer,
				Locations[PlayerIndex]->GetActorLocation(),
				Locations[PlayerIndex]->GetActorRotation()
				);

		if (!Player)
		{
			continue;
		}

		Player->PlayerIndex = PlayerIndex;
		PlayerController->Possess(Player);

		AStolematesPlayerController* StolematesPC =
			Cast<AStolematesPlayerController>(
				PlayerController
				);

		if (StolematesPC)
		{
			StolematesPC->ClientSetSharedCamera(camera);
		}

		players.Push(Player);
		PlayerIndex++;
	}

	if (players.Num() == 0)
	{
		return;
	}

	for (FConstPlayerControllerIterator It =
		GetWorld()->GetPlayerControllerIterator();
		It;
		++It)
	{
		AStolematesPlayerController* StolematesPC =
			Cast<AStolematesPlayerController>(It->Get());

		if (StolematesPC)
		{
			StolematesPC->ClientShowInGameHUD(
				players.Num(),
				TimeToWin
			);
		}
	}

	gameOver = false;
	SetAllPlayersInputLocked(true);

	ExpectedMatchStartPlayerCount = 0;

	for (FConstPlayerControllerIterator It =
		GetWorld()->GetPlayerControllerIterator();
		It;
		++It)
	{
		if (It->Get())
		{
			ExpectedMatchStartPlayerCount++;
		}
	}

	ExpectedMatchStartPlayerCount =
		FMath::Max(ExpectedMatchStartPlayerCount, 1);

	MatchStartReadyControllers.Empty();
	bWaitingForMatchStartReady = true;

	BroadcastMatchStartMessage(
		TEXT("Connecting..."),
		true
	);
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

	if (StolematesGI &&
		StolematesGI->MatchType == EMatchType::Online)
	{
		if (GetWorld())
		{
			GetWorld()->ServerTravel(
				TEXT("/Game/Levels/Final_Level")
			);
		}

		return;
	}

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


bool AStolenmatesGameModeBase::RequestOnlinePause(
	APlayerController* RequestingPlayer)
{
	if (gameOver ||
		!RequestingPlayer ||
		PlayerWhoPaused)
	{
		return false;
	}

	if (!SetPause(RequestingPlayer))
	{
		return false;
	}

	PlayerWhoPaused = RequestingPlayer;
	BroadcastOnlinePauseMenu(PlayerWhoPaused);

	return true;
}



bool AStolenmatesGameModeBase::RequestOnlineResume(
	APlayerController* RequestingPlayer)
{
	if (!RequestingPlayer ||
		PlayerWhoPaused != RequestingPlayer)
	{
		return false;
	}

	if (!ClearPause())
	{
		return false;
	}

	PlayerWhoPaused = nullptr;
	BroadcastOnlineResume();

	return true;
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



void AStolenmatesGameModeBase::HandlePlayerLeavingMatch(
	APlayerController* LeavingPlayer)
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
		return;
	}

	if (UGameplayStatics::IsGamePaused(GetWorld()) &&
		PlayerWhoPaused == nullptr)
	{
		ClearPause();
		BroadcastOnlineResume();
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



void AStolenmatesGameModeBase::SetAllPlayersInputLocked(
	bool bLocked)
{
	for (TActorIterator<AStolenmatesPlayer> It(GetWorld());
		It;
		++It)
	{
		AStolenmatesPlayer* Player = *It;

		if (Player)
		{
			Player->bInputLocked = bLocked;
		}
	}
}

void AStolenmatesGameModeBase::UnlockPlayersForMatchStart()
{
	SetAllPlayersInputLocked(false);
	BroadcastMatchStartMessage(TEXT(""), false);
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

void AStolenmatesGameModeBase::NotifyPlayerReadyForMatchStart(
	APlayerController* ReadyPlayerController)
{
	if (!bWaitingForMatchStartReady ||
		!ReadyPlayerController)
	{
		return;
	}

	MatchStartReadyControllers.AddUnique(
		ReadyPlayerController
	);

	CheckAllPlayersReadyForMatchStart();
}

void AStolenmatesGameModeBase::CheckAllPlayersReadyForMatchStart()
{
	if (!bWaitingForMatchStartReady)
	{
		return;
	}

	if (MatchStartReadyControllers.Num() <
		ExpectedMatchStartPlayerCount)
	{
		return;
	}

	bWaitingForMatchStartReady = false;

	BroadcastMatchStartMessage(
		TEXT("Starting..."),
		true
	);

	GetWorldTimerManager().ClearTimer(
		MatchStartUnlockTimerHandle
	);

	GetWorldTimerManager().SetTimer(
		MatchStartUnlockTimerHandle,
		this,
		&AStolenmatesGameModeBase::UnlockPlayersForMatchStart,
		MatchStartDelay,
		false
	);
}