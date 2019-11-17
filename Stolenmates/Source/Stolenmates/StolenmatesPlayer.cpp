#include "StolenmatesPlayer.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "TimerManager.h"
#include "Math/UnrealMathUtility.h"
#include "Heart.h"

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
	if (gameOver)
		return;
	if (stunned)
		return;
	AddMovementInput(GetActorRightVector(), val);
	PlayerMovementDirection.Y = val;
}

void AStolenmatesPlayer::ForwardBackAxis(float val)
{
	if (gameOver)
		return;
	if (stunned)
		return;
	AddMovementInput(GetActorForwardVector(), val);
	PlayerMovementDirection.X = val;
}

void AStolenmatesPlayer::JumpReleased()
{
	StopJumping();
}

void AStolenmatesPlayer::JumpPressed()
{
	if (gameOver)
		return;
	if (!hasHeart && !stunned)
	{
		Jump();
	}
}

void AStolenmatesPlayer::DashPressed()
{
	if (gameOver)
		return;
	if (hasHeart && !stunned)
	{
		if (dashReady)
		{
			
			LaunchCharacter(PlayerMovementDirection* DashForce, false, true);
			dashReady = false;
			GetWorldTimerManager().SetTimer(dashTimerHandle, this, &AStolenmatesPlayer::DashReset, DashCooldown, false);
		}
	}
}

void AStolenmatesPlayer::DashReset()
{
	dashReady = true;
}

void AStolenmatesPlayer::EndStun()
{
	stunned = false;
}

void AStolenmatesPlayer::StunPlayer(float StunDuration)
{
	stunned = true;
	JumpReleased();
	GetWorldTimerManager().SetTimer(stunTimerHandle, this, &AStolenmatesPlayer::EndStun, StunDuration, false);
}

void AStolenmatesPlayer::OnCompHit(UPrimitiveComponent * HitComponent, AActor * OtherActor, UPrimitiveComponent * OtherComponent, FVector NormalImpulse, const FHitResult & Hit)
{
	if (gameOver)
		return;
	AStolenmatesPlayer* other = Cast<AStolenmatesPlayer>(OtherActor);
	if (other)
	{
		if (hasHeart && !other->stunned)
		{
			heart->AttachToActor(OtherActor, FAttachmentTransformRules::KeepRelativeTransform,"HeartSocket");
			hasHeart = false;
			other->hasHeart = true;
			other->heart = heart;
			other->GetCharacterMovement()->MaxWalkSpeed = MovementSpeedWithHeart;
			GetCharacterMovement()->MaxWalkSpeed = MovementSpeedWithoutHeart;
			StunPlayer(HeartLossStunDuration);
		}
	}
}

void AStolenmatesPlayer::OnCompOverlap(UPrimitiveComponent * OverlappedComponent, AActor * OtherActor, UPrimitiveComponent * OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	if (gameOver)
		return;
	AHeart* other = Cast<AHeart>(OtherActor);
	if (other)
	{
		OtherActor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform, "HeartSocket");
		heart = other;
		hasHeart = true;
		heart->heartCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}


void AStolenmatesPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (gameOver)
		return;
	if (hasHeart)
		timeHoldingHeart += DeltaTime;
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
}
