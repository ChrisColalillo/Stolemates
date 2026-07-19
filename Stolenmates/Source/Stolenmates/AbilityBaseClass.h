// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilityBaseClass.generated.h"

class AStolenmatesPlayer;

UCLASS()
class STOLENMATES_API AAbilityBaseClass : public AActor
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "Ability")
		void fireAbility(AStolenmatesPlayer* abilityUser);
};