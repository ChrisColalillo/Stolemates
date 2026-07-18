// Fill out your copyright notice in the Description page of Project Settings.


#include "StolematesPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "StolenmatesGameModeBase.h"

void AStolematesPlayerController::RequestPauseGame()
{
	ServerRequestPauseGame();
}

void AStolematesPlayerController::RequestResumeGame()
{
	ServerRequestResumeGame();
}

void AStolematesPlayerController::ServerRequestPauseGame_Implementation()
{
	AStolenmatesGameModeBase* GameMode =
		Cast<AStolenmatesGameModeBase>(UGameplayStatics::GetGameMode(this));

	if (GameMode)
	{
		GameMode->RequestOnlinePause(this);
	}
}

void AStolematesPlayerController::ServerRequestResumeGame_Implementation()
{
	AStolenmatesGameModeBase* GameMode =
		Cast<AStolenmatesGameModeBase>(UGameplayStatics::GetGameMode(this));

	if (GameMode)
	{
		GameMode->RequestOnlineResume(this);
	}
}

void AStolematesPlayerController::ClientShowInGameHUD_Implementation(int32 PlayerCount, float TimeToWin)
{
	ShowInGameHUD(PlayerCount, TimeToWin);
}

void AStolematesPlayerController::ClientShowGameOverHUD_Implementation(int32 WinnerIndex)
{
	ShowGameOverHUD(WinnerIndex);
}


void AStolematesPlayerController::ClientUpdatePowerUpHUD_Implementation(int32 PlayerIndex, int32 PowerUpIndex)
{
	UpdatePowerUpHUD(PlayerIndex, PowerUpIndex);
}



void AStolematesPlayerController::ClientSetSharedCamera_Implementation(AActor* CameraActor)
{
	if (CameraActor)
	{
		SetViewTargetWithBlend(CameraActor);
	}
}



void AStolematesPlayerController::RequestLeaveToMainMenu()
{
	ServerRequestLeaveToMainMenu();
}

void AStolematesPlayerController::ServerRequestLeaveToMainMenu_Implementation()
{
	AStolenmatesGameModeBase* GameMode =
		Cast<AStolenmatesGameModeBase>(UGameplayStatics::GetGameMode(this));

	if (GameMode)
	{
		GameMode->HandlePlayerLeavingMatch(this);
	}

	ClientHandleReturnToMainMenu();
}

void AStolematesPlayerController::ClientHandleReturnToMainMenu_Implementation()
{
	ReturnToMainMenuRequested();
}

void AStolematesPlayerController::ClientShowOnlinePauseMenu_Implementation(bool bCanResume)
{
	ShowOnlinePauseMenu(bCanResume);
}

void AStolematesPlayerController::ClientHideOnlinePauseMenu_Implementation()
{
	HideOnlinePauseMenu();
}

void AStolematesPlayerController::ClientSetMatchStartMessage_Implementation(const FString& Message, bool bVisible)
{
	SetMatchStartMessage(Message, bVisible);
}

void AStolematesPlayerController::MarkMatchStartReady()
{
	ServerMarkMatchStartReady();
}

void AStolematesPlayerController::ServerMarkMatchStartReady_Implementation()
{
	AStolenmatesGameModeBase* GM = GetWorld() ? GetWorld()->GetAuthGameMode<AStolenmatesGameModeBase>() : nullptr;

	if (GM)
	{
		GM->NotifyPlayerReadyForMatchStart(this);
	}
}