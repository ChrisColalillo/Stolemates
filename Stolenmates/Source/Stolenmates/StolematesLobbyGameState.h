// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "StolematesLobbyGameState.generated.h"

/**
 * 
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerListChangedDelegate);

UCLASS()
class STOLENMATES_API AStolematesLobbyGameState : public AGameStateBase
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintAssignable, Category = "Lobby")
		FOnPlayerListChangedDelegate OnPlayerListChanged;

	UFUNCTION(BlueprintCallable, Category = "Lobby")
		void BroadcastPlayerListChanged();
};
