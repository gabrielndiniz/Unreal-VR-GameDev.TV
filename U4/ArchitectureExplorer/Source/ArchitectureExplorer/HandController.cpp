// Fill out your copyright notice in the Description page of Project Settings.


#include "HandController.h"
#include "MotionControllerComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PhysicsVolume.h"

// Sets default values
AHandController::AHandController()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MotionController"));
	SetRootComponent(MotionController);
}

// Called when the game starts or when spawned
void AHandController::BeginPlay()
{
	Super::BeginPlay();

	
	
}

// Called every frame
void AHandController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


}

void AHandController::SetHand(EControllerHand ControllerHand)
{
	ThisControllerHand = ControllerHand;
	MotionController->SetTrackingSource(ControllerHand);
	MotionController->bDisplayDeviceModel = true;
}

void AHandController::PairController(AHandController* Controller)
{
	OtherController = Controller;
	OtherController->OtherController = this;
}


void AHandController::ActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor, bool bNewCanClimb)
{
	
	//bool bNewCanClimb = CanClimbFunctionC();
	if (!bCanClimb && bNewCanClimb)
	{
		VRPlayerController->PlayHapticEffect(HapticEffect, ThisControllerHand);
	}
	bCanClimb = bNewCanClimb;
}

void AHandController::ActorEndOverlap(AActor* OverlappedActor, AActor* OtherActor, bool bNewCanClimb)
{
	bCanClimb = bNewCanClimb;
}

void AHandController::GetVRPlayerController(APlayerController* PlayerController)
{
	VRPlayerController = PlayerController;
}


bool AHandController::CanClimbFunctionC() const
{
	
	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors);
	for (AActor* OverlappingActor : OverlappingActors)
	{
		
		if(OverlappingActor->ActorHasTag(TEXT("Climbable")))
		{
			return true;
		}
	}
	return false;
}

void AHandController::Grip()
{
	if (!bCanClimb)
	{
		return;
	}

	if (!bIsClimbing)
	{
		ClimbingStartLocation = GetActorLocation();

		bIsClimbing = true;
	}

	if (OtherController->bIsClimbing)
	{
		OtherController->Release();
	}

}

void AHandController::Release()
{
	bIsClimbing = false;
	
}

