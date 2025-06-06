// Fill out your copyright notice in the Description page of Project Settings.


#include "ProblemManager.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "JsonUtilities.h"

void UProblemManager::RequestProblems()
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

	Request->SetURL("http://localhost:8080/api/problems/set");
}

