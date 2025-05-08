// Copyright Night Gamer, Inc. All Rights Reserved.

#include "EveMassSubsystem.h"
#include "EveMassActor.h"
#include "EveMassActorFragment.h"
#include "EveMassMovementFragment.h"
#include "EveMassColorFragment.h"
#include "EveMassRandomMovementTag.h"
#include "MassEntitySubsystem.h"
#include "MassSpawnerSubsystem.h"

void UEveMassSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	// Get Mass subsystems
	UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	UMassSpawnerSubsystem* SpawnerSubsystem = GetWorld()->GetSubsystem<UMassSpawnerSubsystem>();
	if (!EntitySubsystem || !SpawnerSubsystem) return;

	// Get the Mass Entity Manager
	FMassEntityManager& EntityManager = EntitySubsystem->GetMutableEntityManager();

	// Define archetype
	FMassEntityTemplate EntityTemplate;
	FMassArchetypeHandle Archetype = EntityManager.CreateArchetype({FEveMassMovementFragment::StaticStruct(), FEveMassActorFragment::StaticStruct(), FEveMassColorFragment::StaticStruct(), FEveMassRandomMovementTag::StaticStruct()});
	EntityTemplate.SetArchetype(Archetype);

	// Spawn entities and actors
	const int32 NumEntities = 100;
	TArray<FMassEntityHandle> Entities;
	SpawnerSubsystem->SpawnEntities(EntityTemplate, NumEntities, Entities);

	for (const FMassEntityHandle& Entity : Entities)
	{
		FEveMassMovementFragment* MovementFragment = EntityManager.GetFragmentDataPtr<FEveMassMovementFragment>(Entity);
		FEveMassActorFragment* ActorFragment = EntityManager.GetFragmentDataPtr<FEveMassActorFragment>(Entity);
		FEveMassColorFragment* ColorFragment = EntityManager.GetFragmentDataPtr<FEveMassColorFragment>(Entity);

		if (MovementFragment && ActorFragment && ColorFragment)
		{
			// Spawn an actor
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			UClass* BlueprintClass = LoadClass<AEveMassActor>(nullptr, TEXT("/Script/Engine.Blueprint'/Game/BP_EveMassActor.BP_EveMassActor_C'"));
			AEveMassActor* Actor = GetWorld()->SpawnActor<AEveMassActor>(BlueprintClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

			// Initialize fragments
			MovementFragment->Velocity = FVector(FMath::FRandRange(-100.0f, 100.0f), FMath::FRandRange(-100.0f, 100.0f), FMath::FRandRange(-100.0f, 100.0f));
			ActorFragment->Actor = Actor;
			ColorFragment->Color = FLinearColor(FMath::FRand(), FMath::FRand(), FMath::FRand(), 1.0f);
		}
	}
}