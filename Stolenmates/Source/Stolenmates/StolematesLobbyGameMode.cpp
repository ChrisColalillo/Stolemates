// Fill out your copyright notice in the Description page of Project Settings.


#include "StolematesLobbyGameMode.h"
#include "StolematesLobbyGameState.h"

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