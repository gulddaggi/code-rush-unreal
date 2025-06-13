// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Http.h"
#include "CodeRushGameInstance.generated.h"

USTRUCT(BlueprintType)
struct CODERUSH_API FProblemDTO {
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	int32 id;

	UPROPERTY(BlueprintReadWrite)
	FString category;

	UPROPERTY(BlueprintReadWrite)
	FString type;

	UPROPERTY(BlueprintReadWrite)
	FString title;

	UPROPERTY(BlueprintReadWrite)
	FString description;

	UPROPERTY(BlueprintReadWrite)
	FString answer;

	UPROPERTY(BlueprintReadWrite)
	FString targetSnippet;

	UPROPERTY(BlueprintReadWrite)
	FString correctFix;

	UPROPERTY(BlueprintReadWrite)
	TArray<FString> choices;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FProblemSetLoadedDelegate);

/**
 * 
 */
UCLASS()
class CODERUSH_API UCodeRushGameInstance : public UGameInstance{
	GENERATED_BODY()

public:
	virtual void Init() override;

	UFUNCTION(BlueprintCallable)
	void CreateUser(const FString& Nickname);

	UFUNCTION(BlueprintCallable)
	void GetProblemSet();

	UFUNCTION(BlueprintCallable)
	void SubmitSubjectiveAnswer(int32 ProblemId, const FString& WrittenAnswer, const FString& Category, const FString& TargetSnippet);

	UFUNCTION(BlueprintCallable)
	void SubmitObjectiveAnswer(int32 ProblemId, const FString& SelectedChoice, const FString& Category, const FString& TargetSnippet, const FString& FixAttempt);

	UPROPERTY(BlueprintReadWrite)
	int32 CurrentUserId;

	UPROPERTY(BlueprintReadWrite)
	TArray<FProblemDTO> ProblemSet;

	UPROPERTY(BlueprintReadWrite)
	int32 CurrentProblemIndex;

	UPROPERTY(BlueprintAssignable)
	FProblemSetLoadedDelegate OnProblemSetLoaded;

private:
	void OnCreateUserResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);
	void OnGetProblemSetResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);
	void OnSubmitAnswerResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};
