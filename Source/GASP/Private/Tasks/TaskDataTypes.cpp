// Fill out your copyright notice in the Description page of Project Settings.


#include "Tasks/TaskDataTypes.h"


// Sets default values
ATaskDataTypes::ATaskDataTypes()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ATaskDataTypes::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ATaskDataTypes::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ATaskDataTypes::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

