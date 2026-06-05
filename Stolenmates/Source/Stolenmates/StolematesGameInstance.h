// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"

#include "StolematesGameInstance.generated.h"

/**
 * 
 */

UENUM(BlueprintType)
enum class EMatchType : uint8
{
	Local UMETA(DisplayName = "Local"),
	Online UMETA(DisplayName = "Online")
};

UCLASS()
class STOLENMATES_API UStolematesGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:

	virtual void Init() override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Game Mode")
		EMatchType MatchType = EMatchType::Local;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Game Mode")
		int32 LocalPlayerCount = 4;

	UFUNCTION(BlueprintCallable, Category = "Online")
		void HostOnlineGame();

	UFUNCTION(BlueprintCallable, Category = "Online")
		void FindJoinableSessions();

	UFUNCTION(BlueprintCallable, Category = "Online")
		void JoinOnlineSession();

	UFUNCTION(BlueprintCallable, Category = "Online")
		void LeaveOnlineGame();

private:

	TSharedPtr<FOnlineSessionSettings> SessionSettings;
	TSharedPtr<FOnlineSessionSearch> SessionSearch;

	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	void HandleNetworkFailure(
		UWorld* World,
		UNetDriver* NetDriver,
		ENetworkFailure::Type FailureType,
		const FString& ErrorString
	);

};