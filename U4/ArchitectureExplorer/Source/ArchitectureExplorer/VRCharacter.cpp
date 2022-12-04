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
#include "Kismet/GameplayStatics.h"
#include "Components/SplineComponent.h"

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

	//TeleportPath = CreateDefaultSubobject<USplineComponent>(TEXT("TeleportPath"));
	//TeleportPath->SetupAttachment(VRRoot);

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

	WorldContextObject = this;

	if (TeleportPath == nullptr)
	{
		TeleportPath = NewObject<USplineComponent>(this);
		TeleportPath->SetupAttachment(VRRoot);
		TeleportPath->ClearSplinePoints(true);
	}

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

bool AVRCharacter::FindTeleportDestination(TArray<FVector> &OutPath, FVector &OutLocation)
{
	FVector Start;
	FVector Look;
	if (RightController == nullptr && LeftController == nullptr)
	{
		Start = Camera->GetComponentLocation();
		Look = Camera->GetForwardVector();
		if (Hand != 0)
		{
			TeleportPath->SetupAttachment(VRRoot);
			TeleportPath->ClearSplinePoints(true);
			Hand=0;
		}
	}
	else if (TeleportWithRightHand())
	{
		Start = RightController->GetComponentLocation();
		Look = RightController->GetForwardVector();
		if (Hand != 1)
		{
			TeleportPath->SetupAttachment(RightController);
			TeleportPath->ClearSplinePoints(true);
			Hand=1;
		}
	}
	else
	{
		Start = LeftController->GetComponentLocation();
		Look = LeftController->GetForwardVector();
		if (Hand != 2)
		{
			TeleportPath->SetupAttachment(LeftController);
			TeleportPath->ClearSplinePoints(true);
			Hand=2;
		}
	}


	PredictParams = FPredictProjectilePathParams(TeleportProjectileRadius,
		Start,
		Look*TeleportProjectileSpeed,
		TeleportSimulationTime,
		ECC_Visibility,
		this //I forgot to IGNORE myself
		);
	PredictParams.bTraceComplex = true; //If it does not have collisions in a good way
	PredictParams.DrawDebugType = EDrawDebugTrace::ForOneFrame;

	bHit = UGameplayStatics::PredictProjectilePath(WorldContextObject, PredictParams, PredictResult);

	OutPath.Empty();
	if(!bHit)
	{
		OutPath.Add(Start);
		return false;
	}

	if (PredictResult.PathData.Num() == 0)
	{
		return false;
	}

	if (PredictResult.PathData.Num() == 1)
	{
		OutPath.Add(Start);
	}

	for (FPredictProjectilePathPointData PointData : PredictResult.PathData)
	{
		OutPath.Add(PointData.Location);
	}
		
	FNavLocation NavLocation;
	bool bOnNavMesh = GetWorld()->GetNavigationSystem()->GetMainNavData()->ProjectPoint(PredictResult.HitResult.Location, NavLocation, TeleportProjectionExtent);
	
	if (!bOnNavMesh) return false;

	OutLocation = NavLocation.Location;

	return true;
}

bool AVRCharacter::TeleportWithRightHand()
{
	if (RightController == nullptr)
	{
		return false;
	}
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
	TArray<FVector> Path;
	
	bool bHasDestination = FindTeleportDestination(Path, Location);

	if (bHasDestination)
	{
		DestinationMarker->SetVisibility(true);

		DestinationMarker->SetWorldLocation(Location);

		DrawTeleportPath(Path);
	} 
	else
	{
		DestinationMarker->SetVisibility(false);
	}
}

void AVRCharacter::DrawTeleportPath(const TArray<FVector>& Path)
{
	UpdateSplinePoints(Path);

	for (int32 i =0; i < Path.Num(); ++i)
	{
		
		if (TeleportPathMeshPool.Num() <= i)
		{
			UStaticMeshComponent* DynamicMesh = NewObject<UStaticMeshComponent>(this);
			DynamicMesh->AttachToComponent(VRRoot,FAttachmentTransformRules::KeepRelativeTransform);
			DynamicMesh->SetStaticMesh(TeleportArchMesh);
			DynamicMesh->SetMaterial(0,TeleportArchMaterial);
			DynamicMesh->SetRelativeScale3D(TeleportArchScale);
			DynamicMesh->RegisterComponent();
			TeleportPathMeshPool.Add(DynamicMesh);
			DynamicMesh->SetWorldLocation(Path[i]);
		}
		else
		{
		TeleportPathMeshPool[i]->SetWorldLocation(Path[i]);
		}

	}
}


void AVRCharacter::UpdateSplinePoints(const TArray<FVector>& Path)
{
	if (TeleportPath == nullptr)
	{
		UE_LOG(LogTemp,Warning,TEXT("UpdateSplinePoints: TeleportPath is nullptr"));
		return;
	}
	TeleportPath->ClearSplinePoints(false);

	for (int32 i =0; i < Path.Num(); ++i)
	{
		FSplinePoint Point(i,Path[i],ESplinePointType::Curve);
		TeleportPath->AddPoint(Point, false);
	}
	
	TeleportPath->UpdateSpline();
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

