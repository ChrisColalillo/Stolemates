// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Camera.generated.h"


class ACharacter;
class UCameraComponent;
class USpringArmComponent;
UCLASS()
class STOLENMATES_API ACamera : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACamera();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera Settings")
		float minHeight = 1000;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera Settings")
		float maxHeight = 4000;

	UCameraComponent* CameraComp;
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	void Focus(TArray<ACharacter*> players);

};
