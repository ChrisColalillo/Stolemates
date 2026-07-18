// Fill out your copyright notice in the Description page of Project Settings.


#include "StolematesGameInstance.h"

#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Interfaces/OnlineFriendsInterface.h"
#include "StolenmatesPlayer.h"
#include "EngineUtils.h"
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
		UE_LOG(LogTemp, Error, TEXT("No Online Subsystem Found"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Online Subsystem Found: %s"),
		*Subsystem->GetSubsystemName().ToString());

	// Get Unreal's session interface from the subsystem
	// This is the object responsible for creating, finding,
	// joining, and destroying multiplayer sessions.
	IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();

	// Safety check
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Session Interface is not valid"));
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

	UE_LOG(LogTemp, Warning, TEXT("Attempting to create session"));

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
	UE_LOG(LogTemp, Warning,
		TEXT("Create Session Complete. Success: %s"),
		bWasSuccessful ? TEXT("true") : TEXT("false"));

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
		UE_LOG(LogTemp, Error, TEXT("No Online Subsystem Found"));
		return;
	}

	IOnlineFriendsPtr FriendsInterface = Subsystem->GetFriendsInterface();

	if (!FriendsInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Friends Interface is not valid"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Reading Steam friends list before finding sessions"));

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
	UE_LOG(LogTemp, Warning,
		TEXT("Read Friends List Complete. Success: %s Error: %s"),
		bWasSuccessful ? TEXT("true") : TEXT("false"),
		*ErrorStr
	);

	if (!bWasSuccessful)
	{
		OnJoinableSessionsUpdated();
		return;
	}

	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();

	if (!Subsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("No Online Subsystem Found"));
		OnJoinableSessionsUpdated();
		return;
	}

	IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();

	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Session Interface is not valid"));
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

	UE_LOG(LogTemp, Warning, TEXT("Searching for friend-hosted sessions"));

	SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
}

void UStolematesGameInstance::OnFindSessionsComplete(bool bWasSuccessful)
{
	FriendSessionIndices.Empty();

	UE_LOG(LogTemp, Warning,
		TEXT("Find Sessions Complete. Success: %s"),
		bWasSuccessful ? TEXT("true") : TEXT("false"));

	if (!bWasSuccessful || !SessionSearch.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Session search failed"));
		OnJoinableSessionsUpdated();
		return;
	}

	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();

	if (!Subsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("No Online Subsystem Found"));
		OnJoinableSessionsUpdated();
		return;
	}

	IOnlineFriendsPtr FriendsInterface = Subsystem->GetFriendsInterface();

	if (!FriendsInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Friends Interface is not valid"));
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
		UE_LOG(LogTemp, Warning, TEXT("Could not get Steam friends list"));
		OnJoinableSessionsUpdated();
		return;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("Sessions found: %d | Friends loaded: %d"),
		SessionSearch->SearchResults.Num(),
		Friends.Num()
	);

	for (int32 i = 0; i < SessionSearch->SearchResults.Num(); i++)
	{
		const FOnlineSessionSearchResult& Result = SessionSearch->SearchResults[i];

		if (IsSessionOwnerFriend(Result, Friends))
		{
			FriendSessionIndices.Add(i);

			UE_LOG(LogTemp, Warning,
				TEXT("Friend lobby found: %s"),
				*Result.Session.OwningUserName
			);
		}
	}

	UE_LOG(LogTemp, Warning,
		TEXT("Friend lobbies found: %d"),
		FriendSessionIndices.Num()
	);

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
		UE_LOG(LogTemp, Warning, TEXT("Invalid friend session display index: %d"), SessionIndex);
		return;
	}

	if (IsFoundSessionFull(SessionIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("Friend session index %d is full"), SessionIndex);
		return;
	}

	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();

	if (!Subsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("No Online Subsystem Found"));
		return;
	}

	IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();

	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Session Interface is not valid"));
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

	UE_LOG(LogTemp, Warning,
		TEXT("Attempting to join friend session display index %d / search result index %d"),
		SessionIndex,
		SearchResultIndex
	);

	SessionInterface->JoinSession(
		0,
		NAME_GameSession,
		SessionSearch->SearchResults[SearchResultIndex]
	);
}

void UStolematesGameInstance::JoinOnlineSession()
{
	UE_LOG(LogTemp, Warning, TEXT("Legacy JoinOnlineSession called. Use JoinOnlineSessionByIndex for the join popup."));

	JoinOnlineSessionByIndex(0);
}



void UStolematesGameInstance::OnJoinSessionComplete(
	FName SessionName,
	EOnJoinSessionCompleteResult::Type Result)
{
	UE_LOG(LogTemp, Warning, TEXT("Join Session Complete"));

	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();

	if (!Subsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("No Online Subsystem Found"));
		return;
	}

	IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();

	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Session Interface is not valid"));
		return;
	}

	FString TravelURL;

	if (SessionInterface->GetResolvedConnectString(SessionName, TravelURL))
	{
		UE_LOG(LogTemp, Warning, TEXT("Resolved Travel URL: %s"), *TravelURL);

		APlayerController* PlayerController = GetFirstLocalPlayerController();

		if (PlayerController)
		{
			PlayerController->ClientTravel(TravelURL, ETravelType::TRAVEL_Absolute);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Could not resolve connect string"));
	}
}




void UStolematesGameInstance::LeaveOnlineGameInternal(bool bReturnToPlayMenu)
{
	bReturnToPlayMenuAfterDestroy = bReturnToPlayMenu;

	// Get the active online subsystem.
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();

	if (!Subsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("No Online Subsystem Found"));

		// Even if there is no online subsystem, return to main menu.
		ReturnToHUDLevel(bReturnToPlayMenu);

		return;
	}

	// Get the session interface.
	IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();

	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Session Interface is not valid"));

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

	UE_LOG(LogTemp, Warning, TEXT("Attempting to destroy session"));

	// Destroy the current game session.
	SessionInterface->DestroySession(NAME_GameSession);
}



void UStolematesGameInstance::OnDestroySessionComplete(
	FName SessionName,
	bool bWasSuccessful)
{
	UE_LOG(LogTemp, Warning,
		TEXT("Destroy Session Complete. Success: %s"),
		bWasSuccessful ? TEXT("true") : TEXT("false"));

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
	UE_LOG(LogTemp, Warning,
		TEXT("Network Failure. Type: %d Error: %s"),
		static_cast<int32>(FailureType),
		*ErrorString
	);

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

	// Local games can return directly to the main menu.
	MatchType = EMatchType::Local;

	UGameplayStatics::OpenLevel(
		GetWorld(),
		FName("/Game/Levels/HUDLEVEL")
	);
}



void UStolematesGameInstance::LeaveOnlineGame()
{
	UE_LOG(LogTemp, Warning, TEXT("LEAVE GAME: Returning to Main menu after session destroy."));
	LeaveOnlineGameInternal(false);
}

void UStolematesGameInstance::LeaveOnlineLobby()
{
	UE_LOG(LogTemp, Warning, TEXT("LEAVE LOBBY: Returning to Play menu after session destroy."));
	LeaveOnlineGameInternal(true);
}



void UStolematesGameInstance::ReturnToHUDLevel(bool bReturnToPlayMenu)
{
	bOpenPlayMenuOnHUDLoad = bReturnToPlayMenu;

	UE_LOG(LogTemp, Warning, TEXT("RETURN TO HUDLEVEL. Open Play Menu: %s"),
		bOpenPlayMenuOnHUDLoad ? TEXT("true") : TEXT("false"));

	UGameplayStatics::OpenLevel(
		GetWorld(),
		FName("/Game/Levels/HUDLEVEL")
	);
}



bool UStolematesGameInstance::ConsumeOpenPlayMenuOnHUDLoad()
{
	const bool bShouldOpenPlayMenu = bOpenPlayMenuOnHUDLoad;

	UE_LOG(LogTemp, Warning, TEXT("CONSUME OPEN PLAY MENU FLAG: %s"),
		bShouldOpenPlayMenu ? TEXT("true") : TEXT("false"));

	bOpenPlayMenuOnHUDLoad = false;
	return bShouldOpenPlayMenu;
}


void UStolematesGameInstance::DebugSteamInputControllers()
{
#if PLATFORM_WINDOWS
	if (!SteamAPI_IsSteamRunning())
	{
		UE_LOG(LogTemp, Error, TEXT("STEAM CONTROLLER DEBUG: Steam is not running."));
		return;
	}

	if (!SteamController())
	{
		UE_LOG(LogTemp, Error, TEXT("STEAM CONTROLLER DEBUG: SteamController() returned null."));
		return;
	}

	SteamController()->Init();
	SteamController()->RunFrame();

	ControllerHandle_t ControllerHandles[STEAM_CONTROLLER_MAX_COUNT];
	const int32 NumControllers = SteamController()->GetConnectedControllers(ControllerHandles);

	UE_LOG(LogTemp, Warning, TEXT("STEAM CONTROLLER DEBUG: Connected controllers: %d"), NumControllers);

	for (int32 i = 0; i < NumControllers; i++)
	{
		const ControllerHandle_t Handle = ControllerHandles[i];
		const ESteamInputType InputType = SteamController()->GetInputTypeForHandle(Handle);

		UE_LOG(LogTemp, Warning,
			TEXT("STEAM CONTROLLER DEBUG: Controller %d | Handle: %llu | Type: %d"),
			i,
			(uint64)Handle,
			(int32)InputType
		);
	}
#else
	UE_LOG(LogTemp, Warning, TEXT("STEAM CONTROLLER DEBUG: Not running on Windows."));
#endif
}


void UStolematesGameInstance::PollSteamControllerActions()
{
#if PLATFORM_WINDOWS
	const bool bShouldPollSteamGameplay =
		(MatchType == EMatchType::Online) ||
		(MatchType == EMatchType::Local && LocalPlayerCount > 1);

	if (!bShouldPollSteamGameplay)
	{
		return;
	}
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

		UE_LOG(LogTemp, Warning,
			TEXT("STEAM ACTION: SteamController Init = %s"),
			bInitializedSteamController ? TEXT("true") : TEXT("false"));
	}

	SteamController()->RunFrame();

	const ControllerActionSetHandle_t GameplayActionSet =
		SteamController()->GetActionSetHandle("Gameplay");

	const ControllerDigitalActionHandle_t JumpAction =
		SteamController()->GetDigitalActionHandle("Jump");

	const ControllerDigitalActionHandle_t UseAbilityAction =
		SteamController()->GetDigitalActionHandle("UseAbility");

	const ControllerAnalogActionHandle_t MoveAction =
		SteamController()->GetAnalogActionHandle("Move");

	if (GameplayActionSet == 0 || JumpAction == 0 || UseAbilityAction == 0 || MoveAction == 0)
	{
		return;
	}

	ControllerHandle_t ControllerHandles[STEAM_CONTROLLER_MAX_COUNT];
	const int32 NumControllers = SteamController()->GetConnectedControllers(ControllerHandles);

	static TMap<uint64, bool> LastJumpStateByHandle;
	static TMap<uint64, bool> LastUseAbilityStateByHandle;

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

		const bool bJumpDown = JumpData.bState;

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

		const bool bUseAbilityDown = UseAbilityData.bState;

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

		UE_LOG(LogTemp, Warning,
			TEXT("STEAM MENU DEBUG: SteamController Init = %s"),
			bInitializedSteamController ? TEXT("true") : TEXT("false"));
	}

	SteamController()->RunFrame();

	const ControllerActionSetHandle_t MenuActionSet =
		SteamController()->GetActionSetHandle("Menu");

	const ControllerDigitalActionHandle_t MenuAcceptAction =
		SteamController()->GetDigitalActionHandle("MenuAccept");

	const ControllerDigitalActionHandle_t MenuBackAction =
		SteamController()->GetDigitalActionHandle("MenuBack");

	const ControllerAnalogActionHandle_t MenuMoveAction =
		SteamController()->GetAnalogActionHandle("MenuMove");

	if (MenuActionSet == 0 || MenuAcceptAction == 0 || MenuBackAction == 0 || MenuMoveAction == 0)
	{
		UE_LOG(LogTemp, Error,
			TEXT("STEAM MENU DEBUG: Missing handle. MenuSet=%llu Accept=%llu Back=%llu Move=%llu"),
			(uint64)MenuActionSet,
			(uint64)MenuAcceptAction,
			(uint64)MenuBackAction,
			(uint64)MenuMoveAction);

		return;
	}

	ControllerHandle_t ControllerHandles[STEAM_CONTROLLER_MAX_COUNT];
	const int32 NumControllers = SteamController()->GetConnectedControllers(ControllerHandles);

	if (NumControllers <= 0)
	{
		return;
	}

	static TMap<uint64, bool> LastAcceptStateByHandle;
	static TMap<uint64, bool> LastBackStateByHandle;
	static TMap<uint64, FString> LastMoveDirectionByHandle;

	const float MoveThreshold = 0.55f;

	for (int32 i = 0; i < NumControllers; i++)
	{
		const ControllerHandle_t Handle = ControllerHandles[i];
		const uint64 HandleKey = static_cast<uint64>(Handle);

		SteamController()->ActivateActionSet(Handle, MenuActionSet);

		const ControllerDigitalActionData_t AcceptData =
			SteamController()->GetDigitalActionData(Handle, MenuAcceptAction);

		const bool bAcceptDown = AcceptData.bState;
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

		const bool bBackDown = BackData.bState;
		const bool bLastBackDown =
			LastBackStateByHandle.Contains(HandleKey)
			? LastBackStateByHandle[HandleKey]
			: false;

		if (bBackDown && !bLastBackDown)
		{
			bPendingSteamMenuBack = true;
		}

		LastBackStateByHandle.Add(HandleKey, bBackDown);
		
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