// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Heart.generated.h"

class UStaticMeshComponent;
class UCapsuleComponent;

UCLASS()
class STOLENMATES_API AHeart : public AActor
{
	GENERATED_BODY()

public:
	AHeart();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		UStaticMeshComponent* heartMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
		UCapsuleComponent* heartCollider;
};