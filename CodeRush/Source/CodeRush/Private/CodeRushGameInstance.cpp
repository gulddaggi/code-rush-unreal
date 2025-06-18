// Fill out your copyright notice in the Description page of Project Settings.


#include "CodeRushGameInstance.h"
#include "Json.h"
#include "JsonUtilities.h"

void UCodeRushGameInstance::Init()
{
	Super::Init();

	UE_LOG(LogTemp, Log, TEXT("CodeRush GameInstance Init"));
	CurrentProblemIndex = 0;
}

void UCodeRushGameInstance::CreateUser(const FString& Nickname)
{
	TSharedRef<FJsonObject> RequestBody = MakeShared<FJsonObject>();
	RequestBody->SetStringField("nickname", Nickname);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RequestBody, Writer);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL("http://localhost:8080/api/users");
	Request->SetVerb("POST");
	Request->SetHeader("Content-Type", "application/json");
	Request->SetContentAsString(OutputString);
	Request->OnProcessRequestComplete().BindUObject(this, &UCodeRushGameInstance::OnCreateUserResponse);
	Request->ProcessRequest();
}

void UCodeRushGameInstance::OnCreateUserResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[CreateUser] Http request failed"));
		return;
	}

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		CurrentUserId = JsonObject->GetIntegerField("id");
		UE_LOG(LogTemp, Log, TEXT("[CreateUser] User created with ID: %d"), CurrentUserId);

		FTimerHandle TempHandle;
		GetWorld()->GetTimerManager().SetTimer(TempHandle, [this]()
			{
				GetProblemSet();
			}, 1.0f, false);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[CreateUser] Failed to parse JSON response"));
	}
}

void UCodeRushGameInstance::GetProblemSet()
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL("http://localhost:8080/api/problems/set");
	Request->SetVerb("GET");
	Request->SetTimeout(60.0f);
	Request->SetHeader("Content-Type", "application/json");
	Request->OnProcessRequestComplete().BindUObject(this, &UCodeRushGameInstance::OnGetProblemSetResponse);
	bool bDispatched = Request->ProcessRequest();
	UE_LOG(LogTemp, Warning, TEXT("[HTTP] Sending GET to: %s"), *Request->GetURL());

	if (!bDispatched)
	{
		UE_LOG(LogTemp, Error, TEXT("[HTTP] ProcessRequest failed to dispatch request"));
	}
}

void UCodeRushGameInstance::SubmitObjectiveAnswer(int32 ProblemId, const FString& SelectedChoice, const FString& Category, const FString& TargetSnippet, const FString& FixAttempt)
{
	TSharedRef<FJsonObject> RequestBody = MakeShared<FJsonObject>();
	RequestBody->SetNumberField("problemId", ProblemId);
	RequestBody->SetStringField("selectedChoice", SelectedChoice);

	if (Category.Equals("BUGFIX", ESearchCase::IgnoreCase))
	{
		RequestBody->SetStringField("targetSnippet", TargetSnippet);
		RequestBody->SetStringField("fixAttempt", FixAttempt);
	}

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RequestBody, Writer);

	FString Endpoint = FString::Printf(
		TEXT("http://localhost:8080/api/submit/%s?userId=%d"),
		*Category.ToLower(),
		CurrentUserId
	);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Endpoint);
	Request->SetVerb("POST");
	Request->SetHeader("Content-Type", "application/json");
	Request->SetContentAsString(OutputString);
	Request->OnProcessRequestComplete().BindUObject(this, &UCodeRushGameInstance::OnSubmitAnswerResponse);
	Request->ProcessRequest();
}

void UCodeRushGameInstance::SubmitSubjectiveAnswer(int32 ProblemId, const FString& WrittenAnswer, const FString& Category, const FString& TargetSnippet)
{
	TSharedRef<FJsonObject> RequestBody = MakeShared<FJsonObject>();
	RequestBody->SetNumberField("problemId", ProblemId);
	RequestBody->SetStringField("fixAttempt", WrittenAnswer);
	RequestBody->SetStringField("targetSnippet", TargetSnippet);
	RequestBody->SetStringField("selectedChoice", TEXT("")); // API 요구상 항상 포함

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RequestBody, Writer);

	FString Endpoint = FString::Printf(
		TEXT("http://localhost:8080/api/submit/%s?userId=%d"),
		*Category.ToLower(),
		CurrentUserId
	);

	UE_LOG(LogTemp, Log, TEXT("[Submit] Endpoint: %s, ProblemId: %d, TargetSnippet: %s"), *Endpoint, ProblemId, *TargetSnippet);
	UE_LOG(LogTemp, Warning, TEXT("[Submit] TargetSnippet:\n%s"), *TargetSnippet);
	UE_LOG(LogTemp, Warning, TEXT("[Submit] WrittenAnswer:\n%s"), *WrittenAnswer);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Endpoint);
	Request->SetVerb("POST");
	Request->SetHeader("Content-Type", "application/json");
	Request->SetContentAsString(OutputString);
	Request->OnProcessRequestComplete().BindUObject(this, &UCodeRushGameInstance::OnSubmitAnswerResponse);
	Request->ProcessRequest();
}

void UCodeRushGameInstance::OnSubmitAnswerResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (bWasSuccessful && Response.IsValid())
	{
		FString ResponseStr = Response->GetContentAsString();
		UE_LOG(LogTemp, Log, TEXT("[SubmitAnswer] Success: %s"), *ResponseStr);

		bool bIsCorrect = ResponseStr.Equals(TEXT("true"), ESearchCase::IgnoreCase);

		if (bIsCorrect)
		{
			UE_LOG(LogTemp, Log, TEXT("[SubmitAnswer] Correct ✅"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[SubmitAnswer] Incorrect ❌"));
		}

		OnAnswerResultReceived.Broadcast(bIsCorrect);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[SubmitAnswer] Failed"));
	}
}

void UCodeRushGameInstance::OnGetProblemSetResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{

	if (!bWasSuccessful || !Response.IsValid())
	{
		if (!bWasSuccessful)
		{
			UE_LOG(LogTemp, Error, TEXT("[GetProblemSet] Request failed to send"));
		}
		else if (!Response.IsValid())
		{
			UE_LOG(LogTemp, Error, TEXT("[GetProblemSet] Response invalid"));
		}

		return;
	}

	FString ContentType = Response->GetHeader("Content-Type");
	UE_LOG(LogTemp, Warning, TEXT("[GetProblemSet] Response Content-Type: %s"), *ContentType);

	int32 StatusCode = Response->GetResponseCode();
	UE_LOG(LogTemp, Warning, TEXT("[GetProblemSet] HTTP Response Code: %d"), StatusCode);

	UE_LOG(LogTemp, Warning, TEXT("Response code: %d"), Response->GetResponseCode());

	TArray<TSharedPtr<FJsonValue>> JsonArray;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

	UE_LOG(LogTemp, Log, TEXT("[GetProblemSet] Raw Response:\n%s"), *Response->GetContentAsString());

	if (FJsonSerializer::Deserialize(Reader, JsonArray))
	{
		ProblemSet.Empty();
		for (TSharedPtr<FJsonValue> Value : JsonArray)
		{
			TSharedPtr<FJsonObject> Obj = Value->AsObject();
			FProblemDTO Problem;

			// id (Null-safe 처리)
			int32 TmpId;
			if (Obj->HasField("id"))
			{
				FString IdString;
				if (Obj->TryGetStringField("id", IdString))
				{
					Problem.id = FCString::Atoi(*IdString);
				}
				else if (Obj->TryGetNumberField("id", TmpId))
				{
					Problem.id = TmpId;
				}
				else
				{
					Problem.id = -1;
				}
			}
			else
			{
				Problem.id = -1;
			}

			Problem.category = Obj->GetStringField("category");
			Problem.type = Obj->GetStringField("type");
			Problem.title = Obj->GetStringField("title");
			Problem.answer = Obj->GetStringField("answer");

			FString FullDescription = Obj->GetStringField("description");
			Problem.description = FullDescription; // 우선 전체 저장

			FString DescPart, CodePart;
			if (FullDescription.Split(TEXT("코드:"), &DescPart, &CodePart))
			{
				Problem.description = DescPart.TrimStartAndEnd();
				CodePart.TrimStartInline();
				Problem.targetSnippet = CodePart; // 기본적으로 description에 있는 코드 사용
			}


			// JSON 필드 기반 targetSnippet이 존재할 경우만 덮어쓰기
			FString TmpString;
			if (Problem.targetSnippet.IsEmpty())
			{
				if (Obj->TryGetStringField("targetSnippet", TmpString))
				{
					Problem.targetSnippet = TmpString;
				}
			}

			// correctFix
			if (Obj->TryGetStringField("correctFix", TmpString))
			{
				Problem.correctFix = TmpString;
			}
			else
			{
				Problem.correctFix = TEXT("");
			}

			// Choices 파싱
			Problem.choices.Empty();
			const TArray<TSharedPtr<FJsonValue>>* ChoicesArray;
			if (Obj->TryGetArrayField("choices", ChoicesArray))
			{
				for (auto ChoiceValue : *ChoicesArray)
				{
					Problem.choices.Add(ChoiceValue->AsString());
				}
			}

			ProblemSet.Add(Problem);
		}

		UE_LOG(LogTemp, Log, TEXT("[GetProblemSet] Loaded %d problems"), ProblemSet.Num());
		CurrentProblemIndex = 0;
		OnProblemSetLoaded.Broadcast();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[GetProblemSet] Failed to parse JSON array"));
	}
}

void UCodeRushGameInstance::GoToNextProblem()
{
	if (CurrentProblemIndex + 1 < ProblemSet.Num())
	{
		CurrentProblemIndex++;
		UE_LOG(LogTemp, Log, TEXT("[NextProblem] Moving to Problem #%d"), CurrentProblemIndex);

		OnProblemChanged.Broadcast(ProblemSet[CurrentProblemIndex]);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[NextProblem] No more problems left."));
	}

}