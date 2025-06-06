#pragma once

#include "CoreMinimal.h"
#include "ObjectiveSubmissionRequest.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct CODERUSH_API FObjectiveSubmissionRequest
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	int32 ProblemId;

	UPROPERTY(BlueprintReadWrite)
	FString SelectedChoice;

	UPROPERTY(BlueprintReadWrite)
	FString TargetSnippet; // BUGFIX ������ ���� ���

	UPROPERTY(BlueprintReadWrite)
	FString FixAttempt;    // BUGFIX ������ ���� ���

	// ������
	FObjectiveSubmissionRequest()
		: ProblemId(0), SelectedChoice(TEXT("")), TargetSnippet(TEXT("")), FixAttempt(TEXT("")) {}

	FObjectiveSubmissionRequest(int32 InProblemId, const FString& InSelectedChoice, const FString& InTargetSnippet = TEXT(""), const FString& InFixAttempt = TEXT(""))
		: ProblemId(InProblemId), SelectedChoice(InSelectedChoice), TargetSnippet(InTargetSnippet), FixAttempt(InFixAttempt) {}
};
