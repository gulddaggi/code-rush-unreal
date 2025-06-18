// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Http.h"
#include "CodeRushGameInstance.generated.h"

UENUM(BlueprintType)
enum class EGamePhase : uint8
{
	Title       UMETA(DisplayName = "Title"),
	Lobby       UMETA(DisplayName = "Lobby"),
	Loading     UMETA(DisplayName = "Loading"),
	InGame      UMETA(DisplayName = "InGame"),
	Result      UMETA(DisplayName = "Result")
};

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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAnswerResultReceived, bool, bIsCorrect);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProblemChanged, const FProblemDTO&, Problem);

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

	UPROPERTY(BluepringReadWrite)
	EGamePhase CurrentPhase;

	UPROPERTY(BlueprintAssignable)
	FProblemSetLoadedDelegate OnProblemSetLoaded;

	UPROPERTY(BlueprintAssignable)
	FOnAnswerResultReceived OnAnswerResultReceived;

	UPROPERTY(BlueprintAssignable)
	FOnProblemChanged OnProblemChanged;

	UFUNCTION(BlueprintCallable)
	void GoToNextProblem();

	UFUNCTION(BlueprintCallable)
	void SetGamePhase(EGamePhase NewPhase);

private:
	void OnCreateUserResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);
	void OnGetProblemSetResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);
	void OnSubmitAnswerResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};
