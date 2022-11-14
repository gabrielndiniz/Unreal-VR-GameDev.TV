// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "VRCharacter.generated.h"


UCLASS()
class ARCHITECTUREEXPLORER_API AVRCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AVRCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private: //functions

	void UpdateDestinationMarker();
	
	void MoveForward(float throttle);
	void MoveRight(float throttle);

	void BeginTeleport();
	void FinishTeleport();

	void FadeOut();
	void FadeIn();
	
	
private: //variables

	UPROPERTY(VisibleAnywhere)
	class UCameraComponent* Camera;

	UPROPERTY()
	APlayerController* PlayerController;

	UPROPERTY()
	APlayerCameraManager* CameraManager;

	UPROPERTY()
	FTimerHandle TimeHandle;
	
	UPROPERTY(VisibleAnywhere)
	class USceneComponent* VRRoot;

	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* DestinationMarker;

	UPROPERTY(EditAnywhere)
	float speed = 1000.f;

	UPROPERTY(EditAnywhere)
	float MaxTeleportDistance = 1000;

	UPROPERTY()
	FVector MarkerDistance = FVector(0,0,0);
	
	UPROPERTY(EditAnywhere)
	float FadeInDuration = 1;
	
	UPROPERTY(EditAnywhere)
	float FadeOutDuration = 1;
	
	UPROPERTY(EditAnywhere)
	bool bFadeInAudio = false;

	UPROPERTY(EditAnywhere)
	bool bFadeOutAudio = false;
};
