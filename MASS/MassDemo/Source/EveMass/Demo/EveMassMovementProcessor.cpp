// Copyright Night Gamer, Inc. All Rights Reserved.

#include "EveMassMovementProcessor.h"
#include "EveMassActorFragment.h"
#include "EveMassMovementFragment.h"
#include "EveMassColorFragment.h"
#include "EveMassRandomMovementTag.h"
#include "MassExecutionContext.h"
#include "MassRepresentationTypes.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"

UEveMassMovementProcessor::UEveMassMovementProcessor()
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = (int32)EProcessorExecutionFlags::All;
	UE_LOG(LogTemp, Log, TEXT("UEveMassMovementProcessor initialized!"));
}

void UEveMassMovementProcessor::ConfigureQueries()
{
	EntityQuery.AddRequirement<FEveMassMovementFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FEveMassActorFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FEveMassColorFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddTagRequirement<FEveMassRandomMovementTag>(EMassFragmentPresence::All);
	EntityQuery.RegisterWithProcessor(*this);
}

void UEveMassMovementProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	// Define movement bounds
	const FVector MovementBoundsMin = FVector(-1000.0f, -1000.0f, -1000.0f);
	const FVector MovementBoundsMax = FVector(1000.0f, 1000.0f, 1000.0f);

	EntityQuery.ForEachEntityChunk(EntityManager, Context, [MovementBoundsMin, MovementBoundsMax](FMassExecutionContext& Context)
	{
		const TArrayView<FEveMassMovementFragment> MovementFragments = Context.GetMutableFragmentView<FEveMassMovementFragment>();
		const TArrayView<FEveMassActorFragment> ActorFragments = Context.GetMutableFragmentView<FEveMassActorFragment>();
		const TArrayView<FEveMassColorFragment> ColorFragments = Context.GetMutableFragmentView<FEveMassColorFragment>();

		for (int32 i = 0; i < Context.GetNumEntities(); ++i)
		{
			FVector& Velocity = MovementFragments[i].Velocity;
			AActor* Actor = ActorFragments[i].Actor;
			FLinearColor& Color = ColorFragments[i].Color;

			if (Actor)
			{
				// Update actor position
				FVector NewLocation = Actor->GetActorLocation() + Velocity * Context.GetDeltaTimeSeconds();

				// Clamp position within bounds
				NewLocation.X = FMath::Clamp(NewLocation.X, MovementBoundsMin.X, MovementBoundsMax.X);
				NewLocation.Y = FMath::Clamp(NewLocation.Y, MovementBoundsMin.Y, MovementBoundsMax.Y);
				NewLocation.Z = FMath::Clamp(NewLocation.Z, MovementBoundsMin.Z, MovementBoundsMax.Z);

				Actor->SetActorLocation(NewLocation);

				// Simple bounce logic
				if (NewLocation.X >= MovementBoundsMax.X || NewLocation.X <= MovementBoundsMin.X) Velocity.X *= -1;
				if (NewLocation.Y >= MovementBoundsMax.Y || NewLocation.Y <= MovementBoundsMin.Y) Velocity.Y *= -1;
				if (NewLocation.Z >= MovementBoundsMax.Z || NewLocation.Z <= MovementBoundsMin.Z) Velocity.Z *= -1;

				// Update color based on velocity
				float Speed = Velocity.Size();
				Color = FLinearColor(FMath::Sin(Speed * 0.01f), FMath::Cos(Speed * 0.01f), FMath::Sin(Speed * 0.02f), 1.0f);

				// Apply color to actor's material
				TArray<UActorComponent*> TaggedComponents = Actor->GetComponentsByTag(UStaticMeshComponent::StaticClass(), TEXT("Cube"));
				if (TaggedComponents.Num() > 0)
				{
					UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>(TaggedComponents[0]);
					if (MeshComponent)
					{
						// Get or create dynamic material instance
						UMaterialInterface* BaseMaterial = MeshComponent->GetMaterial(0);
						if (BaseMaterial)
						{
							UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(BaseMaterial);
							if (!DynamicMaterial)
							{
								DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, nullptr);
								MeshComponent->SetMaterial(0, DynamicMaterial);
							}

							// Set color parameter
							DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), Color);
						}
					}
				}
			}
		}
	});
}