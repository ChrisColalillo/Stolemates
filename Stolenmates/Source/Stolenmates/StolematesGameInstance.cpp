// Fill out your copyright notice in the Description page of Project Settings.


#include "StolematesGameInstance.h"

#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

void UStolematesGameInstance::HostOnlineGame()
{
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
	SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(
		this,
		&UStolematesGameInstance::OnCreateSessionComplete
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
	// Get active online subsystem:
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();

	if (!Subsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("No Online Subsystem Found"));
		return;
	}

	// Get the session interface, which handles finding and joining sessions.
	IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();

	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Session Interface is not valid"));
		return;
	}

	// Create a search object that stores search settings and results.
	SessionSearch = MakeShareable(new FOnlineSessionSearch());

	// false = search online/Steam sessions, not LAN sessions.
	SessionSearch->bIsLanQuery = false;

	// Maximum number of sessions to return.
	SessionSearch->MaxSearchResults = 20;

	// Steam presence sessions usually need this query filter.
	SessionSearch->QuerySettings.Set(
		SEARCH_PRESENCE,
		true,
		EOnlineComparisonOp::Equals
	);

	// Register callback for when the search finishes.
	SessionInterface->OnFindSessionsCompleteDelegates.AddUObject(
		this,
		&UStolematesGameInstance::OnFindSessionsComplete
	);

	UE_LOG(LogTemp, Warning, TEXT("Searching for joinable sessions"));

	// Start searching for sessions.
	SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
}

void UStolematesGameInstance::OnFindSessionsComplete(bool bWasSuccessful)
{
	UE_LOG(LogTemp, Warning,
		TEXT("Find Sessions Complete. Success: %s"),
		bWasSuccessful ? TEXT("true") : TEXT("false"));

	if (!bWasSuccessful || !SessionSearch.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Session search failed"));
		return;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("Sessions found: %d"),
		SessionSearch->SearchResults.Num());

	if (SessionSearch->SearchResults.Num() > 0)
	{
		JoinOnlineSession();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No joinable sessions found"));
	}
}

void UStolematesGameInstance::JoinOnlineSession()
{
	// Make sure we have valid search results before trying to join.
	if (!SessionSearch.IsValid() || SessionSearch->SearchResults.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No session search results available to join"));
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

	// Register callback for when join attempt finishes.
	SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(
		this,
		&UStolematesGameInstance::OnJoinSessionComplete
	);

	UE_LOG(LogTemp, Warning, TEXT("Attempting to join first found session"));

	// Join the first session found.
	SessionInterface->JoinSession(
		0,
		NAME_GameSession,
		SessionSearch->SearchResults[0]
	);
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




void UStolematesGameInstance::LeaveOnlineGame()
{
	// Get the active online subsystem.
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();

	if (!Subsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("No Online Subsystem Found"));

		// Even if there is no online subsystem, return to main menu.
		UGameplayStatics::OpenLevel(
			GetWorld(),
			FName("/Game/Levels/HUDLEVEL")
		);

		return;
	}

	// Get the session interface.
	IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();

	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Session Interface is not valid"));

		// Even if session cleanup fails, return to main menu.
		UGameplayStatics::OpenLevel(
			GetWorld(),
			FName("/Game/Levels/HUDLEVEL")
		);

		return;
	}

	// Register callback for when session destruction finishes.
	SessionInterface->OnDestroySessionCompleteDelegates.AddUObject(
		this,
		&UStolematesGameInstance::OnDestroySessionComplete
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
	UGameplayStatics::OpenLevel(
		GetWorld(),
		FName("/Game/Levels/HUDLEVEL")
	);
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