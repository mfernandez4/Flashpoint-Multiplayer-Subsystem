/// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"

UMultiplayerSessionsSubsystem::UMultiplayerSessionsSubsystem():

		
	/// initialize OnCreateSessionCompleteDelegate variable with an object of this type, and simultaneously pass in the callback function binding it to the delegate
	/// create a delegate object that will be used to bind the callback function to the delegate, using the CreateUObject function to create a new instance of the delegate object
	/// the delegate object is then assigned to the OnCreateSessionCompleteDelegate variable using the address of OnCreateSessionComplete
	/// this is the same as saying OnCreateSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &AMenuSystemCharacter::OnCreateSessionComplete);
	CreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete)),

	/// initialize OnFindSessionsCompleteDelegate variable with an object of this type, and simultaneously pass in the callback function binding it to the delegate
	/// create a delegate object that will be used to bind the callback function to the delegate, using the CreateUObject function to create a new instance of the delegate object
	FindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionsComplete)),

	/// initialize OnJoinSessionCompleteDelegate variable with an object of this type, and simultaneously pass in the callback function binding it to the delegate
	/// create a delegate object that will be used to bind the callback function to the delegate, using the CreateUObject function to create a new instance of the delegate object
	JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete)),
	
	/// initialize OnDestroySessionCompleteDelegate variable with an object of this type, and simultaneously pass in the callback function binding it to the delegate
	/// create a delegate object that will be used to bind the callback function to the delegate, using the CreateUObject function to create a new instance of the delegate object
	DestroySessionCompleteDelegate(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionComplete)),
	
	/// initialize OnFindFriendSessionCompleteDelegate variable with an object of this type, and simultaneously pass in the callback function binding it to the delegate
	/// create a delegate object that will be used to bind the callback function to the delegate, using the CreateUObject function to create a new instance of the delegate object
	StartSessionCompleteDelegate(FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnStartSessionComplete))
{
	/// Access the OnlineSubsystem using the getter function, then check if the OnlineSubsystem is Valid
	/// If OnlineSubsystem is Valid, access the OnlineSubsystem Interface
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	
	/// If the OnlineSubsystem is valid, then we can use the interface
	if (Subsystem)
	{
		/// Get the Interface to the OnlineSessionInterface
		/// The OnlineSessionInterface is used to create, join, and destroy sessions
		/// We can access get SessionInterface and store it in our SessionInterface pointer
		SessionInterface = Subsystem->GetSessionInterface();
	}
}


void UMultiplayerSessionsSubsystem::CreateSession(int32 NumPublicConnections, FString MatchType)
{
	/// Check if the OnlineSubsystem is Valid
	if (!SessionInterface.IsValid())
	{
		/// If the OnlineSubsystem is not valid, then we cannot create a session
		return;
	}
	
	/// Check if there is already a session in progress with the same name
	auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
	/// If the ExistingSession is not null, then we have already created a session
	/// Destroy the session before creating a new one
	if (ExistingSession != nullptr)
	{
		bCreateSessionOnDestroy = true;
		LastNumPublicConnections = NumPublicConnections;
		LastMatchType = MatchType;

		DestroySession();
	}

	/// Once we create a session, we need to add the CreateSessionCompleteDelegate to the AddOnCreateSessionCompleteDelegate_Handle list
	/// When the session is created, the OnCreateSessionComplete function will be called, which is bound the OnCreateSessionCompleteDelegate
	/// Store the delegate in a FDelegateHandle so we can remove it later from the delegate list
	CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);
	
	/// Using the MakeShareable function to create a TSharedPtr from FOnlineSessionSettings
	/// Container for all settings describing a single online session
	LastSessionSettings = MakeShareable(new FOnlineSessionSettings());
	/// Check if the subsystem is null, then we are on a local machine, so we are using a LAN match
	LastSessionSettings->bIsLANMatch = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL" ? true : false; 
	LastSessionSettings->NumPublicConnections = NumPublicConnections; /// Set the number of public connections to the value passed in
	LastSessionSettings->bAllowJoinInProgress = true; /// Allow players to join sessions that are in progress
	LastSessionSettings->bAllowJoinViaPresence = true; /// Allow players to join sessions using presence
	LastSessionSettings->bShouldAdvertise = true; /// Advertise the session to other players
	LastSessionSettings->bUsesPresence = true; /// Use presence to join sessions
	LastSessionSettings->bUseLobbiesIfAvailable = true; /// Whether to use lobbies if they are available or not
	LastSessionSettings->Set(FName("MatchType"), MatchType, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing); /// Set the match type to the value passed in
	LastSessionSettings->BuildUniqueId = 1; /// Set the build unique id to 1

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController(); /// Get the first local player from the controller
	
	/// Check if create session is successful, if it's not successful, then we will clear the delegate handle from the list
	if (!SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *LastSessionSettings))
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);

		/// Broadcast our own custom delegate
		/// Broadcast the OnCreateSessionComplete delegate, passing in false because the session was not created
		MultiplayerOnCreateSessionComplete.Broadcast(false);
	}
}


void UMultiplayerSessionsSubsystem::FindSessions(int32 MaxSearchResults)
{
	///** Find game sessions **///

	/// Check if the OnlineSessionInterface is valid, if not return out of the function
	if (!SessionInterface.IsValid())
	{
		return;
	}
	
	/// Add the FindSessionsCompleteDelegate to the OnlineSessionInterface using the AddOnFindSessionsCompleteDelegate_Handle list
	/// When the session is found, the OnFindSessionsComplete function will be called, which is bound the OnFindSessionsCompleteDelegate
	FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);

	/// Setup session search settings which are required to find a session and call the session interface function FindSessions
	/// We will use the FOnlineSessionSearch as a TSharedPtr to store the online session search settings
	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());

	/// Set the search settings
	LastSessionSearch->MaxSearchResults = MaxSearchResults;
	/// Check if the subsystem is null, then we are on a local machine, so we are using a LAN match
	LastSessionSearch->bIsLanQuery = IOnlineSubsystem::Get()->GetSubsystemName() == "NULL" ? true : false; 
	/// Set QuerySettings to make sure we only search for sessions using presence
	LastSessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

	/// Get the LocalPlayer by using GetWorld() and then GetFirstLocalPlayerController()
	/// This will return the first local player controller, which we can then access the GetPrefferedUniqueNetId function on to pass to the FindSessions function
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();

	/// Call the FindSessions function on the OnlineSessionInterface, passing in the FUniqueNetId and the SessionSearch TSharedPtr
	/// This will return a list of sessions that match the search settings we set earlier
	if (!SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef()))
	{
		//if (GEngine)
		//{
		//	GEngine->AddOnScreenDebugMessage(
		//		-2,
		//		15.f,
		//		FColor::Red,
		//		FString::Printf(TEXT("Failed to FindSessions!"))
		//	);
		//}
		/// If the FindSessions function fails, then we will clear the delegate handle from the list
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);

		/// Broadcast our own custom delegate
		/// Passing in an empty TArray of type FOnlineSessionSearchResult and false because the session was not found
		MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
	}
	
}


void UMultiplayerSessionsSubsystem::JoinSession(const FOnlineSessionSearchResult& SessionResult)
{
	/// Check if the OnlineSessionInterface is valid, if not return out of the function
	if (!SessionInterface.IsValid())
	{
		/// Broadcast our own custom delegate
		/// Passing in EOnJoinSessionCompleteResult with type UnknownError
		MultiplayerOnJoinSessionComplete.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		return;
	}

	/// Add the JoinSessionCompleteDelegate to the OnlineSessionInterface using the AddOnJoinSessionCompleteDelegate_Handle list
	/// When the session is joined, the OnJoinSessionComplete function will be called, which is bound the OnJoinSessionCompleteDelegate
	JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);

	/// Get the LocalPlayer by using GetWorld() and then GetFirstLocalPlayerController()
	/// This will return the first local player controller, which we can then access the GetPrefferedUniqueNetId function on to pass to the JoinSession function
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();

	/// Call the JoinSession function on the OnlineSessionInterface, passing in the FUniqueNetId and the SessionSearch TSharedPtr
	/// This will return a list of sessions that match the search settings we set earlier
	if (!SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SessionResult))
	{
		/// If the JoinSession function fails, then we will clear the delegate handle from the list
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Red,
				FString::Printf(TEXT("Failed to Join Session!"))
			);
		}

		/// Broadcast our own custom delegate
		/// Passing in EOnJoinSessionCompleteResult with type UnknownError
		MultiplayerOnJoinSessionComplete.Broadcast((EOnJoinSessionCompleteResult::UnknownError));
	}
}


void UMultiplayerSessionsSubsystem::DestroySession()
{
	if (!SessionInterface.IsValid())
	{
		MultiplayerOnDestroySessionComplete.Broadcast(false);
		return;
	}

	DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);

	if (!SessionInterface->DestroySession(NAME_GameSession))
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
		MultiplayerOnDestroySessionComplete.Broadcast(false);
	}
}


void UMultiplayerSessionsSubsystem::StartSession()
{

}


void UMultiplayerSessionsSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	/// Fired off when creating a session is complete
	if (SessionInterface)
	{
		/// Clear the delegate handle from the list of delegates
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
	}
	
	/// Broadcast our own custom delegate
	/// Broadcast the OnCreateSessionComplete delegate, passing in bWasSuccessful as the parameter
	MultiplayerOnCreateSessionComplete.Broadcast(bWasSuccessful);
}


void UMultiplayerSessionsSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{

	/// Fired off when finding a session is successful
	if (SessionInterface)
	{
		/// Clear the delegate handle from the list of delegates
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
	}
	

	/// Check if the search was successful, if it was successful but the results are empty
	/// Then we will broadcast our own custom delegate with an empty TArray of type FOnlineSessionSearchResult and false as the parameter
	/// We will return out of the function because we didn't find any valid search results
	if (LastSessionSearch->SearchResults.Num() <=0)
	{
		/// Broadcast our own custom delegate
		/// Passing in an empty TArray of type FOnlineSessionSearchResult and false because a session was not found
		//if (GEngine)
		//{
		//	GEngine->AddOnScreenDebugMessage(
		//		-1,
		//		15.f,
		//		FColor::Red,
		//		FString::Printf(TEXT("No Session Found!"))
		//	);
		//}
		MultiplayerOnFindSessionsComplete.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		return;
	}

	
	/// Broadcast our own custom delegate
	/// Broadcast the OnCreateSessionComplete delegate, passing in bWasSuccessful as the parameter
	MultiplayerOnFindSessionsComplete.Broadcast(LastSessionSearch->SearchResults, bWasSuccessful);
}


void UMultiplayerSessionsSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	/// Fired off when joining a session is complete
	if (SessionInterface)
	{
		/// Clear the delegate handle from the list of delegates
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
	}
	
	/// Broadcast our own custom delegate
	/// Broadcast the OnJoinSessionComplete delegate, passing in Result as the parameter
	MultiplayerOnJoinSessionComplete.Broadcast(Result);
}


void UMultiplayerSessionsSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface)
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
	}
	if (bWasSuccessful && bCreateSessionOnDestroy)
	{
		bCreateSessionOnDestroy = false;
		CreateSession(LastNumPublicConnections, LastMatchType);
	}
	MultiplayerOnDestroySessionComplete.Broadcast(bWasSuccessful);
}


void UMultiplayerSessionsSubsystem::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{

}
