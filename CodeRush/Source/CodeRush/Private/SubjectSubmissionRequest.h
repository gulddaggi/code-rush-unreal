#pragma once

#include "CoreMinimal.h"
#include "SubjectSubmissionRequest.generated.h"

USTRUCT(BlueprintType)
struct CODERUSH_API FSubjectSubmissionRequest
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	int32 ProblemId;

	UPROPERTY(BlueprintReadWrite)
	FString SelectedChoice; // �� ���ڿ� �־ �� (API �䱸���� �� �ʿ�)

	UPROPERTY(BlueprintReadWrite)
	FString WrittenAnswer;

	FSubjectSubmissionRequest()
		: ProblemId(0), SelectedChoice(TEXT("")), WrittenAnswer(TEXT("")) {}

	FSubjectSubmissionRequest(int32 InProblemId, const FString& InWrittenAnswer)
		: ProblemId(InProblemId), SelectedChoice(TEXT("")), WrittenAnswer(InWrittenAnswer) {}
};
