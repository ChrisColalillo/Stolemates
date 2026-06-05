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

	if (StolematesGI && StolematesGI->MatchType == EMatchType::Online)
	{
		StartOnlineGame();
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
		player->GetMesh()->SetMaterial(0, PlayerMaterials[i]);
		UGameplayStatics::SpawnDecalAttached(PlayerDecals[i], FVector(256, 256 * player->DecalScale, 256 * player->DecalScale), player->GetRootComponent(), FName("Decal"), FVector(0, 0, 0), FRotator(90, 0, 0), EAttachLocation::SnapToTargetIncludingScale, 0);
		UGameplayStatics::CreatePlayer(this, i, true);
		UGameplayStatics::GetPlayerController(player, i)->Possess(player);
		UGameplayStatics::GetPlayerController(player, i)->SetViewTargetWithBlend(camera);
		players.Push(player);
	}

	LoadHud(UGameplayStatics::GetPlayerController(players[0], 0));
	GetWorldTimerManager().SetTimer(powerUpSpawnTimerHandle, this, &AStolenmatesGameModeBase::SpawnPowerUp, 5.0f, true);
	gameOver = false;
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

		if (PlayerMaterials.IsValidIndex(PlayerIndex))
		{
			Player->GetMesh()->SetMaterial(0, PlayerMaterials[PlayerIndex]);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("No material found for online player index: %d"), PlayerIndex);
		}

		if (PlayerDecals.IsValidIndex(PlayerIndex))
		{
			UGameplayStatics::SpawnDecalAttached(
				PlayerDecals[PlayerIndex],
				FVector(256, 256 * Player->DecalScale, 256 * Player->DecalScale),
				Player->GetRootComponent(),
				FName("Decal"),
				FVector(0, 0, 0),
				FRotator(90, 0, 0),
				EAttachLocation::SnapToTargetIncludingScale,
				0
			);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("No decal found for online player index: %d"), PlayerIndex);
		}

		PlayerController->Possess(Player);

		if (camera)
		{
			PlayerController->SetViewTargetWithBlend(camera);
		}

		players.Push(Player);

		UE_LOG(LogTemp, Warning, TEXT("Online character spawned and possessed"));

		PlayerIndex++;
	}

	UE_LOG(LogTemp, Warning, TEXT("Online players spawned: %d"), players.Num());

	if (players.Num() > 0)
	{
		LoadHud(UGameplayStatics::GetPlayerController(this, 0));
	}

	GetWorldTimerManager().SetTimer(
		powerUpSpawnTimerHandle,
		this,
		&AStolenmatesGameModeBase::SpawnPowerUp,
		5.0f,
		true
	);

	gameOver = false;
}

void AStolenmatesGameModeBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (gameOver)
		return;
	camera->Focus(players);
	for (int i = 0; i < players.Num(); i++)
	{
		if (getTimer(i) >= TimeToWin)
		{
			for (int j = 0; j < players.Num(); j++)
			{
				Cast<AStolenmatesPlayer>(players[j])->gameOver = true;
			}
			GameOverHud(UGameplayStatics::GetPlayerController(players[0], 0));
			gameOver = true;
			return;
		}
	}
}

void AStolenmatesGameModeBase::playAgain()
{
	UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetName()), false);
	TArray<AActor*> Locations;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), PlayerSpawners, Locations);
	for (int i = 0; i < players.Num(); i++)
	{
		players[i]->SetActorLocationAndRotation(Locations[i]->GetActorLocation(), Locations[i]->GetActorRotation());
		Cast<AStolenmatesPlayer>(players[i])->gameOver = false;
	}
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

