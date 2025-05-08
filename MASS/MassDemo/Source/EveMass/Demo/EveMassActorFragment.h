// Copyright Night Gamer, Inc. All Rights Reserved.

#pragma once

#include "MassEntityTypes.h"
#include "EveMassActorFragment.generated.h"

USTRUCT()
struct EVEMASS_API FEveMassActorFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	AActor* Actor;
};