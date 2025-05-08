// Copyright Night Gamer, Inc. All Rights Reserved.

#pragma once

#include "MassProcessor.h"
#include "EveMassMovementProcessor.generated.h"

UCLASS()
class EVEMASS_API UEveMassMovementProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UEveMassMovementProcessor();

protected:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
	
};