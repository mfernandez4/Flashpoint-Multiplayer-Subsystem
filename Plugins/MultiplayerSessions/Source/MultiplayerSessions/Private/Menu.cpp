/// Fill out your copyright notice in the Description page of Project Settings.


#include "Menu.h"
#include "Components/Button.h"
#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSubsystem.h"

void UMenu::MenuSetup(int32 NumberOfPublicConnections, FString TypeOfMatch)
{
	/// Set member variables for the menu; NumPublicConnections and MatchType
	NumPublicConnections = NumberOfPublicConnections;
	MatchType = TypeOfMatch;
	
	AddToViewport(); /// Add the menu to the viewport
	SetVisibility(ESlateVisibility::Visible); /// Make the menu visible
	bIsFocusable = true; /// Make the menu focusable
	
	UWorld* World = GetWorld(); /// Get the world
	
	/// check if the world is valid
	if (World)
	{
		/// get the player controller
		APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
		/// check if the player controller is valid
		if (PlayerController)
		{
			/// create a variable to store the input mode
			/// set the input mode to UI only
			FInputModeUIOnly InputMode;
			/// set the input mode to focus on the menu
			InputMode.SetWidgetToFocus(this->TakeWidget());
			/// set the mouse viewport behavior to not lock the cursor to the viewport
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			/// set the input mode to the player controller
			PlayerController->SetInputMode(InputMode);
			/// set the cursor to be visible
			PlayerController->bShowMouseCursor = true;
		}
	}
	
	///
	/// Access MultiplayerSessionSubsystem from the game instance
	/// It gets created when after the game has been launched and is destroyed when the game is closed
	/// First we get the game instance and check if it is valid
	/// Then we get the multiplayer session subsystem from the game instance
	///
	UGameInstance* GameInstance = GetGameInstance();
	
	/// check if the game instance is valid
	if (GameInstance)
	{
		/// get the multiplayer session subsystem from the game instance
		/// store the result in a variable, 'MultiplayerSessionSubsystem'
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
	}

	/// check if the multiplayer session subsystem is valid
	/// if it is valid, then we can bind the callbacks to the delegates in the subsystem
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionComplete.AddDynamic(this, &ThisClass::OnCreateSession);
		MultiplayerSessionsSubsystem->MultiplayerOnFindSessionsComplete.AddUObject(this, &ThisClass::OnFindSessions);
		MultiplayerSessionsSubsystem->MultiplayerOnJoinSessionComplete.AddUObject(this, &ThisClass::OnJoinSession);
		MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &ThisClass::OnDestroySession);
		MultiplayerSessionsSubsystem->MultiplayerOnStartSessionComplete.AddDynamic(this, &ThisClass::OnStartSession);
	}
}

bool UMenu::Initialize()
{
	/// call the base class implementation
	/// if it fails to initialize, then return false
	if (!Super::Initialize())
	{
		return false;
	}
	
	/// Check if the host button is valid
	/// If it is, add a dynamic function to the button 
	/// Which binds the variable to the blueprint button widget of the same name
	if (HostButton)
	{
		HostButton->OnClicked.AddDynamic(this, &ThisClass::HostButtonClicked);
	}
	
	/// Check if the join button is valid
	/// If it is, add a dynamic function to the button 
	/// Which binds the variable to the blueprint button widget of the same name
	if (JoinButton)
	{
		JoinButton->OnClicked.AddDynamic(this, &ThisClass::JoinButtonClicked);
	}
	
	return true;
}

void UMenu::OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld)
{
	MenuTearDown();
	Super::OnLevelRemovedFromWorld(InLevel, InWorld);
}

void UMenu::OnCreateSession(bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		/// Display a message to the user that the session was created
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Yellow,
				FString(TEXT("Session Created Successfully!"))
			);
		}

		/// Get the world, and check if it is valid
		/// Then SeverTravel is called on the world, and the level is set to the lobby level
		UWorld* World = GetWorld();
		if (World)
		{
			World->ServerTravel(FString("/Game/Maps/Lobby?listen"));
		}
		
	}
	else
	{
		/// Display a message to the user that the session was not created
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Red,
				FString(TEXT("Session Creation Failed!"))
			);
		}
	}
}

void UMenu::OnFindSessions(const TArray<FOnlineSessionSearchResult>& SearchResults, bool bWasSuccessful)
{
	/// Check if MultiplayerSessionSubsystem is valid
	/// If it is not valid, then we will simply return out of the function
	if (MultiplayerSessionsSubsystem == nullptr)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-2,
				15.f,
				FColor::Red,
				FString::Printf(TEXT("Multiplayer Session Subsystem is a nullptr!"))
			);
		}

		return;
	}
	
	/// loop through the list of sessions and print out the session names
	/// We will use the SessionSearch TSharedPtr to get the list of sessions
	for (auto Result : SearchResults)
	{
		/// Get the session name
		FString Id = Result.GetSessionIdStr();
		/// Get the session owning player
		FString User = Result.Session.OwningUserName;
		
		/// Create a variable to store the matchType value passed from the SearchResults
		FString SettingsValue;
		
		/// Get the match type, using the Get function on the SessionSearch
		Result.Session.SessionSettings.Get(FName("MatchType"), SettingsValue);
		
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Cyan,
				FString::Printf(TEXT("Found Session Id: %s, User: %s"), *Id, *User)
			);
		}
		
		/// Check if the match type is the same as the match type we are looking for
		if (SettingsValue == MatchType)
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					-1,
					15.f,
					FColor::Cyan,
					FString::Printf(TEXT("Found Match Type: %s"), *MatchType)
				);
			}
			/// Once we found a session that meets out search criteria
			/// Call JoinSession on the MultiplayerSessionsSubsytem, passing in the found session Result
			/// Don't need to continue looping through the searchResults, so return out of this function
			MultiplayerSessionsSubsystem->JoinSession(Result);
			return;
		}
	}
}

void UMenu::OnJoinSession(EOnJoinSessionCompleteResult::Type Result)
{
	/// This function will be called in response to the delegate broadcast sent by the session interface.
	/// Once the action of joining a session has been completed.


	/// Access the OnlineSubsystem using the getter function, then check if the OnlineSubsystem is Valid
	/// If OnlineSubsystem is Valid, access the OnlineSubsystem Interface
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();

	/// If the OnlineSubsystem is valid, then we can use the interface
	if (Subsystem)
	{
		/// Get the Interface to the OnlineSessionInterface
		/// The OnlineSessionInterface is used to create, join, and destroy sessions
		/// We can access get SessionInterface and store it in our SessionInterface pointer
		IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();

		/// Check if the OnlineSessionInterface is valid
		if (SessionInterface.IsValid())
		{
			/// Store the IP address in the FString variable "Address"
			FString Address;

			/// Get Address or ResolvedConnectString
			SessionInterface->GetResolvedConnectString(NAME_GameSession, Address);

			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					-1,
					15.f,
					FColor::Yellow,
					FString::Printf(TEXT("Connect String: %s"), *Address)
				);
			}

			/// Get the player controller by using GetGameInstance
			APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();

			/// Check if the player controller is valid
			/// Call the ServerTravel function on the PlayerController, passing in the Address and the TravelType
			if (PlayerController)
			{
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(
						-1,
						15.f,
						FColor::Cyan,
						FString::Printf(TEXT("PlayerController Found!"))
					);
				}
				
				PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
			}
			else
			{
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(
						-1,
						15.f,
						FColor::Red,
						FString::Printf(TEXT("PlayerController Not Found!"))
					);
				}
			}
		}
	}
}

void UMenu::OnDestroySession(bool bWasSuccessful)
{

}

void UMenu::OnStartSession(bool bWasSuccessful)
{

}

void UMenu::HostButtonClicked()
{
	/// Action to perform when the host button is clicked
	/*if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1, 
			15.f, 
			FColor::Yellow, 
			FString(TEXT("Host Button Clicked"))
		);
	}*/
	
	/// Check if the multiplayer session subsystem is valid
	if (MultiplayerSessionsSubsystem)
	{
		/// Create a new session, set the match type, and set the max number of players
		MultiplayerSessionsSubsystem->CreateSession(NumPublicConnections, MatchType);
		
	}
}

void UMenu::JoinButtonClicked()
{
	/*if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			15.f,
			FColor::Yellow,
			FString(TEXT("Join Button Clicked"))
		);
	}*/
	if (MultiplayerSessionsSubsystem)
	{
		/// Find a session, set the max number of players, and set the match type
		MultiplayerSessionsSubsystem->FindSessions(10000);
	}
}

void UMenu::MenuTearDown()
{
	RemoveFromParent();
	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			FInputModeGameOnly InputMode;
			PlayerController->SetInputMode(InputMode);
			PlayerController->SetShowMouseCursor(false);
		}
	}
}
