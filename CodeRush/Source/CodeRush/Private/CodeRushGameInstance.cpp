// Fill out your copyright notice in the Description page of Project Settings.


#include "CodeRushGameInstance.h"
#include "Json.h"
#include "JsonUtilities.h"

void UCodeRushGameInstance::Init()
{
	Super::Init();

	UE_LOG(LogTemp, Log, TEXT("CodeRush GameInstance Init"));
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

void UCodeRushGameInstance::OnGetProblemSetResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[GetProblemSet] HTTP request failed"));
		return;
	}

	TArray<TSharedPtr<FJsonValue>> JsonArray;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

	if (FJsonSerializer::Deserialize(Reader, JsonArray))
	{
		ProblemSet.Empty();
		for (TSharedPtr<FJsonValue> Value : JsonArray)
		{
			TSharedPtr<FJsonObject> Obj = Value->AsObject();
			FProblemDTO Problem;
			Problem.type = Obj->GetStringField("type");
			Problem.title = Obj->GetStringField("title");
			Problem.description = Obj->GetStringField("description");
			Problem.answer = Obj->GetStringField("answer");
			Problem.targetSnippet = Obj->GetStringField("targetSnippet");
			Problem.correctFix = Obj->GetStringField("correctFix");

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
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[GetProblemSet] Failed to parse JSON array"));
	}
}

