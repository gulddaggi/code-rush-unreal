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
	FString SelectedChoice; // 빈 문자열 넣어도 됨 (API 요구사항 상 필요)

	UPROPERTY(BlueprintReadWrite)
	FString WrittenAnswer;

	FSubjectSubmissionRequest()
		: ProblemId(0), SelectedChoice(TEXT("")), WrittenAnswer(TEXT("")) {}

	FSubjectSubmissionRequest(int32 InProblemId, const FString& InWrittenAnswer)
		: ProblemId(InProblemId), SelectedChoice(TEXT("")), WrittenAnswer(InWrittenAnswer) {}
};
