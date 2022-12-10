// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HandController.generated.h"

UCLASS()
class ARCHITECTUREEXPLORER_API AHandController : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AHandController();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	
	void SetHand(EControllerHand ControllerHand);
	
	//Callback
	UFUNCTION(BlueprintCallable)
	void ActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor, bool bNewCanClimb);
	UFUNCTION(BlueprintCallable)
	void ActorEndOverlap(AActor* OverlappedActor, AActor* OtherActor, bool bNewCanClimb);

private:

	//Helpers

	bool CanClimbFunctionC() const;
	
private:
	//Default sub object
	UPROPERTY(VisibleAnywhere)
	class UMotionControllerComponent* MotionController;
	

private:

	//State
	bool bCanClimb = false;
};
