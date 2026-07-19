#include "StolenmatesPlayer.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "TimerManager.h"
#include "Heart.h"
#include "AbilityBaseClass.h"



// Sets default values
AStolenmatesPlayer::AStolenmatesPlayer()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &AStolenmatesPlayer::OnCompHit);
	GetCapsuleComponent()->OnComponentBeginOverlap.AddDynamic(this, &AStolenmatesPlayer::OnCompOverlap);
}

// Called when the game starts or when spawned
void AStolenmatesPlayer::BeginPlay()
{
	Super::BeginPlay();
	JumpMaxHoldTime = MaxJumpHoldTime;
	GetCharacterMovement()->MaxWalkSpeed = MovementSpeedWithoutHeart;
}

void AStolenmatesPlayer::LeftRightAxis(float val)
{
	if (bInputLocked)
		return;
	if (gameOver)
		return;
	if (stunned)
		return;

	AddMovementInput(FVector(0.0f, 1.0f, 0.0f), val);
	PlayerMovementDirection.Y = val;
	FVector FlatDirection = PlayerMovementDirection;
	FlatDirection.Z = 0.0f;

	if (!FlatDirection.IsNearlyZero())
	{
		previousMovementDirection = FlatDirection.GetSafeNormal();
	}
}

void AStolenmatesPlayer::ForwardBackAxis(float val)
{
	if (bInputLocked)
		return;
	if (gameOver)
		return;
	if (stunned)
		return;

	AddMovementInput(FVector(1.0f, 0.0f, 0.0f), val);
	PlayerMovementDirection.X = val;
	FVector FlatDirection = PlayerMovementDirection;
	FlatDirection.Z = 0.0f;

	if (!FlatDirection.IsNearlyZero())
	{
		previousMovementDirection = FlatDirection.GetSafeNormal();
	}
}

void AStolenmatesPlayer::JumpReleased()
{
	StopJumping();
}

void AStolenmatesPlayer::JumpPressed()
{
	if (bInputLocked)
		return;
	if (gameOver)
		return;
	if (!hasHeart && !stunned && !Mini)
	{
		Jump();
	}
}

void AStolenmatesPlayer::SteamActionJumpPressed()
{
	JumpPressed();
	DashPressed();
}

void AStolenmatesPlayer::SteamActionJumpReleased()
{
	JumpReleased();
}

void AStolenmatesPlayer::SteamActionUseAbilityPressed()
{
	SteamInputUseAbilityPressed_BP();
}

void AStolenmatesPlayer::SteamActionPausePressed()
{
	if (gameOver)
	{
		return;
	}

	SteamInputPausePressed_BP();
}

void AStolenmatesPlayer::SteamActionMove(float MoveRight, float MoveForward)
{
	LeftRightAxis(MoveRight);
	ForwardBackAxis(MoveForward);
}

void AStolenmatesPlayer::DashPressed()
{
	if (bInputLocked)
		return;
	if (gameOver)
		return;

	FVector DashDirection = PlayerMovementDirection;
	DashDirection.Z = 0.0f;

	if (DashDirection.IsNearlyZero())
	{
		DashDirection = previousMovementDirection;
		DashDirection.Z = 0.0f;
	}

	if (!HasAuthority())
	{
		ServerDashPressed(DashDirection);
		return;
	}

	PerformDash(DashDirection);
}


void AStolenmatesPlayer::ServerDashPressed_Implementation(FVector DashDirection)
{
	PerformDash(DashDirection);
}

void AStolenmatesPlayer::PerformDash(FVector DashDirection)
{
	if (bInputLocked)
		return;
	
	if (gameOver)
		return;

	if (!hasHeart)
		return;

	if (stunned)
		return;

	if (Mini)
		return;

	if (!dashReady)
		return;

	DashDirection.Z = 0.0f;

	if (DashDirection.IsNearlyZero())
	{
		return;
	}

	DashDirection.Normalize();

	dash = true;
	LaunchCharacter(DashDirection * DashForce, false, true);

	dashReady = false;
	GetWorldTimerManager().SetTimer(dashTimerHandle, this, &AStolenmatesPlayer::DashReset, DashCooldown, false);

}

void AStolenmatesPlayer::DashReset()
{
	dashReady = true;
}

void AStolenmatesPlayer::EndStun()
{
	stunned = false;
	catfished = false;
}

void AStolenmatesPlayer::EndInvincibility()
{
	invincible = false;
}

void AStolenmatesPlayer::UseAbility()
{
	if (bInputLocked)
		return;

	if (gameOver)
		return;

	if (stunned)
		return;

	if (!HasAuthority())
	{
		ServerUseAbility();
		return;
	}

	if (overrideAbility)
	{
		overrideAbility->fireAbility(this);
		return;
	}

	if (heldAbility)
	{
		heldAbility->fireAbility(this);
		return;
	}

}



void AStolenmatesPlayer::ServerUseAbility_Implementation()
{
	UseAbility();
}




void AStolenmatesPlayer::SetInvincibility(float iTime)
{
	invincible = true;
	GetWorldTimerManager().SetTimer(invinciblityTimerHandle, this, &AStolenmatesPlayer::EndInvincibility, iTime, false);
}

void AStolenmatesPlayer::StunPlayer(float StunDuration)
{
	stunned = true;
	JumpReleased();
	GetWorldTimerManager().SetTimer(stunTimerHandle, this, &AStolenmatesPlayer::EndStun, StunDuration, false);
}

void AStolenmatesPlayer::SetAbility(
	AbilitiesENUM ability,
	AAbilityBaseClass* staticAbility)
{
	switch (ability)
	{
	case AbilitiesENUM::ITEM_BOX_ABILITY:
		heldAbility = staticAbility;
		break;

	case AbilitiesENUM::ZONE_ABILITY_OVERRIDE:
		overrideAbility = staticAbility;
		break;
	}
}

void AStolenmatesPlayer::OnCompHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (bInputLocked || gameOver)
	{
		return;
	}

	AStolenmatesPlayer* OtherPlayer =
		Cast<AStolenmatesPlayer>(OtherActor);

	if (!OtherPlayer ||
		!hasHeart ||
		OtherPlayer->stunned ||
		invincible ||
		!heart)
	{
		return;
	}

	heart->AttachToComponent(
		OtherPlayer->GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		socketName
	);

	if (heart->heartCollider)
	{
		heart->heartCollider->SetCollisionEnabled(
			ECollisionEnabled::NoCollision
		);
	}

	hasHeart = false;
	holdingHeart = false;

	OtherPlayer->hasHeart = true;
	OtherPlayer->holdingHeart = true;
	OtherPlayer->heart = heart;

	OtherPlayer->SetInvincibility(
		HeartGainInvincibilityDuration
	);

	OtherPlayer->GetCharacterMovement()->MaxWalkSpeed =
		MovementSpeedWithHeart;

	GetCharacterMovement()->MaxWalkSpeed =
		MovementSpeedWithoutHeart;

	StunPlayer(HeartLossStunDuration);
}

void AStolenmatesPlayer::OnCompOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (bInputLocked || gameOver)
	{
		return;
	}

	AHeart* OtherHeart = Cast<AHeart>(OtherActor);

	if (!OtherHeart)
	{
		return;
	}

	OtherHeart->AttachToComponent(
		GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		socketName
	);

	heart = OtherHeart;
	hasHeart = true;
	holdingHeart = true;

	if (heart->heartCollider)
	{
		heart->heartCollider->SetCollisionEnabled(
			ECollisionEnabled::NoCollision
		);
	}
}


void AStolenmatesPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (bInputLocked)
		return;
	
	if (gameOver)
		return;
	
	if (HasAuthority() && hasHeart)
	{
		timeHoldingHeart += DeltaTime;
	}

}

// Called to bind functionality to input
void AStolenmatesPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAxis("MoveRight", this, &AStolenmatesPlayer::LeftRightAxis);
	PlayerInputComponent->BindAxis("MoveForward", this, &AStolenmatesPlayer::ForwardBackAxis);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AStolenmatesPlayer::JumpPressed);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AStolenmatesPlayer::DashPressed);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &AStolenmatesPlayer::JumpReleased);
	PlayerInputComponent->BindAction("UseAbility", IE_Pressed, this, &AStolenmatesPlayer::UseAbility);
}



void AStolenmatesPlayer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AStolenmatesPlayer, PlayerIndex);
	DOREPLIFETIME(AStolenmatesPlayer, timeHoldingHeart);
	DOREPLIFETIME(AStolenmatesPlayer, stunned);
	DOREPLIFETIME(AStolenmatesPlayer, catfished);
	DOREPLIFETIME(AStolenmatesPlayer, hasHeart);
	DOREPLIFETIME(AStolenmatesPlayer, holdingHeart);
	DOREPLIFETIME(AStolenmatesPlayer, gameOver);
	DOREPLIFETIME(AStolenmatesPlayer, invincible);
	DOREPLIFETIME(AStolenmatesPlayer, Mini);
	DOREPLIFETIME(AStolenmatesPlayer, bInputLocked);
}
