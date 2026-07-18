// Fill out your copyright notice in the Description page of Project Settings.


#include "StolematesLobbyGameState.h"

void AStolematesLobbyGameState::BroadcastPlayerListChanged_Implementation()
{
	OnPlayerListChanged.Broadcast();
}
