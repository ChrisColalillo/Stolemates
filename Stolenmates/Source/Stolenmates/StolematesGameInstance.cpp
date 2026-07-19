// Fill out your copyright notice in the Description page of Project Settings.


#include "StolematesGameInstance.h"

#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Interfaces/OnlineFriendsInterface.h"
#include "StolenmatesPlayer.h"
#include "steam/steam_api.h"

static AStolenmatesPlayer* FindStolematesPlayerByLocalControllerIndex(UObject* WorldContextObject, int32 LocalControllerIndex)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	APlayerController* PlayerController =
		UGameplayStatics::GetPlayerController(WorldContextObject, LocalControllerIndex);

	if (!PlayerController)
	{
		return nullptr;
	}

	return Cast<AStolenmatesPlayer>(PlayerController->GetPawn());
}

void UStolematesGameInstance::HostOnlineGame()
{
	RemoveExtraLocalPlayers();
	
	// Get the active online subsystem (Steam, Null, Xbox Live, etc.)
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();

	// If no subsystem exists, hosting cannot continue
	if (!Subsystem)
	{
		return;
	}

	// Get Unreal's session interface from the subsystem
	// This is the object responsible for creating, finding,
	// joining, and destroying multiplayer sessions.
	IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();

	// Safety check
	if (!SessionInterface.IsValid())
	{
		return;
	}

	// Create a new session settings object
	SessionSettings = MakeShareable(new FOnlineSessionSettings());

	// Steam session settings
	SessionSettings->bIsLANMatch = false;              // Use Steam, not LAN
	SessionSettings->NumPublicConnections = 4;         // Maximum players
	SessionSettings->bShouldAdvertise = true;          // Allow other players to find this session
	SessionSettings->bUsesPresence = true;             // Required for Steam presence sessions
	SessionSettings->bAllowJoinInProgress = true;      // Allow players to join after session creation
	SessionSettings->bAllowJoinViaPresence = true;     // Allow joining through Steam friends

	// Register a callback.
	// When Steam finishes creating the session,
	// Unreal will automatically call OnCreateSessionComplete().
	if (CreateSessionCompleteDelegateHandle.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		CreateSessionCompleteDelegateHandle.Reset();
	}

	CreateSessionCompleteDelegateHandle =
		SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
			FOnCreateSessionCompleteDelegate::CreateUObject(
				this,
				&UStolematesGameInstance::OnCreateSessionComplete
			)
		);

	// Ask Steam to create a session named GameSession
	SessionInterface->CreateSession(
		0,                  // Local player index
		NAME_GameSession,   // Session name
		*SessionSettings    // Settings defined above
	);
}



void UStolematesGameInstance::OnCreateSessionComplete(
	FName SessionName,
	bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		// Session is now advertised through Steam.
		// Open the lobby level as a listen server so other players can join.
		UGameplayStatics::OpenLevel(
			GetWorld(),
			FName("/Game/Levels/LOBBYLEVEL"),
			true,
			"listen"
		);
	}
}



void UStolematesGameInstance::FindJoinableSessions()
{
	RemoveExtraLocalPlayers();
	FriendSessionIndices.Empty();

	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();

	if (!Subsystem)
	{
		return;
	}

	IOnlineFriendsPtr FriendsInterface = Subsystem->GetFriendsInterface();

	if (!FriendsInterface.IsValid())
	{
		return;
	}

	FriendsInterface->ReadFriendsList(
		0,
		EFriendsLists::ToString(EFriendsLists::Default),
		FOnReadFriendsListComplete::CreateUObject(
			this,
			&UStolematesGameInstance::OnReadFriendsListComplete
		)
	);
}

void UStolematesGameInstance::OnReadFriendsListComplete(
	int32 LocalUserNum,
	bool bWasSuccessful,
	const FString& ListName,
	const FString& ErrorStr
)
{
	if (!bWasSuccessful)
	{
		OnJoinableSessionsUpdated();
		return;
	}

	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();

	if (!Subsystem)
	{
		OnJoinableSessionsUpdated();
		return;
	}

	IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();

	if (!SessionInterface.IsValid())
	{
		OnJoinableSessionsUpdated();
		return;
	}

	SessionSearch = MakeShareable(new FOnlineSessionSearch());

	SessionSearch->bIsLanQuery = false;
	SessionSearch->MaxSearchResults = 50;

	SessionSearch->QuerySettings.Set(
		SEARCH_PRESENCE,
		true,
		EOnlineComparisonOp::Equals
	);

	if (FindSessionsCompleteDelegateHandle.IsValid())
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		FindSessionsCompleteDelegateHandle.Reset();
	}

	FindSessionsCompleteDelegateHandle =
		SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
			FOnFindSessionsCompleteDelegate::CreateUObject(
				this,
				&UStolematesGameInstance::OnFindSessionsComplete
			)
		);

	SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
}

void UStolematesGameInstance::OnFindSessionsComplete(bool bWasSuccessful)
{
	FriendSessionIndices.Empty();

	if (!bWasSuccessful || !SessionSearch.IsValid())
	{
		OnJoinableSessionsUpdated();
		return;
	}

	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();

	if (!Subsystem)
	{
		OnJoinableSessionsUpdated();
		return;
	}

	IOnlineFriendsPtr FriendsInterface = Subsystem->GetFriendsInterface();

	if (!FriendsInterface.IsValid())
	{
		OnJoinableSessionsUpdated();
		return;
	}

	TArray<TSharedRef<FOnlineFriend>> Friends;

	const bool bGotFriends = FriendsInterface->GetFriendsList(
		0,
		EFriendsLists::ToString(EFriendsLists::Default),
		Friends
	);

	if (!bGotFriends)
	{
		OnJoinableSessionsUpdated();
		return;
	}

	for (int32 i = 0; i < SessionSearch->SearchResults.Num(); i++)
	{
		const FOnlineSessionSearchResult& Result = SessionSearch->SearchResults[i];

		if (IsSessionOwnerFriend(Result, Friends))
		{
			FriendSessionIndices.Add(i);
		}
	}

	OnJoinableSessionsUpdated();
}

bool UStolematesGameInstance::IsSessionOwnerFriend(
	const FOnlineSessionSearchResult& SearchResult,
	const TArray<TSharedRef<FOnlineFriend>>& Friends
) const
{
	if (!SearchResult.Session.OwningUserId.IsValid())
	{
		return false;
	}

	const FString SessionOwnerId = SearchResult.Session.OwningUserId->ToString();

	for (const TSharedRef<FOnlineFriend>& Friend : Friends)
	{
		const FString FriendId = Friend->GetUserId()->ToString();

		if (FriendId == SessionOwnerId)
		{
			return true;
		}
	}

	return false;
}

bool UStolematesGameInstance::GetSearchResultIndexFromDisplayIndex(
	int32 DisplayIndex,
	int32& OutSearchResultIndex
) const
{
	OutSearchResultIndex = INDEX_NONE;

	if (!SessionSearch.IsValid())
	{
		return false;
	}

	if (!FriendSessionIndices.IsValidIndex(DisplayIndex))
	{
		return false;
	}

	const int32 SearchResultIndex = FriendSessionIndices[DisplayIndex];

	if (!SessionSearch->SearchResults.IsValidIndex(SearchResultIndex))
	{
		return false;
	}

	OutSearchResultIndex = SearchResultIndex;
	return true;
}

int32 UStolematesGameInstance::GetFoundSessionCount() const
{
	return FriendSessionIndices.Num();
}

FString UStolematesGameInstance::GetFoundSessionName(int32 SessionIndex) const
{
	int32 SearchResultIndex;

	if (!GetSearchResultIndexFromDisplayIndex(SessionIndex, SearchResultIndex))
	{
		return TEXT("Invalid Session");
	}

	const FOnlineSessionSearchResult& Result = SessionSearch->SearchResults[SearchResultIndex];

	FString HostName = Result.Session.OwningUserName;

	if (HostName.IsEmpty())
	{
		HostName = TEXT("Friend");
	}

	return HostName + TEXT("'s Lobby");
}

int32 UStolematesGameInstance::GetFoundSessionMaxPlayers(int32 SessionIndex) const
{
	int32 SearchResultIndex;

	if (!GetSearchResultIndexFromDisplayIndex(SessionIndex, SearchResultIndex))
	{
		return 0;
	}

	return SessionSearch->SearchResults[SearchResultIndex].Session.SessionSettings.NumPublicConnections;
}

int32 UStolematesGameInstance::GetFoundSessionCurrentPlayers(int32 SessionIndex) const
{
	int32 SearchResultIndex;

	if (!GetSearchResultIndexFromDisplayIndex(SessionIndex, SearchResultIndex))
	{
		return 0;
	}

	const FOnlineSessionSearchResult& Result = SessionSearch->SearchResults[SearchResultIndex];

	const int32 MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;
	const int32 OpenConnections = Result.Session.NumOpenPublicConnections;

	return MaxPlayers - OpenConnections;
}

bool UStolematesGameInstance::IsFoundSessionFull(int32 SessionIndex) const
{
	int32 SearchResultIndex;

	if (!GetSearchResultIndexFromDisplayIndex(SessionIndex, SearchResultIndex))
	{
		return true;
	}

	return SessionSearch->SearchResults[SearchResultIndex].Session.NumOpenPublicConnections <= 0;
}

void UStolematesGameInstance::JoinOnlineSessionByIndex(int32 SessionIndex)
{
	int32 SearchResultIndex;

	if (!GetSearchResultIndexFromDisplayIndex(SessionIndex, SearchResultIndex))
	{
		return;
	}

	if (IsFoundSessionFull(SessionIndex))
	{
		return;
	}

	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();

	if (!Subsystem)
	{
		return;
	}

	IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();

	if (!SessionInterface.IsValid())
	{
		return;
	}

	if (JoinSessionCompleteDelegateHandle.IsValid())
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		JoinSessionCompleteDelegateHandle.Reset();
	}

	JoinSessionCompleteDelegateHandle =
		SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
			FOnJoinSessionCompleteDelegate::CreateUObject(
				this,
				&UStolematesGameInstance::OnJoinSessionComplete
			)
		);

	SessionInterface->JoinSession(
		0,
		NAME_GameSession,
		SessionSearch->SearchResults[SearchResultIndex]
	);
}


void UStolematesGameInstance::OnJoinSessionComplete(
	FName SessionName,
	EOnJoinSessionCompleteResult::Type Result)
{
	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		return;
	}

	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();

	if (!Subsystem)
	{
		return;
	}

	IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();

	if (!SessionInterface.IsValid())
	{
		return;
	}

	FString TravelURL;

	if (!SessionInterface->GetResolvedConnectString(SessionName, TravelURL))
	{
		return;
	}

	APlayerController* PlayerController = GetFirstLocalPlayerController();

	if (!PlayerController)
	{
		return;
	}

	PlayerController->ClientTravel(
		TravelURL,
		ETravelType::TRAVEL_Absolute
	);
}



void UStolematesGameInstance::LeaveOnlineGameInternal(bool bReturnToPlayMenu)
{
	bReturnToPlayMenuAfterDestroy = bReturnToPlayMenu;

	// Get the active online subsystem.
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();

	if (!Subsystem)
	{
		// Even if there is no online subsystem, return to main menu.
		ReturnToHUDLevel(bReturnToPlayMenu);

		return;
	}

	// Get the session interface.
	IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();

	if (!SessionInterface.IsValid())
	{
		// Even if session cleanup fails, return to main menu.
		ReturnToHUDLevel(bReturnToPlayMenu);

		return;
	}

	if (DestroySessionCompleteDelegateHandle.IsValid())
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
		DestroySessionCompleteDelegateHandle.Reset();
	}

	DestroySessionCompleteDelegateHandle =
		SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
			FOnDestroySessionCompleteDelegate::CreateUObject(
				this,
				&UStolematesGameInstance::OnDestroySessionComplete
			)
		);

	// Destroy the current game session.
	SessionInterface->DestroySession(NAME_GameSession);
}



void UStolematesGameInstance::OnDestroySessionComplete(
	FName SessionName,
	bool bWasSuccessful)
{
	// After leaving/destroying the session, return to the main menu level.
	const bool bShouldReturnToPlayMenu = bReturnToPlayMenuAfterDestroy;
	bReturnToPlayMenuAfterDestroy = false;

	ReturnToHUDLevel(bShouldReturnToPlayMenu);
}



void UStolematesGameInstance::Init()
{
	Super::Init();

	if (GEngine)
	{
		GEngine->OnNetworkFailure().AddUObject(
			this,
			&UStolematesGameInstance::HandleNetworkFailure
		);
	}
}



void UStolematesGameInstance::HandleNetworkFailure(
	UWorld* World,
	UNetDriver* NetDriver,
	ENetworkFailure::Type FailureType,
	const FString& ErrorString
)
{
	// If the host leaves or connection is lost, return the client to the menu.
	UGameplayStatics::OpenLevel(
		this,
		FName("/Game/Levels/HUDLEVEL")
	);
}



void UStolematesGameInstance::RemoveExtraLocalPlayers()
{
	if (!GetWorld())
	{
		return;
	}

	const TArray<ULocalPlayer*>& LocalPlayers = GetLocalPlayers();

	// Remove players from the end so indexes do not shift while looping.
	// Keep player 0. Remove player 1, 2, 3, etc.
	for (int32 i = LocalPlayers.Num() - 1; i >= 1; i--)
	{
		if (!LocalPlayers[i])
		{
			continue;
		}

		APlayerController* PlayerController = LocalPlayers[i]->PlayerController;

		if (PlayerController)
		{
			UGameplayStatics::RemovePlayer(PlayerController, true);
		}
	}
}



void UStolematesGameInstance::ReturnToMainMenu()
{
	// Clean up extra local players created by local multiplayer.
	RemoveExtraLocalPlayers();

	if (MatchType == EMatchType::Online)
	{
		// Online games/lobbies need session cleanup.
		LeaveOnlineGame();
		return;
	}

	UGameplayStatics::OpenLevel(
		GetWorld(),
		FName("/Game/Levels/HUDLEVEL")
	);
}



void UStolematesGameInstance::LeaveOnlineGame()
{
	LeaveOnlineGameInternal(false);
}

void UStolematesGameInstance::LeaveOnlineLobby()
{
	LeaveOnlineGameInternal(true);
}



void UStolematesGameInstance::ReturnToHUDLevel(bool bReturnToPlayMenu)
{
	bOpenPlayMenuOnHUDLoad = bReturnToPlayMenu;

	UGameplayStatics::OpenLevel(
		GetWorld(),
		FName("/Game/Levels/HUDLEVEL")
	);
}



bool UStolematesGameInstance::ConsumeOpenPlayMenuOnHUDLoad()
{
	const bool bShouldOpenPlayMenu = bOpenPlayMenuOnHUDLoad;

	bOpenPlayMenuOnHUDLoad = false;
	return bShouldOpenPlayMenu;
}


void UStolematesGameInstance::PollSteamControllerActions()
{
#if PLATFORM_WINDOWS
	
	if (!SteamAPI_IsSteamRunning())
	{
		return;
	}

	if (!SteamController())
	{
		return;
	}

	static bool bInitializedSteamController = false;

	if (!bInitializedSteamController)
	{
		bInitializedSteamController = SteamController()->Init();
	}

	SteamController()->RunFrame();

	const ControllerActionSetHandle_t GameplayActionSet =
		SteamController()->GetActionSetHandle("Gameplay");

	const ControllerDigitalActionHandle_t JumpAction =
		SteamController()->GetDigitalActionHandle("Jump");

	const ControllerDigitalActionHandle_t UseAbilityAction =
		SteamController()->GetDigitalActionHandle("UseAbility");

	const ControllerDigitalActionHandle_t PauseAction =
		SteamController()->GetDigitalActionHandle("Pause");

	const ControllerAnalogActionHandle_t MoveAction =
		SteamController()->GetAnalogActionHandle("Move");

	if (GameplayActionSet == 0 ||
		JumpAction == 0 ||
		UseAbilityAction == 0 ||
		PauseAction == 0 ||
		MoveAction == 0)
	{
		return;
	}

	ControllerHandle_t ControllerHandles[STEAM_CONTROLLER_MAX_COUNT];
	const int32 NumControllers = SteamController()->GetConnectedControllers(ControllerHandles);

	static TMap<uint64, bool> LastJumpStateByHandle;
	static TMap<uint64, bool> LastUseAbilityStateByHandle;

	if (NumControllers <= 0)
	{
		LastJumpStateByHandle.Empty();
		LastUseAbilityStateByHandle.Empty();
		SteamPauseHeldHandles.Empty();
		return;
	}

	for (int32 i = 0; i < NumControllers; i++)
	{
		const ControllerHandle_t Handle = ControllerHandles[i];
		const uint64 HandleKey = static_cast<uint64>(Handle);

		SteamController()->ActivateActionSet(Handle, GameplayActionSet);

		const int32 LocalControllerIndex =
			(MatchType == EMatchType::Online) ? 0 : i;

		AStolenmatesPlayer* Player =
			FindStolematesPlayerByLocalControllerIndex(this, LocalControllerIndex);

		// Move
		const ControllerAnalogActionData_t MoveData =
			SteamController()->GetAnalogActionData(Handle, MoveAction);

		float MoveRight = MoveData.bActive ? MoveData.x : 0.0f;
		float MoveForward = MoveData.bActive ? MoveData.y : 0.0f;

		const float DeadZone = 0.15f;

		if (FMath::Abs(MoveRight) < DeadZone)
		{
			MoveRight = 0.0f;
		}

		if (FMath::Abs(MoveForward) < DeadZone)
		{
			MoveForward = 0.0f;
		}

		if (Player)
		{
			Player->SteamActionMove(MoveRight, MoveForward);
		}

		// Jump
		const ControllerDigitalActionData_t JumpData =
			SteamController()->GetDigitalActionData(Handle, JumpAction);

		const bool bJumpDown =
			JumpData.bActive && JumpData.bState;

		const bool bLastJumpDown =
			LastJumpStateByHandle.Contains(HandleKey)
			? LastJumpStateByHandle[HandleKey]
			: false;

		if (bJumpDown && !bLastJumpDown)
		{
			if (Player)
			{
				Player->SteamActionJumpPressed();
			}
		}
		else if (!bJumpDown && bLastJumpDown)
		{
			if (Player)
			{
				Player->SteamActionJumpReleased();
			}
		}

		LastJumpStateByHandle.Add(HandleKey, bJumpDown);

		// Use Ability
		const ControllerDigitalActionData_t UseAbilityData =
			SteamController()->GetDigitalActionData(Handle, UseAbilityAction);

		const bool bUseAbilityDown =
			UseAbilityData.bActive && UseAbilityData.bState;

		const bool bLastUseAbilityDown =
			LastUseAbilityStateByHandle.Contains(HandleKey)
			? LastUseAbilityStateByHandle[HandleKey]
			: false;

		if (bUseAbilityDown && !bLastUseAbilityDown)
		{
			if (Player)
			{
				Player->SteamActionUseAbilityPressed();
			}
		}

		LastUseAbilityStateByHandle.Add(HandleKey, bUseAbilityDown);

		// Pause
		const ControllerDigitalActionData_t PauseData =
			SteamController()->GetDigitalActionData(Handle, PauseAction);

		const bool bPauseActive = PauseData.bActive;
		const bool bPauseDown = PauseData.bState;

		// Only an active Pause action reporting Up counts as a real release.
		// An inactive action during an action-set transition must not clear the latch.
		if (bPauseActive && !bPauseDown)
		{
			SteamPauseHeldHandles.Remove(HandleKey);
		}
		else if (bPauseActive &&
			bPauseDown &&
			!SteamPauseHeldHandles.Contains(HandleKey))
		{
			SteamPauseHeldHandles.Add(HandleKey);

			if (Player)
			{
				Player->SteamActionPausePressed();
			}
		}

	}
#endif
}

void UStolematesGameInstance::PollSteamMenuActions()
{
#if PLATFORM_WINDOWS
	if (!SteamAPI_IsSteamRunning())
	{
		return;
	}

	if (!SteamController())
	{
		return;
	}

	static bool bInitializedSteamController = false;

	if (!bInitializedSteamController)
	{
		bInitializedSteamController = SteamController()->Init();
	}

	SteamController()->RunFrame();

	const ControllerActionSetHandle_t MenuActionSet =
		SteamController()->GetActionSetHandle("Menu");

	const ControllerDigitalActionHandle_t MenuAcceptAction =
		SteamController()->GetDigitalActionHandle("MenuAccept");

	const ControllerDigitalActionHandle_t MenuBackAction =
		SteamController()->GetDigitalActionHandle("MenuBack");

	const ControllerDigitalActionHandle_t MenuPauseAction =
		SteamController()->GetDigitalActionHandle("MenuPause");

	const ControllerAnalogActionHandle_t MenuMoveAction =
		SteamController()->GetAnalogActionHandle("MenuMove");

	if (MenuActionSet == 0 ||
		MenuAcceptAction == 0 ||
		MenuBackAction == 0 ||
		MenuPauseAction == 0 ||
		MenuMoveAction == 0)
	{
		return;
	}

	ControllerHandle_t ControllerHandles[STEAM_CONTROLLER_MAX_COUNT];
	const int32 NumControllers = SteamController()->GetConnectedControllers(ControllerHandles);

	static TMap<uint64, bool> LastAcceptStateByHandle;
	static TMap<uint64, bool> LastBackStateByHandle;
	static TMap<uint64, FString> LastMoveDirectionByHandle;

	if (NumControllers <= 0)
	{
		LastAcceptStateByHandle.Empty();
		LastBackStateByHandle.Empty();
		LastMoveDirectionByHandle.Empty();
		SteamPauseHeldHandles.Empty();

		PendingSteamMenuMove = 0;
		bPendingSteamMenuPause = false;
		bPendingSteamMenuAccept = false;
		bPendingSteamMenuBack = false;

		return;
	}

	const float MoveThreshold = 0.55f;

	for (int32 i = 0; i < NumControllers; i++)
	{
		const ControllerHandle_t Handle = ControllerHandles[i];
		const uint64 HandleKey = static_cast<uint64>(Handle);

		SteamController()->ActivateActionSet(Handle, MenuActionSet);

		const ControllerDigitalActionData_t AcceptData =
			SteamController()->GetDigitalActionData(Handle, MenuAcceptAction);

		const bool bAcceptDown =
			AcceptData.bActive && AcceptData.bState;
		const bool bLastAcceptDown =
			LastAcceptStateByHandle.Contains(HandleKey)
			? LastAcceptStateByHandle[HandleKey]
			: false;

		if (bAcceptDown && !bLastAcceptDown)
		{
			bPendingSteamMenuAccept = true;
		}

		LastAcceptStateByHandle.Add(HandleKey, bAcceptDown);

		const ControllerDigitalActionData_t BackData =
			SteamController()->GetDigitalActionData(Handle, MenuBackAction);

		const bool bBackDown =
			BackData.bActive && BackData.bState;
		const bool bLastBackDown =
			LastBackStateByHandle.Contains(HandleKey)
			? LastBackStateByHandle[HandleKey]
			: false;

		if (bBackDown && !bLastBackDown)
		{
			bPendingSteamMenuBack = true;
		}

		LastBackStateByHandle.Add(HandleKey, bBackDown);

		// Menu Pause
		const ControllerDigitalActionData_t PauseData =
			SteamController()->GetDigitalActionData(Handle, MenuPauseAction);

		const bool bPauseActive = PauseData.bActive;
		const bool bPauseDown = PauseData.bState;

		// Only clear after the active MenuPause action sees a real release.
		if (bPauseActive && !bPauseDown)
		{
			SteamPauseHeldHandles.Remove(HandleKey);
		}
		else if (bPauseActive &&
			bPauseDown &&
			!SteamPauseHeldHandles.Contains(HandleKey))
		{
			SteamPauseHeldHandles.Add(HandleKey);
			bPendingSteamMenuPause = true;
		}
		
		// Menu Move
		const ControllerAnalogActionData_t MoveData =
			SteamController()->GetAnalogActionData(Handle, MenuMoveAction);

		FString CurrentDirection = TEXT("None");

		if (MoveData.bActive)
		{
			if (MoveData.y > MoveThreshold)
			{
				CurrentDirection = TEXT("Up");
			}
			else if (MoveData.y < -MoveThreshold)
			{
				CurrentDirection = TEXT("Down");
			}
			else if (MoveData.x < -MoveThreshold)
			{
				CurrentDirection = TEXT("Left");
			}
			else if (MoveData.x > MoveThreshold)
			{
				CurrentDirection = TEXT("Right");
			}
		}

		const FString LastDirection =
			LastMoveDirectionByHandle.Contains(HandleKey)
			? LastMoveDirectionByHandle[HandleKey]
			: TEXT("None");

		if (CurrentDirection != TEXT("None") && CurrentDirection != LastDirection)
		{
			if (CurrentDirection == TEXT("Up"))
			{
				PendingSteamMenuMove = -1;
			}
			else if (CurrentDirection == TEXT("Down"))
			{
				PendingSteamMenuMove = 1;
			}
			else if (CurrentDirection == TEXT("Left"))
			{
				PendingSteamMenuMove = -2;
			}
			else if (CurrentDirection == TEXT("Right"))
			{
				PendingSteamMenuMove = 2;
			}
		}

		LastMoveDirectionByHandle.Add(HandleKey, CurrentDirection);
	}
#endif
}


int32 UStolematesGameInstance::ConsumeSteamMenuMove()
{
	const int32 Result = PendingSteamMenuMove;
	PendingSteamMenuMove = 0;
	return Result;
}

bool UStolematesGameInstance::ConsumeSteamMenuAccept()
{
	const bool bResult = bPendingSteamMenuAccept;
	bPendingSteamMenuAccept = false;
	return bResult;
}

bool UStolematesGameInstance::ConsumeSteamMenuBack()
{
	const bool bResult = bPendingSteamMenuBack;
	bPendingSteamMenuBack = false;
	return bResult;
}

bool UStolematesGameInstance::ConsumeSteamMenuPause()
{
	const bool bResult = bPendingSteamMenuPause;
	bPendingSteamMenuPause = false;
	return bResult;
}