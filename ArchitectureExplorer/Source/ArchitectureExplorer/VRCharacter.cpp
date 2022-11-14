// Fill out your copyright notice in the Description page of Project Settings.


#include "VRCharacter.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
#include "Components/CapsuleComponent.h"

// Sets default values
AVRCharacter::AVRCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	VRRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VRRoot"));
	VRRoot->SetupAttachment(GetRootComponent());


	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(VRRoot);
		
	DestinationMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DestinationMarker"));
	DestinationMarker->SetupAttachment(GetRootComponent());
	
}

// Called when the game starts or when spawned
void AVRCharacter::BeginPlay()
{
	Super::BeginPlay();
	DestinationMarker->SetVisibility(false);

	PlayerController = Cast<APlayerController>(GetController());

	if (!PlayerController)
	{
		return;
	}
	CameraManager = PlayerController->PlayerCameraManager;
	
}

// Called every frame
void AVRCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	FVector NewCameraOffset = Camera->GetComponentLocation() - GetActorLocation(); //It is possible to smooth this value
	NewCameraOffset.Z = 0;	
	AddActorWorldOffset(NewCameraOffset, true);
	VRRoot->AddWorldOffset(-NewCameraOffset, true);

	UpdateDestinationMarker();

	

	
}

void AVRCharacter::UpdateDestinationMarker()
{
	FVector Start = Camera->GetComponentLocation();
	FVector End = Start+Camera->GetForwardVector()*MaxTeleportDistance;
	
	FHitResult HitResult;
	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility);
	DestinationMarker->SetVisibility(bHit);
	if(bHit)
	{
		DestinationMarker->SetWorldLocation(HitResult.Location);
	}
	else
	{
		DestinationMarker->SetRelativeLocation(FVector(0,0,0));
	}
}


// Called to bind functionality to input
void AVRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Respond every frame to the values of our two movement axes, "MoveX" and "MoveY".
	PlayerInputComponent->BindAxis(TEXT("Forward"), this, &AVRCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("Right"), this, &AVRCharacter::MoveRight);
	PlayerInputComponent->BindAction(TEXT("Teleport"),IE_Released, this, &AVRCharacter::BeginTeleport);

}


void AVRCharacter::MoveForward(float throttle)
{
	// Move at speed forward or backward
	AddMovementInput(throttle * Camera->GetForwardVector() * speed);
} 

void AVRCharacter::MoveRight(float throttle)
{
	// Move at speed right or left
	AddMovementInput(throttle * Camera->GetRightVector() * speed);
}

void AVRCharacter::BeginTeleport()
{
	FadeOut();
	GetWorldTimerManager().SetTimer(TimeHandle, FadeOutDuration,false,-1);
	FinishTeleport();
}

void AVRCharacter::FinishTeleport()
{
	FVector Destination =DestinationMarker->GetComponentLocation();
	Destination.Z = Destination.Z+GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	SetActorLocation(Destination);
	FadeIn();
}


void AVRCharacter::FadeOut()
{
	if (!PlayerController)
	{
		return;
	}
	CameraManager->StartCameraFade(0,1,FadeInDuration, FLinearColor::Black,bFadeInAudio,true);
}

void AVRCharacter::FadeIn()
{
	if (!PlayerController)
	{
		return;
	}
	CameraManager->StartCameraFade(1,0,FadeOutDuration, FLinearColor::Black,bFadeOutAudio,true);
}


