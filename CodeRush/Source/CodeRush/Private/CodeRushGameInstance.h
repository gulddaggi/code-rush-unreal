// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Http.h"
#include "CodeRushGameInstance.generated.h"

USTRUCT(BlueprintType)
struct FProblemDTO {
	GENERATED_BODY()

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

/**
 * 
 */
UCLASS()
class UCodeRushGameInstance : public UGameInstance{
	GENERATED_BODY()

public:
	virtual void Init() override;

	UFUNCTION(BlueprintCallable)
	void CreateUser(const FString& Nickname);

	UFUNCTION(BlueprintCallable)
	void GetProblemSet();

	UPROPERTY(BlueprintReadWrite)
	int32 CurrentUserId;

	UPROPERTY(BlueprintReadWrite)
	TArray<FProblemDTO> ProblemSet;

private:
	void OnCreateUserResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);
	void OnGetProblemSetResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);
};
