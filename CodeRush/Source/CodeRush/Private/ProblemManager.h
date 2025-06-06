// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "HttpModule.h"
#include "Http.h"
#include "ProblemManager.generated.h"

/**
 * 
 */
UCLASS()
class CODERUSH_API UProblemManager : public UObject
{
	GENERATED_BODY()

public:
	void RequestProblems();

private:
	void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	
};
