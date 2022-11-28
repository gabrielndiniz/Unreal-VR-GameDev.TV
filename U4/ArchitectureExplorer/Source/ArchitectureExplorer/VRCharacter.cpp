// Fill out your copyright notice in the Description page of Project Settings.

#include "VRCharacter.h"

#include <string>

#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
#include "Components/CapsuleComponent.h"
#include "NavigationSystem.h"
#include "NavigationData.h"
#include "../../Plugins/Developer/RiderLink/Source/RD/thirdparty/clsocket/src/ActiveSocket.h"
#include "Components/PostProcessComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MotionControllerComponent.h"

// Sets default values
AVRCharacter::AVRCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	VRRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VRRoot"));
	VRRoot->SetupAttachment(GetRootComponent());

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(VRRoot);

	LeftController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("LeftController"));
	LeftController->SetupAttachment(VRRoot);
	LeftController->SetTrackingSource(EControllerHand::Left);

	RightController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightController"));
	RightController->SetupAttachment(VRRoot);
	RightController->SetTrackingSource(EControllerHand::Right);

	DestinationMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DestinationMarker"));
	DestinationMarker->SetupAttachment(GetRootComponent());
	

	PostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcessComponent"));
	PostProcessComponent->SetupAttachment(GetRootComponent());
}

// Called when the game starts or when spawned
void AVRCharacter::BeginPlay()
{
	Super::BeginPlay();

	DestinationMarker->SetVisibility(false);

	PC = Cast<APlayerController>(GetController());
	if (PC != nullptr)
	{
		PC->GetViewportSize(ViewPortSizeX,ViewPortSizeY);
	}
	if (BlinkerMaterialBase != nullptr)
	{
		BlinkerMaterialInstance = UMaterialInstanceDynamic::Create(BlinkerMaterialBase, this);
		PostProcessComponent->AddOrUpdateBlendable(BlinkerMaterialInstance);

		BlinkerMaterialInstance->SetScalarParameterValue(TEXT("Radius"), 1);
	}

	LeftController->bDisplayDeviceModel = true;
	RightController->bDisplayDeviceModel = true;
}

// Called every frame
void AVRCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector NewCameraOffset = Camera->GetComponentLocation() - GetActorLocation();
	NewCameraOffset.Z = 0;
	AddActorWorldOffset(NewCameraOffset);
	VRRoot->AddWorldOffset(-NewCameraOffset);

	UpdateDestinationMarker();

	UpdateBlinkers();
	
}

bool AVRCharacter::FindTeleportDestination(FVector &OutLocation)
{
	FVector Start;
	FVector Look;
	FVector End;
	if (RightController == nullptr && LeftController == nullptr)
	{
		Start = Camera->GetComponentLocation();
		Look = Camera->GetForwardVector();
		End = Start + Look * MaxTeleportDistance;
	}
	else if (TeleportWithRightHand())
	{
		Start = RightController->GetComponentLocation();
		Look = RightController->GetForwardVector();
		Look = Look.RotateAngleAxis(30,RightController->GetRightVector());
		End = Start + Look * MaxTeleportDistance;
	}
	else
	{
		Start = LeftController->GetComponentLocation();
		Look = LeftController->GetForwardVector();
		Look = Look.RotateAngleAxis(30,LeftController->GetRightVector());
		End = Start + Look * MaxTeleportDistance;
	}

	FHitResult HitResult;
	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility);

	if (!bHit) return false;

	FNavLocation NavLocation;
	//bool bOnNavMesh = GetWorld()->GetNavigationSystem()->ProjectPoint(HitResult.Location, NavLocation, TeleportProjectionExtent);
	bool bOnNavMesh = GetWorld()->GetNavigationSystem()->GetMainNavData()->ProjectPoint(HitResult.Location, NavLocation, TeleportProjectionExtent);
	
	if (!bOnNavMesh) return false;

	OutLocation = NavLocation.Location;

	return true;
}

bool AVRCharacter::TeleportWithRightHand()
{
	if (FVector::DotProduct(Camera->GetForwardVector(),RightController->GetForwardVector())>=
	FVector::DotProduct(Camera->GetForwardVector(),LeftController->GetForwardVector()))
	{
		return true;
	}
	return false;
}


void AVRCharacter::UpdateDestinationMarker()
{
	FVector Location;
	bool bHasDestination = FindTeleportDestination(Location);

	if (bHasDestination)
	{
		DestinationMarker->SetVisibility(true);

		DestinationMarker->SetWorldLocation(Location);
	} 
	else
	{
		DestinationMarker->SetVisibility(false);
	}
}

// Called to bind functionality to input
void AVRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("Move_Y"), this, &AVRCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("Move_X"), this, &AVRCharacter::MoveRight);
	PlayerInputComponent->BindAction(TEXT("Teleport"), IE_Released, this, &AVRCharacter::BeginTeleport);
}

void AVRCharacter::MoveForward(float throttle)
{
	AddMovementInput(throttle * Camera->GetForwardVector());
}

void AVRCharacter::MoveRight(float throttle)
{
	AddMovementInput(throttle * Camera->GetRightVector());
}

void AVRCharacter::BeginTeleport()
{

	StartFade(0, 1);

	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, this, &AVRCharacter::FinishTeleport, TeleportFadeTime);
}

void AVRCharacter::FinishTeleport()
{
	SetActorLocation(DestinationMarker->GetComponentLocation() + GetCapsuleComponent()->GetScaledCapsuleHalfHeight());

	StartFade(1, 0);
}

void AVRCharacter::StartFade(float FromAlpha, float ToAlpha)
{
	if (PC != nullptr)
	{
		PC->PlayerCameraManager->StartCameraFade(FromAlpha, ToAlpha, TeleportFadeTime, FLinearColor::Black);
	}
}

void AVRCharacter::UpdateBlinkers()
{
	if (RadiusVsVelocity == nullptr)
	{
		return;
	}

	float Speed = GetVelocity().Size();

if (Speed > 0 && Speed > MaxConsideredVelocity)
{
	MaxConsideredVelocity = Speed;
}
	
	float Radius = RadiusVsVelocity->GetFloatValue(Speed/MaxConsideredVelocity);
    BlinkerMaterialInstance->SetScalarParameterValue(TEXT("Radius"),Radius);

	FVector2D Centre = GetBlinkerCentre();
	BlinkerMaterialInstance->SetVectorParameterValue(TEXT("Centre"), FLinearColor(Centre.X,Centre.Y,0));
	
}

FVector2D AVRCharacter::GetBlinkerCentre()
{
	if (PC == nullptr)
	{
		return FVector2D (0.5,0.5);
	}
	if (GetVelocity().IsNearlyZero())
	{
		return FVector2D (0.5,0.5);
	}
	FVector MovementDirection = GetVelocity().GetSafeNormal();
	//At least ten meter distance (1000 units)

	FVector WorldStationaryLocation;
	if (FVector::DotProduct(Camera->GetForwardVector(),MovementDirection)>=0)
	{
		WorldStationaryLocation = Camera->GetComponentLocation()+MovementDirection*1000;
	}
	else
	{
		WorldStationaryLocation = Camera->GetComponentLocation()-MovementDirection*1000;
	}
	FVector2D ScreenStationaryLocation;
	PC->ProjectWorldLocationToScreen(WorldStationaryLocation, ScreenStationaryLocation);
	ScreenStationaryLocation.X /= ViewPortSizeX;
	ScreenStationaryLocation.Y /= ViewPortSizeY;
	
	return ScreenStationaryLocation;
}

