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

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Game Mode")
	TSubclassOf<AActor> PlayerSpawners;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Game Mode")
	TSubclassOf<AActor> BPCamera;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Game Mode")
	TSubclassOf<AStolenmatesPlayer> BPPlayer;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Game Mode")
	TArray<UMaterialInterface*> PlayerMaterials;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Game Mode")
	float TimeToWin = 45.0f;

	ACamera* camera;
	TArray<ACharacter*> players;
	FTimerHandle powerUpSpawnTimerHandle;
	bool gameOver = false;
public:
	AStolenmatesGameModeBase();
	virtual void StartPlay() override;
	virtual void Tick(float DeltaTime) override;
};
