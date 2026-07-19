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

class FOnlineFriend;

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

	UPROPERTY(BlueprintReadWrite, Category = "Online")
		int32 ExpectedOnlinePlayerCount = 1;

	UFUNCTION(BlueprintCallable, Category = "Online")
		void HostOnlineGame();

	UFUNCTION(BlueprintCallable, Category = "Online")
		void FindJoinableSessions();

	UFUNCTION(BlueprintCallable, Category = "Online")
		void LeaveOnlineGame();

	UFUNCTION(BlueprintCallable, Category = "Game Flow")
		void ReturnToMainMenu();

	UFUNCTION(BlueprintCallable, Category = "Online")
		int32 GetFoundSessionCount() const;

	UFUNCTION(BlueprintCallable, Category = "Online")
		FString GetFoundSessionName(int32 SessionIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Online")
		int32 GetFoundSessionCurrentPlayers(int32 SessionIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Online")
		int32 GetFoundSessionMaxPlayers(int32 SessionIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Online")
		bool IsFoundSessionFull(int32 SessionIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Online")
		void JoinOnlineSessionByIndex(int32 SessionIndex);

	UFUNCTION(BlueprintImplementableEvent, Category = "Online")
		void OnJoinableSessionsUpdated();

	UFUNCTION(BlueprintCallable, Category = "Online")
		void LeaveOnlineLobby();

	UFUNCTION(BlueprintCallable, Category = "Menu")
		bool ConsumeOpenPlayMenuOnHUDLoad();

	UFUNCTION(BlueprintCallable, Category = "Steam Controller")
		void PollSteamControllerActions();

	UFUNCTION(BlueprintCallable, Category = "Steam Controller")
		void PollSteamMenuActions();

	UFUNCTION(BlueprintCallable, Category = "Steam Input")
		bool ConsumeSteamMenuPause();

	UFUNCTION(BlueprintCallable, Category = "Steam Controller")
		int32 ConsumeSteamMenuMove();

	UFUNCTION(BlueprintCallable, Category = "Steam Controller")
		bool ConsumeSteamMenuAccept();

	UFUNCTION(BlueprintCallable, Category = "Steam Controller")
		bool ConsumeSteamMenuBack();

	int32 PendingSteamMenuMove = 0;
	bool bPendingSteamMenuPause = false;
	bool bPendingSteamMenuAccept = false;
	bool bPendingSteamMenuBack = false;

	TSet<uint64> SteamPauseHeldHandles;

private:

	TSharedPtr<FOnlineSessionSettings> SessionSettings;
	TSharedPtr<FOnlineSessionSearch> SessionSearch;

	FDelegateHandle CreateSessionCompleteDelegateHandle;
	FDelegateHandle FindSessionsCompleteDelegateHandle;
	FDelegateHandle JoinSessionCompleteDelegateHandle;
	FDelegateHandle DestroySessionCompleteDelegateHandle;

	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void RemoveExtraLocalPlayers();

	void HandleNetworkFailure(
		UWorld* World,
		UNetDriver* NetDriver,
		ENetworkFailure::Type FailureType,
		const FString& ErrorString
	);

	TArray<int32> FriendSessionIndices;

	void OnReadFriendsListComplete(
		int32 LocalUserNum,
		bool bWasSuccessful,
		const FString& ListName,
		const FString& ErrorStr
	);

	bool IsSessionOwnerFriend(
		const FOnlineSessionSearchResult& SearchResult,
		const TArray<TSharedRef<FOnlineFriend>>& Friends
	) const;

	bool GetSearchResultIndexFromDisplayIndex(
		int32 DisplayIndex,
		int32& OutSearchResultIndex
	) const;

	void LeaveOnlineGameInternal(bool bReturnToPlayMenu);
	void ReturnToHUDLevel(bool bReturnToPlayMenu);

	bool bOpenPlayMenuOnHUDLoad = false;
	bool bReturnToPlayMenuAfterDestroy = false;

};