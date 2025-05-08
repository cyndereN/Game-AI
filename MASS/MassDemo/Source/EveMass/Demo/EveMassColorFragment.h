// Copyright Night Gamer, Inc. All Rights Reserved.

#pragma once

#include "MassCommonFragments.h"
#include "EveMassColorFragment.generated.h"

USTRUCT()
struct EVEMASS_API FEveMassColorFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FLinearColor Color;
};