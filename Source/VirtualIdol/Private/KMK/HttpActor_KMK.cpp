// Fill out your copyright notice in the Description page of Project Settings.

#include "KMK/HttpActor_KMK.h"
#include "HttpModule.h"
#include "KMK/JsonParseLib_KMK.h"
#include "GameFramework/Actor.h"
#include "KMK/VirtualGameInstance_KMK.h"
#include "KMK/StartWidget_KMK.h"
#include "Components/CanvasPanel.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "JsonObjectConverter.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "IImageWrapper.h"
#include "JJH/JJH_MapSelectWidget.h"
#include "Kismet/GameplayStatics.h"
#include "JJH/JJH_SelectManager.h"
#include "KMK/AudienceServerComponent_KMK.h"
#include "HSW/HSW_GameState_Auditorium.h"

// Sets default values
AHttpActor_KMK::AHttpActor_KMK()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
}

// Called when the game starts or when spawned
void AHttpActor_KMK::BeginPlay ( )
{
	Super::BeginPlay ( );

	gi = Cast<UVirtualGameInstance_KMK> ( GetWorld ( )->GetGameInstance ( ) );

	if (gi)
	{
		if (gi->IsOfflineMode ( ))
		{
			gi->SetupOfflineDefaults ( );
			loginInfo = gi->GetMyInfo ( );
		}
		else if (!gi->GetMyInfo ( ).token.IsEmpty ( ))
		{
			loginInfo = gi->GetMyInfo ( );
		}

		if (effectArray.Num ( ) > 0)
		{
			gi->effectArray = effectArray;
		}
	}

	SelectManager = Cast<AJJH_SelectManager> ( UGameplayStatics::GetActorOfClass ( this , AJJH_SelectManager::StaticClass ( ) ) );
}

// Called every frame
void AHttpActor_KMK::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
#pragma region Login
// 로그인 관련 서버 연결
void AHttpActor_KMK::ReqLogin ( const FString& id , const FString& pw )
{
	if (!ShouldUseServer ( ))
	{
		HandleOfflineLoginSuccess ( );
		return;
	}

	FHttpModule& httpModule = FHttpModule::Get ( );
	TSharedRef<IHttpRequest> req = httpModule.CreateRequest ( );

	req->SetURL ( TEXT ( "http://back.reward-factory.shop:8123/api/v1/auth/login" ) );
	req->SetVerb ( TEXT ( "POST" ) );
	req->SetHeader ( TEXT ( "content-type" ) , TEXT ( "application/json" ) );
	req->SetContentAsString ( UJsonParseLib_KMK::MakeLoginJson ( id , pw ) );
	req->OnProcessRequestComplete ( ).BindUObject ( this , &AHttpActor_KMK::OnResLogin );
	req->ProcessRequest ( );
}

void AHttpActor_KMK::OnResLogin ( FHttpRequestPtr Request , FHttpResponsePtr Response , bool bConnectedSuccessfully )
{

	if (!bConnectedSuccessfully || !Response.IsValid ( ))
	{
		if (gi) gi->LoginPanel ( );
		return;
	}

	loginInfo = UJsonParseLib_KMK::ParsecMyInfo ( Response->GetContentAsString ( ) );

	if (loginInfo.userImg.Contains ( TEXT ( "http" ) ))
	{
		DownloadImageFromUrl ( loginInfo.userImg , loginInfo );
	}
	else
	{
		if (gi) gi->SwitchWidget ( 1 );
	}

	if (gi && !loginInfo.email.IsEmpty ( ))
	{
		gi->SetMyInfo ( loginInfo );
		gi->SetMyProfile ( );
		ReqCheckMyConcert ( );
	}
	else
	{
		if (gi) gi->LoginPanel ( );
	}
}
#pragma endregion
#pragma region Concert
// 콘서트장 예약하는 부분
void AHttpActor_KMK::ReqSetMyConcert ( FConcertInfo& concert )
{
	if (!ShouldUseServer ( ))
	{
		if (gi)
		{
			gi->concerInfo = concert;
			gi->PopUpVisible ( );
		}
		return;
	}

	FHttpModule& httpModule = FHttpModule::Get ( );
	TSharedRef<IHttpRequest> req = httpModule.CreateRequest ( );

	req->SetURL ( TEXT ( "http://back.reward-factory.shop:8123/api/v1/concerts" ) );
	req->SetVerb ( TEXT ( "POST" ) );
	req->SetHeader ( TEXT ( "content-type" ) , TEXT ( "application/json" ) );
	concert.img = ticketURL;
	req->SetContentAsString ( UJsonParseLib_KMK::MakeConcertJson ( concert ) );

	FString AuthHeader = FString::Printf ( TEXT ( "Bearer %s" ) , *loginInfo.token );
	req->SetHeader ( TEXT ( "Authorization" ) , AuthHeader );

	req->OnProcessRequestComplete ( ).BindUObject ( this , &AHttpActor_KMK::OnResSetConcert );
	req->ProcessRequest ( );
}

void AHttpActor_KMK::OnResSetConcert ( FHttpRequestPtr Request , FHttpResponsePtr Response , bool bConnectedSuccessfully )
{
	if (!bConnectedSuccessfully || !Response.IsValid ( ))
	{
		return;
	}

	if (!bConnectedSuccessfully)
	{
		UE_LOG(LogTemp, Error, TEXT("HTTP 요청 실패"));
        return;
       
    }
	else
	{
        int32 StatusCode = Response->GetResponseCode ( );
        FString ResponseBody = Response->GetContentAsString ( );
		if (StatusCode == 403)
        {
			if(count > 1)  return;
            UE_LOG ( LogTemp , Error , TEXT ( "403 Forbidden: 인증 문제 또는 권한 부족 - 응답 내용: %s" ) , *ResponseBody );
        }
        else
        {
			// 콘서트 예약을 성공한 경우, 성공했다는 팝업을 띄움
            gi->PopUpVisible ( );
        }
	}
}
// 무대 불러오기
void AHttpActor_KMK::ReqCheckStage ( UStartWidget_KMK* startWidget )
{
	if (!startWidget) return;
	sw = startWidget;

	if (!ShouldUseServer ( ))
	{
		sw->ClearSB ( );
		return;
	}

	FHttpModule& httpModule = FHttpModule::Get ( );
	TSharedRef<IHttpRequest> req = httpModule.CreateRequest ( );

	FString authHeader = FString::Printf ( TEXT ( "Bearer %s" ) , *gi->loginInfo.token );
	req->SetHeader ( TEXT ( "Authorization" ) , authHeader );
	req->SetHeader ( TEXT ( "Content-Type" ) , TEXT ( "application/json" ) );
	req->SetURL ( TEXT ( "http://back.reward-factory.shop:8123/api/v1/stages" ) );
	req->SetVerb ( TEXT ( "GET" ) );
	req->OnProcessRequestComplete ( ).BindUObject ( this , &AHttpActor_KMK::OnResCheckStage );
	req->ProcessRequest ( );
}

void AHttpActor_KMK::OnResCheckStage ( FHttpRequestPtr Request , FHttpResponsePtr Response , bool bConnectedSuccessfully )
{
	if (!bConnectedSuccessfully || !Response.IsValid ( ))
	{
		return;
	}

	if (Response->GetResponseCode ( ) == 200)
	{
		UE_LOG ( LogTemp , Error , TEXT ( "CheckStage Successed" ));
		// 성공
		FString respon = Response->GetContentAsString();
		// 서버에 올라가 있는 전체 무대 확인
		allStageInfoArray = UJsonParseLib_KMK::ParsecStageInfos(respon, gi->GetMyInfo().userName);
		for (int i = 0; i < allStageInfoArray.Num ( ); i++)
		{
			// 무대 정보에 있는 img값을 texture 로 변환
			DownloadImageFromUrl(allStageInfoArray[i].img, allStageInfoArray[i] );
			//sw->CreateStageWidget(allStageInfoArray[i] , texture );
		}

		UE_LOG ( LogTemp , Error , TEXT ( "%d" ), allStageInfoArray.Num());
	}
	else
	{
		UE_LOG ( LogTemp , Error , TEXT ( "CheckStage Failed" ));
	}
}

// 내가 만든 공연장이 있다면
void AHttpActor_KMK::ReqCheckMyStage ( UStartWidget_KMK* startWidget )
{
	if (!startWidget) return;
	sw = startWidget;

	if (!ShouldUseServer ( ))
	{
		sw->ClearSB ( );
		return;
	}

	FHttpModule& httpModule = FHttpModule::Get ( );
	TSharedRef<IHttpRequest> req = httpModule.CreateRequest ( );

	FString authHeader = FString::Printf ( TEXT ( "Bearer %s" ) , *gi->loginInfo.token );
	req->SetHeader ( TEXT ( "Authorization" ) , authHeader );
	req->SetHeader ( TEXT ( "Content-Type" ) , TEXT ( "application/json" ) );

	FString url = TEXT ( "http://back.reward-factory.shop:8123/api/v1/stages/user/" ) + FString::FormatAsNumber ( gi->GetMyInfo ( ).userId );
	req->SetURL ( url );
	req->SetVerb ( TEXT ( "GET" ) );
	req->OnProcessRequestComplete ( ).BindUObject ( this , &AHttpActor_KMK::OnResCheckMyStage );
	req->ProcessRequest ( );
}

void AHttpActor_KMK::OnResCheckMyStage ( FHttpRequestPtr Request , FHttpResponsePtr Response , bool bConnectedSuccessfully )
{
	if (!bConnectedSuccessfully || !Response.IsValid ( ))
	{
		return;
	}

	if (bConnectedSuccessfully)
	{
		// 성공 => 내가 만든 세트장이 있다면
		FString respon = Response->GetContentAsString();
		// 스테이지를 array에 넣어주고
		myStageInfoArray = UJsonParseLib_KMK::ParsecStageInfos(respon, gi->GetMyInfo().userName);
		// 각각의 스테이지에 있는 썸네일을 넣어준다
		for (int i = 0; i < myStageInfoArray.Num ( ); i++)
		{
			FConcertInfo concert;
			// 이 코드가 백엔드에 들어간 썸네일을 들고오는 부분
			DownloadImageFromUrl(myStageInfoArray[i].img, myStageInfoArray[i] );
			//sw->CreateStageWidget(myStageInfoArray[i] , texture );
			UE_LOG ( LogTemp , Error , TEXT ( "%s" ) , *myStageInfoArray[i].name);
		}
	}
	else
	{
		UE_LOG ( LogTemp , Warning , TEXT ( "Failed myStage" ) );
	}
}
// 백엔드에서 무대와 관련된 이미지를 생성하는 함수
void AHttpActor_KMK::DownloadImageFromUrl(const FString& imageUrl, const FStageInfo& stageInfo)
{
	FHttpModule* http = &FHttpModule::Get();
    TSharedRef<IHttpRequest> req = http->CreateRequest();
    req->SetURL(imageUrl);
    req->SetVerb("GET");
    req->OnProcessRequestComplete().BindUObject(this, &AHttpActor_KMK::OnImageDownComplete, stageInfo);
    req->ProcessRequest();
}

// 백엔드에서 내 프로필과 관련된 이미지를 생성하는 함수 => StartWidget에서 4개의 선택지에 나오는 프로필 사진
void AHttpActor_KMK::DownloadImageFromUrl ( const FString& imageUrl , const FLoginInfo& Info )
{
	FHttpModule* http = &FHttpModule::Get ( );
	TSharedRef<IHttpRequest> req = http->CreateRequest ( );
	req->SetURL ( imageUrl );
	req->SetVerb ( "GET" );
	req->OnProcessRequestComplete ( ).BindUObject ( this , &AHttpActor_KMK::OnImageDownComplete , Info );
	req->ProcessRequest ( );
}
//  백엔드에서 무대와 관련된 이미지가 생성된 경우
void AHttpActor_KMK::OnImageDownComplete ( FHttpRequestPtr Request , FHttpResponsePtr Response , bool bWasSuccessful , FStageInfo stageInfo )
{
	if (!bWasSuccessful || !Response.IsValid ( ))
	{
		return;
	}

	if (Response->GetResponseCode ( ) != 200)
	{
		return;
	}

	FString ContentType = Response->GetContentType ( );
	if (!ContentType.Contains ( TEXT ( "image" ) ))
	{
		return;
	}

	const TArray<uint8>& ImageData = Response->GetContent ( );
	UTexture2D* texture = UJsonParseLib_KMK::MakeTexture ( ImageData );

	if (texture && sw)
	{
		sw->CreateStageWidget ( stageInfo , texture );
	}
}

//  백엔드에서 프로필과 관련된 이미지가 생성된 경우
void AHttpActor_KMK::OnImageDownComplete ( FHttpRequestPtr Request , FHttpResponsePtr Response , bool bWasSuccessful , FLoginInfo Info )
{
	if (bWasSuccessful && Response.IsValid ( ))
	{
		if (Response->GetResponseCode ( ) != 200)
		{
			UE_LOG ( LogTemp , Error , TEXT ( "Failed to get image. Response Code: %d" ) , Response->GetResponseCode ( ) );
			return;
		}
		FString ContentType = Response->GetContentType ( );
		UE_LOG ( LogTemp , Log , TEXT ( "Content-Type: %s" ) , *ContentType );

		if (!ContentType.Contains ( "image" ))
		{
			UE_LOG ( LogTemp , Error , TEXT ( "Unexpected Content-Type: %s" ) , *ContentType );
			return;
		}
		// 다운로드 받은 PNG 데이터를 파일로 저장
		const TArray<uint8>& ImageData = Response->GetContent ( );
		// 이미지에서 texture로 생성
		UTexture2D* texture = UJsonParseLib_KMK::MakeTexture ( ImageData );
		if (texture)
		{
			UE_LOG ( LogTemp , Log , TEXT ( "Image downloaded and texture created successfully!" ) );
			// 내 정보에 texture 넣기
			Info.texture = texture;
			// 4개의 선택지 창에 있는 프로필 변경
			gi->widget->SetImageProfile(texture);
			// 4개의 선택지 창으로 이동
			gi->SwitchWidget ( 1 );
		}
		else
		{
			UE_LOG ( LogTemp , Error , TEXT ( "Failed to create texture from downloaded image data." ) );
		}
	}
}

#pragma endregion

#pragma region with Ai for Ticket
// 티켓 만들기 => AI 측에 만들어달라고 신호보내기
void AHttpActor_KMK::ReqTicket ( const TMap<FString , FString> data )
{
	if (!ShouldUseServer ( ))
	{
		HandleOfflineTicketCreated ( );
		return;
	}

	FHttpModule& httpModule = FHttpModule::Get ( );
	TSharedRef<IHttpRequest> req = httpModule.CreateRequest ( );

	if (ticketData.Num ( ) <= 0)
	{
		ticketData = data;
	}

	req->SetURL ( TEXT ( "https://singular-swine-deeply.ngrok-free.app/generate-image" ) );
	req->SetVerb ( TEXT ( "POST" ) );
	req->SetHeader ( TEXT ( "content-type" ) , TEXT ( "application/json" ) );
	req->SetTimeout ( 240.f );
	req->SetContentAsString ( UJsonParseLib_KMK::CreateTicketJson ( ticketData ) );
	req->OnProcessRequestComplete ( ).BindUObject ( this , &AHttpActor_KMK::OnResTicket );
	req->ProcessRequest ( );
}

// 응답 받기
void AHttpActor_KMK::OnResTicket ( FHttpRequestPtr Request , FHttpResponsePtr Response , bool bConnectedSuccessfully )
{
	if (!bConnectedSuccessfully || !Response.IsValid ( ))
	{
		return;
	}

 	if (bConnectedSuccessfully && Response.IsValid() && Response->GetResponseCode() == 200)
    {
        // 다운로드 받은 PNG 데이터를 파일로 저장
        TArray<uint8> ImageData = Response->GetContent();

        FString RequestUrl = Request->GetURL();

        FString Nickname;
        FParse::Value(*RequestUrl, TEXT("filename="), Nickname);
        FString FullPath = FPaths::ProjectSavedDir() / TEXT("Pictures/") / Nickname + TEXT(".png");

		UE_LOG ( LogTemp , Warning , TEXT ( "FullPath : %s "), *FullPath );
        if (FFileHelper::SaveArrayToFile(ImageData, *FullPath))
        {
			// 받은 이미지를 서버를 통해 경로를 받아놓음 => 서버에 올리기 위해 필요
			ReqMultipartCapturedWithAI(FullPath);
			// 이미지 => 텍스쳐 변경
            UTexture2D* Texture = UJsonParseLib_KMK::MakeTexture(ImageData);
            if (Texture)
            {
                UE_LOG(LogTemp, Log, TEXT("Texture successfully created from downloaded image"));
				// 티켓이 생성되면 true로 변경하여 로딩바가 사라지게 만듦
				sw->bCreateTicket = true;
				// 로딩바 hidden처리
				sw->SetLoadImage();
                // 만들어진 텍스처를 위젯에 넣기
                OnTextureCreated(Texture);  // Custom method to handle the texture
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to decode the image data."));
            }
        }
        
    }
   else
   {
       if (Response.IsValid())
       {
           UE_LOG(LogTemp, Error, TEXT("Failed to download AI Ticket image. Response code: %d"), Response->GetResponseCode());
       }
       else
       {
           UE_LOG(LogTemp, Error, TEXT("Response is invalid"));
		   if(ticketData.Num() > 0) ReqTicket(ticketData );
       }
   }

}
void AHttpActor_KMK::ReqMultipartCapturedWithAI (const FString& ImagePath , const FString& url )
{
	if (!ShouldUseServer ( ))
	{
		ticketURL = ImagePath;

		if (gi && gi->widget)
		{
			gi->widget->SetTicketButton ( );
		}
		return;
	}

	UE_LOG ( LogTemp , Warning , TEXT ( "Image upload start. 2" ) );

	// Create an HTTP request for the multipart upload
	TSharedRef<IHttpRequest> ImageUploadRequest = FHttpModule::Get ( ).CreateRequest ( );
	ImageUploadRequest->OnProcessRequestComplete ( ).BindUObject ( this , &AHttpActor_KMK::OnReqMultipartCapturedWithAI );

	// Set the URL and verb for the image upload request
	ImageUploadRequest->SetURL ( url );
	ImageUploadRequest->SetVerb ( TEXT ( "POST" ) );

	UE_LOG ( LogTemp , Warning , TEXT ( "ImagePath: %s" ) , *ImagePath );
	UE_LOG ( LogTemp , Warning , TEXT ( "ImageURL: %s" ) , *url );

	// Prepare multipart form data for image upload
	TArray<uint8> FileData;
	bool bSuccess = FFileHelper::LoadFileToArray ( FileData , *ImagePath );
	if (!bSuccess)
	{
		UE_LOG ( LogTemp , Error , TEXT ( "Failed to load file: %s" ) , *ImagePath );
		return; // Exit if file loading fails
	}

	FString FileName = FPaths::GetCleanFilename ( ImagePath );
	FString Boundary = "---------------------------boundary_string";
	FString BeginBoundary = FString ( "--" ) + Boundary + TEXT ( "\r\n" );
	FString EndBoundary = FString ( "\r\n--" ) + Boundary + TEXT ( "--\r\n" );

	FString FileHeader = FString ( "Content-Disposition: form-data; name=\"file\"; filename=\"" ) + FileName + TEXT ( "\"\r\nContent-Type: image/png\r\n\r\n" );

	TArray<uint8> PostData;
	PostData.Append ( (uint8*)TCHAR_TO_ANSI ( *BeginBoundary ) , BeginBoundary.Len ( ) );
	PostData.Append ( (uint8*)TCHAR_TO_ANSI ( *FileHeader ) , FileHeader.Len ( ) );
	PostData.Append ( FileData );
	PostData.Append ( (uint8*)TCHAR_TO_ANSI ( *EndBoundary ) , EndBoundary.Len ( ) );

	// Add Authorization header
	FString AuthHeader = FString::Printf ( TEXT ( "Bearer %s" ) , *gi->loginInfo.token );
	ImageUploadRequest->SetHeader ( TEXT ( "Authorization" ) , AuthHeader );

	// Set multipart header
	FString ContentType = FString::Printf ( TEXT ( "multipart/form-data; boundary=%s" ) , *Boundary );
	ImageUploadRequest->SetHeader ( TEXT ( "Content-Type" ) , *ContentType );

	ImageUploadRequest->SetTimeout ( 180.f );
	// Set the content to send
	ImageUploadRequest->SetContent ( PostData );

	// Process the request
	ImageUploadRequest->ProcessRequest ( );
}

void AHttpActor_KMK::OnReqMultipartCapturedWithAI ( FHttpRequestPtr Request , FHttpResponsePtr Response , bool bConnectedSuccessfully)
{
	if (bConnectedSuccessfully && Response.IsValid ( ))
	{

		int32 ResponseCode = Response->GetResponseCode ( );
		UE_LOG(LogTemp, Error, TEXT("%d" ), Response->GetResponseCode());
		if (ResponseCode == 200) // 성공적으로 응답받은 경우
		{
// 			// 응답 바디를 가져와 Stage의 img에 설정합니다.
			ticketURL = Response->GetContentAsString ( );
			gi->widget->SetTicketButton ( );
			UE_LOG ( LogTemp , Warning , TEXT ( "Image URL set to: %s" ) , *ticketURL );

// 			// JSon응답 문자열 가져오기
// 			FString ResponseBody = Response->GetContentAsString ( );
// 
// 			// JSon 파싱
// 			TSharedPtr<FJsonObject> JsonObject;
// 			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create ( ResponseBody );
// 
// 			if (FJsonSerializer::Deserialize ( Reader , JsonObject ) && JsonObject.IsValid ( ))
// 			{
// 				FString FilePath;
// 				if (JsonObject->TryGetStringField ( TEXT ( "file_path" ) , FilePath ))
// 				{
// 					ticketURL = FilePath;
// 					UE_LOG ( LogTemp , Warning , TEXT ( "Image URL set to: %s" ) , *ticketURL );
// 
// 					// 응답 바디를 가져와 Stage의 img에 설정합니다.
// 					gi->widget->SetTicketButton();
// 				}
// 				else
// 				{
// 					UE_LOG ( LogTemp , Error , TEXT ( "Failed to find 'file_path' in JSON response" ) );
// 				}
// 			}
// 			else
// 			{
// 				UE_LOG ( LogTemp , Error , TEXT ( "Failed to parse JSON response: %s" ) , *ResponseBody );
// 			}
		}
		else
		{
			UE_LOG ( LogTemp , Warning , TEXT ( "Ticket Image URL failed" ) );
		}
	}
	else
	{
		UE_LOG ( LogTemp , Warning , TEXT ( "Image upload failed." ) );
	}
}

void AHttpActor_KMK::OnTextureCreated ( UTexture2D* texture )
{
	if (gi && gi->widget)
	{
		gi->widget->CreateTicketMaterial ( texture );
	}
}
#pragma endregion
#pragma region Translate

void AHttpActor_KMK::ReqTranslateChat ( const FString& json , class UAudienceServerComponent_KMK* server )
{
	// HTTP 모듈 생성
	FHttpModule& httpModule = FHttpModule::Get ( );
	TSharedRef<IHttpRequest> req = httpModule.CreateRequest ( );

	req->SetURL(TEXT("https://singular-swine-deeply.ngrok-free.app/translate") );
	req->SetVerb(TEXT("POST"));
	req->SetHeader(TEXT("content-type") , TEXT("application/json"));
	FString s = UJsonParseLib_KMK::MakeChatTranslate(json);
    req->SetContentAsString (s);

	req->OnProcessRequestComplete().BindUObject(this , &AHttpActor_KMK::OnReqTranslateChat, server);

	req->ProcessRequest();
}

void AHttpActor_KMK::OnReqTranslateChat ( FHttpRequestPtr Request , FHttpResponsePtr Response , bool bConnectedSuccessfully , class UAudienceServerComponent_KMK* server )
{
	if (bConnectedSuccessfully)
	{
		// 성공
		FString transLateString = UJsonParseLib_KMK::ParseChatTranslate ( Response->GetContentAsString ( ) );
		
		if (Response.IsValid() && Response->GetResponseCode() == 200)
		{
			server->ServerRPCChat(gi->GetMyInfo().userName, transLateString );
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to download image. Response code: %d"), Response->GetResponseCode());
		}
		
	}
	else
	{
		UE_LOG ( LogTemp , Warning , TEXT ( "Failed myStage" ) );
	}
}

void AHttpActor_KMK::ReqTranslateChat ( const FString& json , class AHSW_GameState_Auditorium* gs )
{
	// HTTP 모듈 생성
	FHttpModule& httpModule = FHttpModule::Get ( );
	TSharedRef<IHttpRequest> req = httpModule.CreateRequest ( );

	req->SetURL ( TEXT ( "https://singular-swine-deeply.ngrok-free.app/translate" ) );
	req->SetVerb ( TEXT ( "POST" ) );
	req->SetHeader ( TEXT ( "content-type" ) , TEXT ( "application/json" ) );
	FString s = UJsonParseLib_KMK::MakeChatTranslate ( json );
	req->SetContentAsString ( s );

	req->OnProcessRequestComplete ( ).BindUObject ( this , &AHttpActor_KMK::OnReqTranslateChat , gs );

	req->ProcessRequest ( );
}

void AHttpActor_KMK::OnReqTranslateChat ( FHttpRequestPtr Request , FHttpResponsePtr Response , bool bConnectedSuccessfully , class AHSW_GameState_Auditorium* gs )
{
	if (bConnectedSuccessfully)
	{
		// 성공
		FString transLateString = UJsonParseLib_KMK::ParseChatTranslate ( Response->GetContentAsString ( ) );

		if (Response.IsValid ( ) && Response->GetResponseCode ( ) == 200)
		{
			gs->ServerRPCChat (gi->GetMyInfo().userName, transLateString );
		}
		else
		{
			UE_LOG ( LogTemp , Error , TEXT ( "Failed to download image. Response code: %d" ) , Response->GetResponseCode ( ) );
		}

	}
	else
	{
		UE_LOG ( LogTemp , Warning , TEXT ( "Failed myStage" ) );
	}
}
#pragma endregion

#pragma region BE for CheckConcert
// 이미지 다운로드 코드 작성

void AHttpActor_KMK::DownloadImageConcert ( const FString& imageUrl, const FConcertInfo& concertInfo )
{
	FHttpModule* http = &FHttpModule::Get();
    TSharedRef<IHttpRequest> req = http->CreateRequest();
    req->SetURL(imageUrl);
    req->SetVerb("GET");
    req->OnProcessRequestComplete ( ).BindUObject ( this , &AHttpActor_KMK::OnImageDownConcertComplete , concertInfo );
    req->ProcessRequest ( );
}


void AHttpActor_KMK::OnImageDownConcertComplete ( FHttpRequestPtr Request , FHttpResponsePtr Response , bool bWasSuccessful , FConcertInfo concertInfo )
{
if (bWasSuccessful&& Response.IsValid())
	{
		if (Response->GetResponseCode() != 200)
        {
            UE_LOG ( LogTemp , Error , TEXT ( "Failed to get image. Response Code: %d" ) , Response->GetResponseCode ( ) );
            return;
        }
		FString ContentType = Response->GetContentType();
        UE_LOG ( LogTemp , Log , TEXT ( "Content-Type: %s" ) , *ContentType );

        if (!ContentType.Contains ( "image" ))
        {
            UE_LOG ( LogTemp , Error , TEXT ( "Unexpected Content-Type: %s" ) , *ContentType );
            return;
        }
        // 다운로드 받은 PNG 데이터를 파일로 저장
        const TArray<uint8>& ImageData = Response->GetContent();

		UTexture2D* texture = UJsonParseLib_KMK::MakeTexture(ImageData);
        if (texture)
        {
            UE_LOG(LogTemp, Log, TEXT("Image downloaded and texture created successfully!"));
            // 예: 다운로드한 텍스처를 위젯이나 다른 액터에 할당
			concertInfo.texture = texture;
			
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to create texture from downloaded image data."));
        }
		gi->allConcertInfoArray.Add(concertInfo);
    }
   else
   {
       if (Response.IsValid())
       {
           UE_LOG(LogTemp, Error, TEXT("Failed to download image. Response code: %d"), Response->GetResponseCode());
       }
       else
       {
           UE_LOG(LogTemp, Error, TEXT("Response is invalid"));
		   if(ticketData.Num() > 0) ReqTicket(ticketData );
       }
   }
}

void AHttpActor_KMK::ReqCheckMyConcert ( )
{
	if (!ShouldUseServer ( ))
	{
		return;
	}

	FHttpModule& httpModule = FHttpModule::Get ( );
	TSharedRef<IHttpRequest> req = httpModule.CreateRequest ( );

	FString AuthHeader = FString::Printf ( TEXT ( "Bearer %s" ) , *gi->loginInfo.token );
	req->SetHeader ( TEXT ( "Authorization" ) , AuthHeader );
	req->SetHeader ( TEXT ( "Content-Type" ) , TEXT ( "application/json" ) );

	FString url = TEXT ( "http://back.reward-factory.shop:8123/api/v1/concerts/user/" ) + FString::FormatAsNumber ( gi->GetMyInfo ( ).userId );
	req->SetURL ( url );
	req->SetVerb ( TEXT ( "GET" ) );
	req->OnProcessRequestComplete ( ).BindUObject ( this , &AHttpActor_KMK::OnResqCheckMyConcert );
	req->ProcessRequest ( );
}

void AHttpActor_KMK::OnResqCheckMyConcert ( FHttpRequestPtr Request , FHttpResponsePtr Response , bool bConnectedSuccessfully )
{
	if (!bConnectedSuccessfully || !Response.IsValid ( ))
	{
		return;
	}

	if (bConnectedSuccessfully)
	{
		// 성공
		FString respon = Response->GetContentAsString();
		if (Response.IsValid() && Response->GetResponseCode() == 200)
		{
			TArray<FConcertInfo> info = UJsonParseLib_KMK::ParsecAllConcert(respon);
			if (info.Num() > 0)
			{
				gi->SetConcertInfo(info, this);
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to download image. Response code: %d"), Response->GetResponseCode());
		}
		
	}
	else
	{
		UE_LOG ( LogTemp , Warning , TEXT ( "Failed myStage" ) );
	}
}

void AHttpActor_KMK::ReqCheckAllOpenConcert ( )
{
	if (!ShouldUseServer ( ))
	{
		if (gi)
		{
			gi->allConcertInfoArray.Empty ( );
		}
		return;
	}

	FHttpModule& httpModule = FHttpModule::Get ( );
	TSharedRef<IHttpRequest> req = httpModule.CreateRequest ( );

	FString AuthHeader = FString::Printf ( TEXT ( "Bearer %s" ) , *gi->loginInfo.token );
	req->SetHeader ( TEXT ( "Authorization" ) , AuthHeader );
	req->SetHeader ( TEXT ( "Content-Type" ) , TEXT ( "application/json" ) );
	req->SetURL ( TEXT ( "http://back.reward-factory.shop:8123/api/v1/concerts" ) );
	req->SetVerb ( TEXT ( "GET" ) );
	req->OnProcessRequestComplete ( ).BindUObject ( this , &AHttpActor_KMK::OnResCheckAllOpenConcert );
	req->ProcessRequest ( );
}

void AHttpActor_KMK::OnResCheckAllOpenConcert ( FHttpRequestPtr Request , FHttpResponsePtr Response , bool bConnectedSuccessfully )
{
	if (!bConnectedSuccessfully || !Response.IsValid ( ))
	{
		return;
	}

	if (bConnectedSuccessfully)
	{
		// 성공
		FString respon = Response->GetContentAsString();
		if (Response.IsValid() && Response->GetResponseCode() == 200)
		{
			allConcertInfoArray = UJsonParseLib_KMK::ParsecAllConcert(respon);

			// 사진 관련된 항목
			if (allConcertInfoArray.Num ( ) > 0)
			{
				// gi->allConcertInfoArray = allConcertInfoArray;
				for (auto& concert : allConcertInfoArray)
				{
					DownloadImageConcert(concert.img, concert);
					gi->allConcertInfoArray.Add(concert);
				}
				
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to download image. Response code: %d"), Response->GetResponseCode());
		}
		
	}
	else
	{
		UE_LOG ( LogTemp , Warning , TEXT ( "Failed myStage" ) );
	}
}

void AHttpActor_KMK::ReqCheckIdStage ( int32 num )
{
	if (!ShouldUseServer ( ))
	{
		if (sw)
		{
			sw->SetButtEnable ( true );
		}
		return;
	}

	FHttpModule& httpModule = FHttpModule::Get ( );
	TSharedRef<IHttpRequest> req = httpModule.CreateRequest ( );

	FString authHeader = FString::Printf ( TEXT ( "Bearer %s" ) , *gi->loginInfo.token );
	req->SetHeader ( TEXT ( "Authorization" ) , authHeader );
	req->SetHeader ( TEXT ( "Content-Type" ) , TEXT ( "application/json" ) );

	FString url = TEXT ( "http://back.reward-factory.shop:8123/api/v1/stages/" ) + FString::FormatAsNumber ( num );
	req->SetURL ( url );
	req->SetVerb ( TEXT ( "GET" ) );
	req->OnProcessRequestComplete ( ).BindUObject ( this , &AHttpActor_KMK::OnResCheckIdStage );
	req->ProcessRequest ( );
}

void AHttpActor_KMK::OnResCheckIdStage ( FHttpRequestPtr Request , FHttpResponsePtr Response , bool bConnectedSuccessfully )
{
	if (!bConnectedSuccessfully || !Response.IsValid ( ))
	{
		return;
	}

	if (bConnectedSuccessfully)
	{
		// 성공
		FString respon = Response->GetContentAsString();
		if (Response.IsValid() && Response->GetResponseCode() == 200)
		{
			FStageInfo info = UJsonParseLib_KMK::ParsecMyStageInfo(respon);
			gi->SetConcertStageInfo(info);
			sw->SetButtEnable(true);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to download image. Response code: %d"), Response->GetResponseCode());
		}
	}
	else
	{
		UE_LOG ( LogTemp , Warning , TEXT ( "Failed myStage" ) );
	}
}


#pragma endregion

#pragma region with BE for StageSettings

void AHttpActor_KMK::ReqMultipartCapturedURL ( FStageInfo& Stage , const FString& ImagePath )
{   
	UE_LOG ( LogTemp , Warning , TEXT ( "Image upload start to BE." ) );

	// Create an HTTP request for the multipart upload
	TSharedRef<IHttpRequest> ImageUploadRequest = FHttpModule::Get ( ).CreateRequest ( );
	ImageUploadRequest->OnProcessRequestComplete ( ).BindUObject ( this , 
&AHttpActor_KMK::OnReqMultipartCapturedURL , &Stage );

	// Set the URL and verb for the image upload request
	ImageUploadRequest->SetURL ( TEXT ( "http://back.reward-factory.shop:8123/api/v1/files/upload" ) );
	ImageUploadRequest->SetVerb ( TEXT ( "POST" ) );


	UE_LOG ( LogTemp , Error , TEXT ( "Image to BE Path: %s" ) , *ImagePath );

	// Prepare multipart form data for image upload
	TArray<uint8> FileData;
	bool bSuccess = FFileHelper::LoadFileToArray ( FileData , *ImagePath );
	if (!bSuccess)
	{
		UE_LOG ( LogTemp , Error , TEXT ( "Failed to load file: %s" ) , *ImagePath );
		return; // Exit if file loading fails
	}

	FString FileName = FPaths::GetCleanFilename ( ImagePath );
	FString Boundary = "---------------------------boundary_string";
	FString BeginBoundary = FString ( "--" ) + Boundary + TEXT ( "\r\n" );
	FString EndBoundary = FString ( "\r\n--" ) + Boundary + TEXT ( "--\r\n" );

	FString FileHeader = FString ( "Content-Disposition: form-data; name=\"file\"; filename=\"" ) + FileName + TEXT ( "\"\r\nContent-Type: image/png\r\n\r\n" );

	TArray<uint8> PostData;
	PostData.Append ( (uint8*)TCHAR_TO_ANSI ( *BeginBoundary ) , BeginBoundary.Len ( ) );
	PostData.Append ( (uint8*)TCHAR_TO_ANSI ( *FileHeader ) , FileHeader.Len ( ) );
	PostData.Append ( FileData );
	PostData.Append ( (uint8*)TCHAR_TO_ANSI ( *EndBoundary ) , EndBoundary.Len ( ) );

	// Add Authorization header
	FString AuthHeader = FString::Printf ( TEXT ( "Bearer %s" ) , *gi->loginInfo.token );
	ImageUploadRequest->SetHeader ( TEXT ( "Authorization" ) , AuthHeader );

	// Set multipart header
	FString ContentType = FString::Printf ( TEXT ( "multipart/form-data; boundary=%s" ) , *Boundary );
	ImageUploadRequest->SetHeader ( TEXT ( "Content-Type" ) , *ContentType );

	ImageUploadRequest->SetTimeout ( 180.f );
	// Set the content to send
	ImageUploadRequest->SetContent ( PostData );

	// Process the request
	ImageUploadRequest->ProcessRequest ( );
}

void AHttpActor_KMK::OnReqMultipartCapturedURL ( FHttpRequestPtr Request , FHttpResponsePtr Response , bool bConnectedSuccessfully , FStageInfo* Stage )
{
	if (bConnectedSuccessfully && Response.IsValid ( ))
	{
		int32 ResponseCode = Response->GetResponseCode ( );
		if (ResponseCode == 200) // 성공적으로 응답받은 경우
		{
			// 응답 바디를 가져와 Stage의 img에 설정합니다.
			FString ImageURL = Response->GetContentAsString ( );
			Stage->img = ImageURL;

			ReqStageInfo(*Stage);
			UE_LOG ( LogTemp , Warning , TEXT ( "Image URL set to: %s" ) , *Stage->img );
		}
	}
	else
	{
		UE_LOG ( LogTemp , Warning , TEXT ( "Image upload failed." ) );
	}
}

void AHttpActor_KMK::ReqStageInfo ( const FStageInfo& Stage)
{

	// JSON으로 변환
	FString JsonString;
	if (!FJsonObjectConverter::UStructToJsonObjectString ( Stage , JsonString ))
	{
		UE_LOG ( LogTemp , Error , TEXT ( "Failed to convert Stage to JSON." ) );
		return;
	}
	UE_LOG ( LogTemp , Warning , TEXT ( "Sending Request: %s" ) , *JsonString );
	
	FJsonObjectConverter::UStructToJsonObjectString (Stage, JsonString);

	TSharedRef<IHttpRequest> Request = FHttpModule::Get ( ).CreateRequest ( );
	Request->OnProcessRequestComplete ( ).BindUObject ( this , &AHttpActor_KMK::OnReqStageInfo );
	Request->SetURL ( TEXT ( "http://back.reward-factory.shop:8123/api/v1/stages" ) );
	Request->SetVerb ( TEXT ( "POST" ) );

	// Add Authorization header
	FString AuthHeader = FString::Printf ( TEXT ( "Bearer %s" ) , *gi->loginInfo.token );
	Request->SetHeader ( TEXT ( "Authorization" ) , AuthHeader );

	// JSON 데이터 전송
	Request->SetHeader ( TEXT ( "Content-Type" ) , TEXT ( "application/json" ) );
	Request->SetContentAsString ( JsonString ); // JSON 문자열 설정

	// 요청 전송
	Request->ProcessRequest ( );
}

void AHttpActor_KMK::OnReqStageInfo ( FHttpRequestPtr Request , FHttpResponsePtr Response , bool bConnectedSuccessfully )
{

	if (Response->GetResponseCode ( ) == 201)
	{
		UE_LOG ( LogTemp , Error , TEXT ( "Uploaded" )); 
		if (SelectManager && SelectManager->MapSelectWidget)
		{
			SelectManager->MapSelectWidget->MakeSetUpFinishBorder_1Visible();
		}
	}
	else
	{
		UE_LOG ( LogTemp , Error , TEXT ( "failed" ) );
	}

}

#pragma endregion

void AHttpActor_KMK::ReqCollcetionConcert ( int32 concertIndex )
{
	// HTTP 모듈 생성
	FHttpModule& httpModule = FHttpModule::Get ( );
	TSharedRef<IHttpRequest> req = httpModule.CreateRequest ( );
	// 요청할 정보를 설정
	req->SetContentAsString(FString::FromInt(concertIndex));
	FString authHeader = FString::Printf ( TEXT ( "Bearer %s" ) , *gi->loginInfo.token );
    req->SetHeader(TEXT("Authorization"), *( authHeader ));
	req->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	FString url = TEXT("http://back.reward-factory.shop:8123/api/user-activity/collect-concert");
	req->SetURL(url );
	req->SetVerb ( TEXT ( "POST" ) );

	req->ProcessRequest ( );
	// 응답받을 함수를 연결
	req->OnProcessRequestComplete ( ).BindUObject ( this , &AHttpActor_KMK::OnReqMultiCollectionConcert );
}

void AHttpActor_KMK::OnReqMultiCollectionConcert ( FHttpRequestPtr Request , FHttpResponsePtr Response , bool bConnectedSuccessfully )
{
	if (bConnectedSuccessfully)
	{
		// 성공
		FString respon = Response->GetContentAsString();
		if (Response->GetResponseCode() == 200)
		{
			UE_LOG ( LogTemp , Warning , TEXT ( "Collcet concert" ) );
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to download image. Response code: %d"), Response->GetResponseCode());
		}
	}
	else
	{
		UE_LOG ( LogTemp , Warning , TEXT ( "Failed Collcet concert" ) );
	}
}

#pragma region BE Music
void AHttpActor_KMK::ReqMusic ( int64 ConcertId )
{
	if (!ShouldUseServer ( ))
	{
		return;
	}
	UE_LOG ( LogTemp , Warning , TEXT ( "ReqMusic Start" ) );

	// API 요청 URL 생성
	FString ApiUrl = FString::Printf ( TEXT ( "http://back.reward-factory.shop:8123/api/v1/songs/concert/%d" ) , ConcertId );

	UE_LOG ( LogTemp , Warning , TEXT ( "Music API URL: *s" ), *ApiUrl );

	// HTTP 요청 생성
	TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get ( ).CreateRequest ( );
	HttpRequest->SetURL ( ApiUrl );
	HttpRequest->SetVerb ( TEXT ( "GET" ) );

	FString AuthHeader = FString::Printf ( TEXT ( "Bearer %s" ) , *loginInfo.token );
	HttpRequest->SetHeader ( TEXT ( "Authorization" ) , AuthHeader );

	HttpRequest->OnProcessRequestComplete ( ).BindUObject ( this , &AHttpActor_KMK::OnReqMusic, ConcertId );
	HttpRequest->ProcessRequest ( );
}

void AHttpActor_KMK::OnReqMusic ( FHttpRequestPtr Request , FHttpResponsePtr Response , bool bWasSuccessful, int64 ConcertId )
{
	UE_LOG ( LogTemp , Warning , TEXT ( "OnReqMusic Start" ) );
	UE_LOG ( LogTemp , Warning , TEXT ( "Response Content: %s" ) , *Response->GetContentAsString ( ) )

	if (!bWasSuccessful || !Response.IsValid ( ) || Response->GetResponseCode ( ) != 200)
	{
		UE_LOG ( LogTemp , Error , TEXT ( "HTTP 요청 실패 또는 잘못된 응답" ) );
		return;
	}


	// JSON 파싱
	TSharedPtr<FJsonValue> JsonParsed;
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create ( Response->GetContentAsString ( ) );
	if (!FJsonSerializer::Deserialize ( JsonReader , JsonParsed ) || !JsonParsed.IsValid ( ))
	{
		UE_LOG ( LogTemp , Error , TEXT ( "JSON 파싱 실패" ) );
		return;
	}

	UE_LOG ( LogTemp , Warning , TEXT ( "JsonParsed Valid: %s" ) , JsonParsed.IsValid ( ) ? TEXT ( "true" ) : TEXT ( "false" ) );


	const TArray<TSharedPtr<FJsonValue>>* Songs;

	if (!JsonParsed->TryGetArray ( Songs ))
	{
		UE_LOG ( LogTemp , Error , TEXT ( "JSON 데이터가 배열이 아닙니다." ) );
		return;
	}

	if (Songs && Songs->Num ( ) > 0)
	{
		UE_LOG ( LogTemp , Warning , TEXT ( "Songs Array Size: %d" ) , Songs->Num ( ) );
	}
	else
	{
		UE_LOG ( LogTemp , Error , TEXT ( "Songs 배열이 비어있음." ) );
		return;
	}

	// JSON 데이터 순회
	for (const TSharedPtr<FJsonValue>& Song : *Songs)
	{
		UE_LOG ( LogTemp , Warning , TEXT ( "Json for loop start" ));

		const TSharedPtr<FJsonObject>* SongObject;
		if (!Song->TryGetObject ( SongObject ))
		{
			UE_LOG ( LogTemp , Warning , TEXT ( "Json 배열의 요소가 객체가 아님." ) );
			continue;
		}

		int32 SongId = ( *SongObject )->GetIntegerField ( TEXT ( "id" ) );
		FString Title = ( *SongObject )->GetStringField ( TEXT ( "title" ) );
		FString Url = ( *SongObject )->GetStringField ( TEXT ( "url" ) );

		UE_LOG ( LogTemp , Warning , TEXT ( "This Song: %s"), *Title );

		// HTTP 요청으로 WAV 다운로드
		TSharedRef<IHttpRequest> WavRequest = FHttpModule::Get ( ).CreateRequest ( );
		WavRequest->SetURL ( Url );
		WavRequest->SetVerb ( TEXT ( "GET" ) );
		WavRequest->OnProcessRequestComplete ( ).BindLambda ( [this , ConcertId , SongId , Title]( FHttpRequestPtr WavRequest , FHttpResponsePtr WavResponse , bool bWasWavSuccessful ) {
			if (bWasWavSuccessful && WavResponse.IsValid ( ) && WavResponse->GetResponseCode ( ) == 200)
			{
				FString FileName = FString::Printf ( TEXT ( "%d_%d_%s.wav" ) , ConcertId , SongId , *Title );
				UE_LOG ( LogTemp , Warning , TEXT ( "Song FileName: %s"), *FileName );
				SaveWavToFile ( FileName , WavResponse->GetContent ( ) );
			}
			else
			{
				UE_LOG ( LogTemp , Error , TEXT ( "WAV 파일 다운로드 실패: %s" ) , *WavRequest->GetURL ( ) );
			}
		} );
		WavRequest->ProcessRequest ( );
	}
}

void AHttpActor_KMK::SaveWavToFile ( const FString& FileName , const TArray<uint8>& Data )
{
	UE_LOG ( LogTemp , Warning , TEXT ( "SaveWavToFile Start") );

	FString SavePath = FPaths::ProjectSavedDir ( ) / TEXT ( "Music" );
	IFileManager::Get ( ).MakeDirectory ( *SavePath , true );
	FString FullPath = SavePath / FileName;

	if (FFileHelper::SaveArrayToFile ( Data , *FullPath ))
	{
		UE_LOG ( LogTemp , Log , TEXT ( "WAV 파일 저장 완료: %s" ) , *FullPath );
	}
	else
	{
		UE_LOG ( LogTemp , Error , TEXT ( "WAV 파일 저장 실패: %s" ) , *FullPath );
	}
}
#pragma endregion

bool AHttpActor_KMK::ShouldUseServer ( ) const
{
	return gi && !gi->IsOfflineMode ( );
}

void AHttpActor_KMK::HandleOfflineLoginSuccess ( )
{
	if (!gi)
	{
		return;
	}

	gi->SetupOfflineDefaults ( );
	loginInfo = gi->GetMyInfo ( );
	gi->SetMyInfo ( loginInfo );
	gi->SetMyProfile ( );
	gi->SwitchWidget ( 1 );
}

void AHttpActor_KMK::HandleOfflineTicketCreated ( )
{
	if (sw)
	{
		sw->bCreateTicket = true;
		sw->SetLoadImage ( );
		sw->SetTicketButton ( );
	}
}