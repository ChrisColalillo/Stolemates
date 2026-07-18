// Fill out your copyright notice in the Description page of Project Settings.


#include "StolematesLobbyGameMode.h"
#include "StolematesLobbyGameState.h"
#include "GameFramework/GameStateBase.h"
#include "StolematesGameInstance.h"

AStolematesLobbyGameMode::AStolematesLobbyGameMode()
{
	GameStateClass = AStolematesLobbyGameState::StaticClass();
}

void AStolematesLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	UE_LOG(LogTemp, Warning, TEXT("Lobby player joined"));

	AStolematesLobbyGameState* LobbyGameState = GetGameState<AStolematesLobbyGameState>();

	if (LobbyGameState)
	{
		LobbyGameState->BroadcastPlayerListChanged();
	}
}

void AStolematesLobbyGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	UE_LOG(LogTemp, Warning, TEXT("Lobby player left"));

	AStolematesLobbyGameState* LobbyGameState = GetGameState<AStolematesLobbyGameState>();

	if (LobbyGameState)
	{
		LobbyGameState->BroadcastPlayerListChanged();
	}
}



void AStolematesLobbyGameMode::StartOnlineMatch()
{
	AGameStateBase* GS = GetGameState<AGameStateBase>();

	int32 PlayerCount = 1;

	if (GS)
	{
		PlayerCount = GS->PlayerArray.Num();

		UE_LOG(LogTemp, Warning, TEXT("Lobby Start: PlayerArray count: %d"), PlayerCount);
	}

	UStolematesGameInstance* StolematesGI =
		Cast<UStolematesGameInstance>(GetGameInstance());

	if (StolematesGI)
	{
		StolematesGI->ExpectedOnlinePlayerCount = FMath::Max(PlayerCount, 1);

		UE_LOG(LogTemp, Warning, TEXT("ExpectedOnlinePlayerCount set to: %d"),
			StolematesGI->ExpectedOnlinePlayerCount);
	}

	int32 ControllerCount = 0;

	if (GetWorld())
	{
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			ControllerCount++;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Lobby Start: PlayerController count: %d"), ControllerCount);

	if (GetWorld())
	{
		GetWorld()->ServerTravel(TEXT("/Game/Levels/Final_Level"));
	}
}