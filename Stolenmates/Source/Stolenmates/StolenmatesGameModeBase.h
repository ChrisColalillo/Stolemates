// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "StolenmatesGameModeBase.generated.h"

/**
 * 
 */
class ACamera;
class AStolenmatesPlayer;
class UMaterialInterface;
UCLASS()
class STOLENMATES_API AStolenmatesGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Game Mode")
	void SpawnPowerUp();
	UFUNCTION(BlueprintImplementableEvent, Category = "Game Mode")
	void LoadHud(APlayerController* PlayerController);
	UFUNCTION(BlueprintImplementableEvent, Category = "Game Mode")
	void GameOverHud(APlayerController* PlayerController);
	UFUNCTION(BlueprintCallable, Category = "Game Mode")
	void playAgain();
	UFUNCTION(BlueprintCallable, Category = "Game Mode")
	int getWinner();
	UFUNCTION(BlueprintCallable, Category = "Game Mode")
	float getTimer(int i);
	UFUNCTION(BlueprintCallable, Category = "Game Mode")
	TArray<ACharacter*> getPlayers();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Game Mode")
	TSubclassOf<AActor> PlayerSpawners;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Game Mode")
	TSubclassOf<AActor> BPCamera;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Game Mode")
	TSubclassOf<AStolenmatesPlayer> BPPlayer;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Game Mode")
	TArray<UMaterialInterface*> PlayerMaterials;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Game Mode")
	TArray<UMaterialInterface*> PlayerDecals;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Game Mode")
	float TimeToWin = 45.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Game Mode")
	float cameraStartHeight = 1000.0f;

	UPROPERTY()
		APlayerController* PlayerWhoPaused = nullptr;

	void TryStartOnlineGame();

	bool bOnlineGameStarted = false;

	ACamera* camera;
	TArray<ACharacter*> players;
	FTimerHandle powerUpSpawnTimerHandle;
	bool gameOver = false;

	FTimerHandle MatchStartUnlockTimerHandle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Match Start")
		float MatchStartDelay = 3.0f;

	void SetAllPlayersInputLocked(bool bLocked);

	UFUNCTION()
		void UnlockPlayersForMatchStart();

	void BroadcastMatchStartMessage(const FString& Message, bool bVisible);

	UPROPERTY()
		TArray<APlayerController*> MatchStartReadyControllers;

	bool bWaitingForMatchStartReady = false;
	int32 ExpectedMatchStartPlayerCount = 1;

public:
	AStolenmatesGameModeBase();

	void StartLocalGame();
	void StartOnlineGame();

	bool RequestOnlinePause(APlayerController* RequestingPlayer);
	bool RequestOnlineResume(APlayerController* RequestingPlayer);

	void BroadcastOnlinePauseMenu(APlayerController* PauseOwner);
	void BroadcastOnlineResume();

	virtual void StartPlay() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "HUD")
		void BroadcastPowerUpHUDUpdate(int32 PlayerIndex, int32 PowerUpIndex);

	UFUNCTION()
		void HandlePlayerLeavingMatch(APlayerController* LeavingPlayer);

	void NotifyPlayerReadyForMatchStart(APlayerController* ReadyPlayerController);
	void CheckAllPlayersReadyForMatchStart();
};
