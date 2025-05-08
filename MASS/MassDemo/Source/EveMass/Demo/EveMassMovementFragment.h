// Copyright Night Gamer, Inc. All Rights Reserved.

#pragma once

#include "MassEntityTypes.h"
#include "EveMassMovementFragment.generated.h"

USTRUCT()
struct EVEMASS_API FEveMassMovementFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FVector Velocity;
};