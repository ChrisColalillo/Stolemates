// Fill out your copyright notice in the Description page of Project Settings.


#include "StolenmatesGameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Components/SkeletalMeshComponent.h"
#include "StolenmatesPlayer.h"
#include "Camera.h"
#include "Materials/Material.h"
#include "TimerManager.h"


AStolenmatesGameModeBase::AStolenmatesGameModeBase()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.0f;
}

void AStolenmatesGameModeBase::StartPlay()
{
	Super::StartPlay();
	camera = Cast<ACamera>(GetWorld()->SpawnActor<AActor>(BPCamera,FVector(0,0,1000) , FRotator::ZeroRotator));
	TArray<AActor*> Locations;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), PlayerSpawners,Locations);
	for(int32 i =0;i< Locations.Num();i++)
	{		
		AStolenmatesPlayer* player = GetWorld()->SpawnActor<AStolenmatesPlayer>(BPPlayer,Locations[i]->GetActorLocation(), Locations[i]->GetActorRotation());
		player->GetMesh()->SetMaterial(0,PlayerMaterials[i]);
		UGameplayStatics::CreatePlayer(this, i, true);
		UGameplayStatics::GetPlayerController(player, i)->Possess(player);
		UGameplayStatics::GetPlayerController(player, i)->SetViewTargetWithBlend(camera);
		players.Push(player);
	}
	LoadHud(UGameplayStatics::GetPlayerController(players[0], 0));
	GetWorldTimerManager().SetTimer(powerUpSpawnTimerHandle, this, &AStolenmatesGameModeBase::SpawnPowerUp, 5.0f, true);
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

