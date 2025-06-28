// Fill out your copyright notice in the Description page of Project Settings.


#include "CodeRushGameInstance.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "Blueprint/UserWidget.h"

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
	Request->SetTimeout(300.0f);
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
			CorrectAnswerCount++;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[SubmitAnswer] Incorrect ❌"));
			IncorrectProblems.Add(ProblemSet[CurrentProblemIndex]);
		}

		OnAnswerResultReceived.Broadcast(bIsCorrect);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[SubmitAnswer] Failed"));
	}
}

void UCodeRushGameInstance::SendProblemRequest()
{
	TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL("http://localhost:8080/api/problems/request");
	Request->SetVerb("POST");
	Request->SetHeader("Content-Type", "application/json");
	Request->OnProcessRequestComplete().BindUObject(this, &UCodeRushGameInstance::OnProblemRequestResponse);
	Request->ProcessRequest();
}

void UCodeRushGameInstance::OnProblemRequestResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (bWasSuccessful && Response->GetResponseCode() == 200)
	{
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		{
			ProblemRequestId = JsonObject->GetStringField("requestId");
			UE_LOG(LogTemp, Log, TEXT("[ProblemRequest] Received requestId: %s"), *ProblemRequestId);

			// 3초마다 polling
			GetWorld()->GetTimerManager().SetTimer(PollingTimerHandle, this, &UCodeRushGameInstance::CheckProblemResult, 3.0f, true);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[ProblemRequest] Failed to request problem set."));
	}
}

void UCodeRushGameInstance::CheckProblemResult()
{
	FString URL = FString::Printf(TEXT("http://localhost:8080/api/problems/result/%s"), *ProblemRequestId);

	TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(URL);
	Request->SetVerb("GET");
	Request->SetHeader("Content-Type", "application/json");
	Request->OnProcessRequestComplete().BindLambda([this](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess)
		{
			if (bSuccess && Resp->GetResponseCode() == 200)
			{
				UE_LOG(LogTemp, Log, TEXT("[CheckProblemResult] Problem set ready!"));
				OnGetProblemSetResponse(Req, Resp, true);

				GetWorld()->GetTimerManager().ClearTimer(PollingTimerHandle);
			}
			else if (bSuccess && Resp->GetResponseCode() == 204)
			{
				UE_LOG(LogTemp, Log, TEXT("[CheckProblemResult] Problem set not ready yet, polling continues..."));
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[CheckProblemResult] Failed to get problem result. Stop polling."));
				GetWorld()->GetTimerManager().ClearTimer(PollingTimerHandle);
			}
		});
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
		for (auto& Value : JsonArray)
		{
			TSharedPtr<FJsonObject> Obj = Value->AsObject();
			if (!Obj.IsValid()) continue;

			FProblemDTO Problem;
			Problem.id = Obj->GetIntegerField("id");
			Problem.category = Obj->GetStringField("category");
			Problem.type = Obj->GetStringField("type");
			Problem.title = Obj->GetStringField("title");
			Problem.description = Obj->GetStringField("description");
			Problem.answer = Obj->GetStringField("answer");

			FString TmpString;
			if (Obj->TryGetStringField("targetSnippet", TmpString))
				Problem.targetSnippet = TmpString;
			if (Obj->TryGetStringField("correctFix", TmpString))
				Problem.correctFix = TmpString;

			const TArray<TSharedPtr<FJsonValue>>* ChoicesArray;
			if (Obj->TryGetArrayField("choices", ChoicesArray))
			{
				for (const auto& ChoiceValue : *ChoicesArray)
				{
					Problem.choices.Add(ChoiceValue->AsString());
				}
			}

			ProblemSet.Add(Problem);
		}

		UE_LOG(LogTemp, Log, TEXT("[GetProblemSet] Loaded %d problems"), ProblemSet.Num());
		
		CurrentProblemIndex = 0;
		CorrectAnswerCount = 0;
		IncorrectProblems.Empty();

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
		SetGamePhase(EGamePhase::Result);
	}

}

void UCodeRushGameInstance::SetGamePhase(EGamePhase NewPhase)
{
	CurrentPhase = NewPhase;

	// 기존 위젯 제거
	if (CurrentWidget)
	{
		CurrentWidget->RemoveFromParent();
		CurrentWidget = nullptr;
	}

	FString WidgetPath;
	switch (NewPhase)
	{
		case EGamePhase::Title:
			WidgetPath = TEXT("/Game/UI/WBP_TitleScreen.WBP_TitleScreen_C");
			break;
		case EGamePhase::Lobby:
			WidgetPath = TEXT("/Game/UI/WBP_LobbyScreen.WBP_LobbyScreen_C");
			break;
		case EGamePhase::Loading:
			WidgetPath = TEXT("/Game/UI/WBP_LoadingScreen.WBP_LoadingScreen_C");
			break;
		case EGamePhase::InGame:
			WidgetPath = TEXT("/Game/UI/WBP_ProblemScreen.WBP_ProblemScreen_C");
			break;
		case EGamePhase::Result:
			WidgetPath = TEXT("/Game/UI/WBP_ResultScreen.WBP_ResultScreen_C");
			break;
		default:
			return;
	}

	TSubclassOf<UUserWidget> WidgetClass = LoadClass<UUserWidget>(nullptr, *WidgetPath);
	if (WidgetClass)
	{
		CurrentWidget = CreateWidget<UUserWidget>(GetWorld(), WidgetClass);
		if (CurrentWidget)
		{
			CurrentWidget->AddToViewport();
		}
	}
}

void UCodeRushGameInstance::ResetGameState()
{
	CurrentProblemIndex = 0;
	CorrectAnswerCount = 0;
	ProblemSet.Empty();
	IncorrectProblems.Empty();
}