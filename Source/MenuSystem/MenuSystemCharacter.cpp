/// Copyright Epic Games, Inc. All Rights Reserved.

#include "MenuSystemCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"


//////////////////////////////////////////////////////////////////////////
// AMenuSystemCharacter

AMenuSystemCharacter::AMenuSystemCharacter():
	
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
	JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete))
	
{
	/// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	/// set our turn rate for input
	TurnRateGamepad = 50.f;

	/// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	/// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; /// Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); /// ...at this rotation rate

	/// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	/// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	/// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; /// The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; /// Rotate the arm based on the controller

	/// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); /// Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; /// Camera does not rotate relative to arm

	/// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	/// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	
	
	/// Access the OnlineSubsystem using the getter function, then check if the OnlineSubsystem is Valid
	/// If OnlineSubsystem is Valid, access the OnlineSubsystem Interface
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		OnlineSessionInterface = OnlineSubsystem->GetSessionInterface();

		if (GEngine)
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Blue,
				FString::Printf(TEXT("Found subsystem %s"), *OnlineSubsystem->GetSubsystemName().ToString())
			);
	}
}

//////////////////////////////////////////////////////////////////////////
/// Input

void AMenuSystemCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	/// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &AMenuSystemCharacter::MoveForward);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &AMenuSystemCharacter::MoveRight);

	/// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	/// "turn" handles devices that provide an absolute delta, such as a mouse.
	/// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn Right / Left Mouse", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("Turn Right / Left Gamepad", this, &AMenuSystemCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("Look Up / Down Mouse", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Look Up / Down Gamepad", this, &AMenuSystemCharacter::LookUpAtRate);

	/// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AMenuSystemCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AMenuSystemCharacter::TouchStopped);
}


void AMenuSystemCharacter::CreateGameSession()
{
	/// Called when pressing the 1 key
	/// Check if the OnlineSessionInterface is valid, if not return out of the function
	if (!OnlineSessionInterface.IsValid())
	{
		return;
	}
	
	/// Get the existing session, if one exists with this name
	/// If that existing session is not a null ptr then we will destroy the session with the same name
	/// That way we can create a new session
	
	auto ExistingSession = OnlineSessionInterface->GetNamedSession(NAME_GameSession);
	if (ExistingSession != nullptr)
	{
		OnlineSessionInterface->DestroySession(NAME_GameSession);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Blue,
				FString::Printf(TEXT("Destroy Existing Game Session"))
			);
		}
	}

	/// Once we create a session, we need to add the CreateSessionCompleteDelegate to the AddOnCreateSessionCompleteDelegate_Handle list
	/// When the session is created, the OnCreateSessionComplete function will be called, which is bound the OnCreateSessionCompleteDelegate
	OnlineSessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

	/// Create SessionSettings, using FOnlineSessionSettings as a TSharePtr
	/// Using the MakeShareable function to create a TSharedPtr from FOnlineSessionSettings
	TSharedPtr<FOnlineSessionSettings> SessionSettings = MakeShareable(new FOnlineSessionSettings());
	
	/// Set SessionSettings
	SessionSettings->bIsLANMatch = false; /// Is this a LAN game?
	SessionSettings->NumPublicConnections = 4; /// Max number of players allowed in the session
	SessionSettings->bAllowJoinInProgress = true; /// Allows people to join a session that is in progress
	SessionSettings->bAllowJoinViaPresence = true; /// Whether joining via player presence is allowed or not
	SessionSettings->bShouldAdvertise = true; /// Whether this match is publicly advertised on the on-line services
	SessionSettings->bUsesPresence = true; /// Whether this match uses presence to matchmake or not
	SessionSettings->bUseLobbiesIfAvailable = true; /// Whether to use lobbies if they are available or not
	SessionSettings->Set(FName("MatchType"), FString("FreeForAll"), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing); /// Set the match type to FreeForAll
	
	/// Get the LocalPlayer by using GetWorld() and then GetFirstLocalPlayerController()
	/// This will return the first local player controller, which we can then access the GetPrefferedUniqueNetId function on to pass to the CreateSession function
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	
	/// Using the OnlineSessionInterface, create a session with the name of "NAME_GameSession", the LocalPlayer's preferred unique net id, and the SessionSettings we created earlier
	OnlineSessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *SessionSettings);
}


void AMenuSystemCharacter::JoinGameSession()
{
	///** Find game sessions **///
	
	/// Check if the OnlineSessionInterface is valid, if not return out of the function
	if (!OnlineSessionInterface.IsValid())
	{
		return;
	}
	
	/// Add the FindSessionsCompleteDelegate to the OnlineSessionInterface using the AddOnFindSessionsCompleteDelegate_Handle list
	/// When the session is found, the OnFindSessionsComplete function will be called, which is bound the OnFindSessionsCompleteDelegate
	OnlineSessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);

	/// Setup session search settings which are required to find a session and call the session interface function FindSessions
	/// We will use the FOnlineSessionSearch as a TSharedPtr to store the online session search settings
	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	
	/// Set the search settings
	SessionSearch->MaxSearchResults = 10000;
	SessionSearch->bIsLanQuery = false;

	/// Set QuerySettings to make sure we only search for sessions using presence
	SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
	
	/// Get the LocalPlayer by using GetWorld() and then GetFirstLocalPlayerController()
	/// This will return the first local player controller, which we can then access the GetPrefferedUniqueNetId function on to pass to the FindSessions function
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	
	/// Call the FindSessions function on the OnlineSessionInterface, passing in the FUniqueNetId and the SessionSearch TSharedPtr
	/// This will return a list of sessions that match the search settings we set earlier
	OnlineSessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), SessionSearch.ToSharedRef());
}


void AMenuSystemCharacter::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	/// This is where we will verify that our session was created successfully
	/// Callback function will be receiving information about the session that was created
	/// If it was created successfully, we will then call the StartSession function
	
	/// Check if session was created successfully
	/// If it was not successful, we will print out the error message
	if (bWasSuccessful)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Blue,
				FString::Printf(TEXT("Created Session: %s"), *SessionName.ToString())
			);
		}
		
		/// Call ServerTravel to load the game map when we create a session
		/// We will use the "/Game/ThirdPersonCPP/Maps/Lobby" map as the lobby map
		/// Lobby map will load as a listen server, so others will be able to join it
		UWorld* World = GetWorld();
		if (World)
		{
			World->ServerTravel(FString("/Game/ThirdPerson/Maps/Lobby?listen"));
		}
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Red,
				FString::Printf(TEXT("Could not create session %s"), *SessionName.ToString())
			);
		}
			
	}

}


void AMenuSystemCharacter::OnFindSessionsComplete(bool bWasSuccessful)
{
	/// This function will be called in response to the delegate broadcast sent by the session interface.
	/// Once the action of finding sessions has been completed.
	

	/// Check if the OnlineSessionInterface is valid, if not return out of the function
	if (!OnlineSessionInterface.IsValid())
	{
		return;
	}

	
	/// loop through the list of sessions and print out the session names
	/// We will use the SessionSearch TSharedPtr to get the list of sessions
	for (auto Result : SessionSearch->SearchResults)
	{	
		/// Get the session name
		FString Id = Result.GetSessionIdStr();
		/// Get the session owning player
		FString User = Result.Session.OwningUserName;
		
		/// Store the MatchType
		FString MatchType;
		/// Get the match type, using the Get function on the SessionSearch
		Result.Session.SessionSettings.Get(FName("MatchType"), MatchType);
		
		
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
			-1,
			15.f,
			FColor::Cyan,
			FString::Printf(TEXT("Found Session Id: %s, User: %s"), *Id, *User)
			);
		}
		
		
		/// Check if the match type is FreeForAll
		if (MatchType == FString("FreeForAll"))
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
				-1,
				15.f,
				FColor::Cyan,
				FString::Printf(TEXT("Joining Match Type: %s"), *MatchType)
				);
			}

		
			/// Add the JoinSessionCompleteDelegate to the OnlineSessionInterface using the AddOnJoinSessionCompleteDelegate_Handle list
			/// When the session is joined, the OnJoinSessionComplete function will be called, which is bound the OnJoinSessionCompleteDelegate
			OnlineSessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);

			/// Get the LocalPlayer by using GetWorld() and then GetFirstLocalPlayerController()
			const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
		
			/// Call the JoinSession function on the OnlineSessionInterface, passing in the FUniqueNetId and the SessionName
			/// This will join the session we found earlier
			OnlineSessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, Result);
		}
	}
}


void AMenuSystemCharacter::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	/// This function will be called in response to the delegate broadcast sent by the session interface.
	/// Once the action of joining a session has been completed.
	
	/// Check if the OnlineSessionInterface is valid, if not return out of the function
	if (!OnlineSessionInterface.IsValid())
	{
		return;
	}
	
	/// Get the IP address of the session
	/// Store the IP address in the FString variable "Address"
	FString Address;
	
	/// Check if the session was joined successfully
	if (OnlineSessionInterface->GetResolvedConnectString(NAME_GameSession, Address))
	{
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
			PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
		}
		
	}
}


void AMenuSystemCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	Jump();
}

void AMenuSystemCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	StopJumping();
}

void AMenuSystemCharacter::TurnAtRate(float Rate)
{
	/// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void AMenuSystemCharacter::LookUpAtRate(float Rate)
{
	/// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void AMenuSystemCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		/// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		/// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AMenuSystemCharacter::MoveRight(float Value)
{
	if ( (Controller != nullptr) && (Value != 0.0f) )
	{
		/// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		/// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		/// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}
