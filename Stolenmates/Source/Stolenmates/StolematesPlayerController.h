// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "StolematesPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class STOLENMATES_API AStolematesPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Pause")
		void RequestPauseGame();

	UFUNCTION(BlueprintCallable, Category = "Pause")
		void RequestResumeGame();

	UFUNCTION(Server, Reliable)
		void ServerRequestPauseGame();

	UFUNCTION(Server, Reliable)
		void ServerRequestResumeGame();

	UFUNCTION(Client, Reliable)
		void ClientShowInGameHUD(int32 PlayerCount, float TimeToWin);

	UFUNCTION(Client, Reliable)
		void ClientShowGameOverHUD(int32 WinnerIndex);

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
		void ShowInGameHUD(int32 PlayerCount, float TimeToWin);

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
		void ShowGameOverHUD(int32 WinnerIndex);

	UFUNCTION(Client, Reliable)
		void ClientUpdatePowerUpHUD(int32 PlayerIndex, int32 PowerUpIndex);

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
		void UpdatePowerUpHUD(int32 PlayerIndex, int32 PowerUpIndex);

	UFUNCTION(Client, Reliable)
		void ClientSetSharedCamera(AActor* CameraActor);

	UFUNCTION(BlueprintCallable, Category = "Pause")
		void RequestLeaveToMainMenu();

	UFUNCTION(Server, Reliable)
		void ServerRequestLeaveToMainMenu();

	UFUNCTION(Client, Reliable)
		void ClientHandleReturnToMainMenu();

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
		void ReturnToMainMenuRequested();

	UFUNCTION(Client, Reliable)
		void ClientShowOnlinePauseMenu(bool bCanResume);

	UFUNCTION(Client, Reliable)
		void ClientHideOnlinePauseMenu();

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
		void ShowOnlinePauseMenu(bool bCanResume);

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
		void HideOnlinePauseMenu();

	UFUNCTION(Client, Reliable)
		void ClientSetMatchStartMessage(const FString& Message, bool bVisible);

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
		void SetMatchStartMessage(const FString& Message, bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "Match Start")
		void MarkMatchStartReady();

	UFUNCTION(Server, Reliable)
		void ServerMarkMatchStartReady();
};
