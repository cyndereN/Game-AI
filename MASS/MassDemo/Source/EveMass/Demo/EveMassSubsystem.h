// Copyright Night Gamer, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EveMassSubsystem.generated.h"

UCLASS()
class EVEMASS_API UEveMassSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
};