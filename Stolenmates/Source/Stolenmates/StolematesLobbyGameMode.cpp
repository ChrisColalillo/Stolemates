// Fill out your copyright notice in the Description page of Project Settings.


#include "StolematesLobbyGameMode.h"
#include "StolematesLobbyGameState.h"
#include "StolematesGameInstance.h"

AStolematesLobbyGameMode::AStolematesLobbyGameMode()
{
	GameStateClass = AStolematesLobbyGameState::StaticClass();
}

void AStolematesLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	AStolematesLobbyGameState* LobbyGameState = GetGameState<AStolematesLobbyGameState>();

	if (LobbyGameState)
	{
		LobbyGameState->BroadcastPlayerListChanged();
	}
}

void AStolematesLobbyGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	AStolematesLobbyGameState* LobbyGameState = GetGameState<AStolematesLobbyGameState>();

	if (LobbyGameState)
	{
		LobbyGameState->BroadcastPlayerListChanged();
	}
}



void AStolematesLobbyGameMode::StartOnlineMatch()
{
	AStolematesLobbyGameState* LobbyGameState =
		GetGameState<AStolematesLobbyGameState>();

	const int32 PlayerCount =
		LobbyGameState
		? FMath::Max(LobbyGameState->PlayerArray.Num(), 1)
		: 1;

	UStolematesGameInstance* StolematesGI =
		Cast<UStolematesGameInstance>(GetGameInstance());

	if (StolematesGI)
	{
		StolematesGI->ExpectedOnlinePlayerCount = PlayerCount;
	}

	if (GetWorld())
	{
		GetWorld()->ServerTravel(
			TEXT("/Game/Levels/Final_Level")
		);
	}
}