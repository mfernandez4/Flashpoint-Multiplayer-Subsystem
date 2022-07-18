/// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Menu.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMenu : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
	void MenuSetup(int32 NumberOfPublicConnections = 4, FString TypeOfMatch = FString(TEXT("FreeForAll")), FString LobbyPath = FString(TEXT("/Game/Maps/Lobby")));
	
protected:
	
	virtual bool Initialize() override;
	virtual void OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld) override;


	///
	/// Callbacks for the custom delegates on the MultiplayerSessionsSubsystem
	///
	
	UFUNCTION()
	void OnCreateSession(bool bWasSuccessful); ///, const FString& Error);
	void OnFindSessions(const TArray<FOnlineSessionSearchResult>& SearchResults, bool bWasSuccessful);
	void OnJoinSession(EOnJoinSessionCompleteResult::Type Result);
	UFUNCTION()
	void OnDestroySession(bool bWasSuccessful);
	UFUNCTION()
	void OnStartSession(bool bWasSuccessful);
	
private:

	/// Bind the variable to the button widget with the same name in the blueprint.  
	/// This is how you can bind a variable to a widget. 
	/// Can cause errors if the variable name doesn't match the widget name.
	UPROPERTY(meta = (BindWidget))
	class UButton* HostButton;
	
	/// Bind the variable to the button widget with the same name in the blueprint.  
	/// This is how you can bind a variable to a widget. 
	/// Can cause errors if the variable name doesn't match the widget name.
	UPROPERTY(meta = (BindWidget)) 
	UButton* JoinButton;

	UFUNCTION()
	void HostButtonClicked();
	
	UFUNCTION()
	void JoinButtonClicked();

	void MenuTearDown();

	
	/// forward declare the multiplayerSessionsSubsystem
	/// This is how you can access the subsystem from another class.
	/// The subsystem is designed to handle all online session functionality
	class UMultiplayerSessionsSubsystem *MultiplayerSessionsSubsystem;

	int32 NumPublicConnections{4};
	FString MatchType{TEXT("FreeForAll")};
	FString PathToLobby{ TEXT("") };
};
