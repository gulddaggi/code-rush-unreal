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

		GetProblemSet();
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
	Request->SetHeader("Content-Type", "application/json");
	Request->OnProcessRequestComplete().BindUObject(this, &UCodeRushGameInstance::OnGetProblemSetResponse);
	Request->ProcessRequest();
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

void UCodeRushGameInstance::SubmitSubjectiveAnswer(int32 ProblemId, const FString& WrittenAnswer, const FString& Category)
{
	TSharedRef<FJsonObject> RequestBody = MakeShared<FJsonObject>();
	RequestBody->SetNumberField("problemId", ProblemId);
	RequestBody->SetStringField("writtenAnswer", WrittenAnswer);
	RequestBody->SetStringField("selectedChoice", TEXT("")); // API �䱸�� �׻� ����

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RequestBody, Writer);

	FString Endpoint = FString::Printf(
		TEXT("http://localhost:8080/api/submit/%s?userId=%d"),
		*Category.ToLower(),
		CurrentUserId
	);

	UE_LOG(LogTemp, Log, TEXT("[Submit] Endpoint: %s, ProblemId: %d"), *Endpoint, ProblemId);

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
		UE_LOG(LogTemp, Log, TEXT("[SubmitAnswer] Success: %s"), *Response->GetContentAsString());
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
		UE_LOG(LogTemp, Error, TEXT("[GetProblemSet] HTTP request failed"));
		return;
	}

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
			// id (Null-safe ó��)
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
			Problem.description = Obj->GetStringField("description");
			Problem.answer = Obj->GetStringField("answer");

			// Null-safe �Ľ�
			FString TmpString;
			if (Obj->TryGetStringField("targetSnippet", TmpString))
			{
				Problem.targetSnippet = TmpString;
			}
			else
			{
				Problem.targetSnippet = TEXT("");
			}

			if (Obj->TryGetStringField("correctFix", TmpString))
			{
				Problem.correctFix = TmpString;
			}
			else
			{
				Problem.correctFix = TEXT("");
			}

			// Choices �Ľ�
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