/// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"


#include "MultiplayerSessionsSubsystem.generated.h"

/*
 * // Manage Online Sessions using session interface functions (Create, Find, Join, etc)
 * // This class is used to manage the Online Sessions.
 * // It is used to create, join, find, start, and destroy sessions.
 * 
 */

///
/// Declaring our own custom delegates for the Menu class to bind callbacks to
///
/// DYNAMIC MULTICAST DELEGATES - Allows the class to be able to bind to functions that are called from other classes
/// DYNAMIC allows the delegate to be serialized and can be saved or loaded from within a blueprint graph
/// In blueprints they are called event dispachers
/// Delegate for when a session is created
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnCreateSessionComplete, bool, bWasSuccessful);

/// Can't use DYNAMIC because FOnlineSessionSearchResult is not compatible with blueprints
/// All parameters that are being passed through must be blueprint compatible to utilize the DYNAMIC Delegate
/// Subtle syntax difference
/// Delegate for when finding sessions
DECLARE_MULTICAST_DELEGATE_TwoParams(FMultiplayerOnFindSessionsComplete, const TArray<FOnlineSessionSearchResult>& SearchResults, bool bWasSuccessful);
/// Delegate for when a session is joined
DECLARE_MULTICAST_DELEGATE_OneParam(FMultiplayerOnJoinSessionComplete, EOnJoinSessionCompleteResult::Type Result);
/// Delegate for when a session is destroyed
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnDestroySessionComplete, bool, bWasSuccessful);
/// Delegate for when a session is started
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerOnStartSessionComplete, bool, bWasSuccessful);


UCLASS()
class MULTIPLAYERSESSIONS_API UMultiplayerSessionsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	UMultiplayerSessionsSubsystem();

	///
	/// To handle session functionality. The Menu class will call these functions.
	///
	
	/// NumPublicConnections: The number of players that can join the session.
	/// MatchType: The type of match that will be created. This is used to determine the type of session.
	void CreateSession(int32 NumPublicConnections, FString MatchType); /// Create a session.
	
	/// FindSessions will find sessions that match the search parameters.
	/// MaxSearchResults: The maximum number of search results to return.
	void FindSessions(int32 MaxSearchResults); /// Find sessions.
	
	/// JoinSession, will join the session with the given session name.
	/// SessionResult: The session that the player will join.
	void JoinSession(const FOnlineSessionSearchResult& SessionResult); /// Join a session.
	
	/// DestroySession, will destroy the session that the player is currently in.
	void DestroySession(); /// Destroy the session.
	
	/// StartSession, will start the session that the host created.
	void StartSession(); /// Start the session.

	
	///
	/// Our own custom delegates for the Menu class to bind callbacks to
	///
	FMultiplayerOnCreateSessionComplete MultiplayerOnCreateSessionComplete;
	FMultiplayerOnFindSessionsComplete MultiplayerOnFindSessionsComplete;
	FMultiplayerOnJoinSessionComplete MultiplayerOnJoinSessionComplete;
	FMultiplayerOnDestroySessionComplete MultiplayerOnDestroySessionComplete;
	FMultiplayerOnStartSessionComplete MultiplayerOnStartSessionComplete;
	
protected:
	
	
	///
	/// Internal callbacks for the delegates we'll add to the Online Session Interface delegate list.
	/// These don't need to be called outside this class.
	///
	
	/// Callback function in response to creating a successful game session; bound to CreateSessionCompleteDelegate
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful); /// Called when the session is created.
	
	/// Callback function in response to fining a successful game session; bound to FindSessionCompleteDelegate
	void OnFindSessionsComplete(bool bWasSuccessful); /// Called when the session is found.
	
	/// Callback function in response to joining a successful game session; bound to JoinSessionCompleteDelegate
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result); /// Called when the session is joined.
	
	/// Callback function in response to destroying a successful game session; bound to DestroySessionCompleteDelegate
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful); /// Called when the session is destroyed.
	
	/// Callback function in response to starting a successful game session; bound to StartSessionCompleteDelegate
	void OnStartSessionComplete(FName SessionName, bool bWasSuccessful); /// Called when the session is started.
	
private:
	/// Smart pointer that wraps the IOnlineSessionInterface
	IOnlineSessionPtr SessionInterface;
	/// Shared Ptr that wraps the FOnlineSessionSettings, storing the last used session settings
	TSharedPtr<FOnlineSessionSettings> LastSessionSettings;
	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;
	
	
	///
	/// To add to the Online Session Interface delegate list.
	/// We'll bind out MultiplayerSessionsSubsystem internal callbacks to these.
	/// 
	
	/// Delegate fired when a create session request has completed.
	FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
	FDelegateHandle CreateSessionCompleteDelegateHandle;
	
	/// Delegate fired when a find session request has completed
	FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
	FDelegateHandle FindSessionsCompleteDelegateHandle;
	
	/// Delegate fired when a join session request has completed
	FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
	FDelegateHandle JoinSessionCompleteDelegateHandle;
	
	/// Delegate fired when a destroy session request has completed
	FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
	FDelegateHandle DestroySessionCompleteDelegateHandle;
	
	/// Delegate fired when a start session request has completed
	FOnStartSessionCompleteDelegate StartSessionCompleteDelegate;
	FDelegateHandle StartSessionCompleteDelegateHandle;
	
};
