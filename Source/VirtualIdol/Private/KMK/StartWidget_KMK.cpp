// Fill out your copyright notice in the Description page of Project Settings.


#include "KMK/StartWidget_KMK.h"
#include "Components/WidgetSwitcher.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/Button.h"
#include "Components/EditableText.h"
#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "Engine/TimerHandle.h"
#include "KMK/RoomWidget_KMK.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Engine/Texture2D.h"
#include "KMK/VirtualGameInstance_KMK.h"
#include "Components/TextBlock.h"
#include "Components/MultiLineEditableText.h"
#include "Components/ScrollBox.h"
#include "KMK/HttpActor_KMK.h"
#include "JJH/JJH_SelectManager.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "IDesktopPlatform.h"
#include "DesktopPlatformModule.h"
#include "Kismet/KismetSystemLibrary.h"

void UStartWidget_KMK::NativeConstruct ( )
{	
	Super::NativeConstruct ( );

	UWorld* World = GetWorld ( );
	if (!World)
	{
		return;
	}

	gi = Cast<UVirtualGameInstance_KMK> ( World->GetGameInstance ( ) );
	httpActor = Cast<AHttpActor_KMK> ( UGameplayStatics::GetActorOfClass ( World , httpFact ) );
	selectManager = Cast<AJJH_SelectManager> ( UGameplayStatics::GetActorOfClass ( World , selectFact ) );

	if (loadMatFact)
	{
		loadMatInst = UMaterialInstanceDynamic::Create ( loadMatFact , this );
	}

	if (httpActor)
	{
		httpActor->sw = this;
	}

	if (Image_Load && loadMatInst)
	{
		Image_Load->SetBrushFromMaterial ( loadMatInst );
		loadMatInst->SetScalarParameterValue ( TEXT ( "Gauge" ) , 0 );
	}

	if (gi && gi->bLogin)
	{
		if (StartSwitcher)
		{
			StartSwitcher->SetActiveWidgetIndex ( 1 );
		}

		if (selectManager)
		{
			selectManager->DeleteStage ( );
		}

		ChangeMyProfile ( );
	}
	else if (gi && gi->IsOfflineMode ( ))
	{
		EnterOfflineLogin ( );
	}

// ButtonBinding 구역, 확인할 필요 없음X => 맨 마지막에 SetButtEnable : 공연시작 버튼 활성화 여부를 위한 함수임
#pragma region Button Binding

#pragma region LoginPanel
	if (Butt_Login && Butt_FailLogin && FailLoginPanel)
	{
		Butt_Login->OnClicked.AddDynamic(this, &UStartWidget_KMK::OnMyLogin);
		Butt_FailLogin->OnClicked.AddDynamic(this, &UStartWidget_KMK::OnFailLogin);
		FailLoginPanel->SetVisibility(ESlateVisibility::Hidden);
	}
#pragma endregion

#pragma region Homepage
	if (Button_Homepage_1 && Button_Homepage_2 && Butt_SignIn)
	{
		Button_Homepage_1->OnClicked.AddDynamic ( this , &UStartWidget_KMK::OnHomepage );
		Button_Homepage_2->OnClicked.AddDynamic ( this , &UStartWidget_KMK::OnHomepage );
		Butt_SignIn->OnClicked.AddDynamic ( this , &UStartWidget_KMK::OnSignInPage );
	}
#pragma endregion

#pragma region FourButtPanel
	if (Butt_CreateStage)
	{
		Butt_CreateStage->OnClicked.AddDynamic ( this , &UStartWidget_KMK::CreateStagePanel );
	}
	if (Butt_ReadyConcert)
	{
		Butt_ReadyConcert->OnClicked.AddDynamic ( this , &UStartWidget_KMK::SettingStagePanel );
	}
	if (Butt_StartConcert)
	{
		Butt_StartConcert->OnClicked.AddDynamic ( this , &UStartWidget_KMK::StartConcertPanel );
		Butt_StartConcert->bIsEnabled = false;
	}
	if (Butt_ComeInStage)
	{
		Butt_ComeInStage->OnClicked.AddDynamic ( this , &UStartWidget_KMK::ComeInStagePanel );
	}
#pragma endregion
#pragma region Select Stage
	if (Butt_UserStage && Butt_MyStage)
	{
		Butt_UserStage->OnClicked.AddDynamic ( this , &UStartWidget_KMK::PressUserStageButt );
		Butt_MyStage->OnClicked.AddDynamic ( this , &UStartWidget_KMK::PressMyStageButt );
	}
#pragma endregion

#pragma region  Setting StagePanel

#pragma region Create Ticket
	if (Butt_CreateTicket && Butt_CreateTicket1)
	{
		Butt_CreateTicket->OnClicked.AddDynamic ( this , &UStartWidget_KMK::PressCreateTicket );
		Butt_CreateTicket1->OnClicked.AddDynamic ( this , &UStartWidget_KMK::PressCreateTicket );
		Butt_CreateTicket->SetIsEnabled(false);
		Butt_CreateTicket1->SetIsEnabled(false);		

	}
#pragma endregion
#pragma region Set Particle
	if (Butt_AppearEffect && Butt_FeverEffect)
	{
		Butt_AppearEffect->OnClicked.AddDynamic ( this , &UStartWidget_KMK::PressAppearEffect );
		Butt_FeverEffect->OnClicked.AddDynamic ( this , &UStartWidget_KMK::PressFeverEffect );
	}
	if (Butt_Select &&Butt_Select1 && Butt_Upload)
	{
		Butt_Select->OnClicked.AddDynamic ( this , &UStartWidget_KMK::PressSelectButt );
		Butt_Select1->OnClicked.AddDynamic ( this , &UStartWidget_KMK::PressSelect1Butt );
		Butt_Upload->OnClicked.AddDynamic ( this , &UStartWidget_KMK::PressUpload );
    }	
	if (SetDayPanel && SetTicketPanel && StageChargePanel)
	{
		SetTitleText(TEXT("공연 설정" ) );

		SetPanelVisible ( SetDayPanel , SetTicketPanel  , StageChargePanel );
	}
#pragma endregion
#pragma region Pay Moneny
	if (Butt_PayMoney)
	{
		Butt_PayMoney->OnClicked.AddDynamic ( this , &UStartWidget_KMK::PressMoneyPay );
	}
	if (Butt_Next)
	{
		Butt_Next->OnClicked.AddDynamic ( this , &UStartWidget_KMK::PressNextButt );
	}
	if (StagePayPanel && Text_Pay && Butt_PayMoney && PayPopUpPanel)
	{
		StagePayPanel->SetVisibility(ESlateVisibility::Hidden);
		Text_Pay->SetVisibility(ESlateVisibility::Hidden);
		Butt_PayMoney->SetVisibility(ESlateVisibility::Hidden);
		PayPopUpPanel->SetVisibility(ESlateVisibility::Hidden);
	}
	if (Butt_Okay)
	{
		Butt_Okay->OnClicked.AddDynamic ( this , &UStartWidget_KMK::PressOkayButt );
	}
#pragma endregion

#pragma endregion

#pragma region Entry
	if (Butt_Yes && Butt_VipEntry && VIPPopUpPanel)
	{
		Butt_VipEntry->OnClicked.AddDynamic ( this , &UStartWidget_KMK::PressVipEntry );
		Butt_Yes->OnClicked.AddDynamic ( this , &UStartWidget_KMK::PressYesButt );
		VIPPopUpPanel->SetVisibility(ESlateVisibility::Hidden);
	}
	if (Butt_NormalEntry && Butt_No)
	{
		Butt_NormalEntry->OnClicked.AddDynamic ( this , &UStartWidget_KMK::PressNormalEntry );
		Butt_No->OnClicked.AddDynamic ( this , &UStartWidget_KMK::PressNoButt );
	}	
	if (Butt_Back1 && Butt_Back2 && Butt_Back && Butt_Back3)
	{
		Butt_Back->OnClicked.AddDynamic ( this , &UStartWidget_KMK::GoBack );
		Butt_Back1->OnClicked.AddDynamic ( this , &UStartWidget_KMK::GoBack );
		Butt_Back2->OnClicked.AddDynamic ( this , &UStartWidget_KMK::GoBack );
		Butt_Back3->OnClicked.AddDynamic ( this , &UStartWidget_KMK::GoBack );
	}
#pragma endregion
	if (gi)
	{
		gi->OnSearchSignatureCompleteDelegate.AddDynamic ( this , &UStartWidget_KMK::CreateRoomWidget );
		gi->SetStartWidget ( this );
	}

	SetButtEnable ( true );
}



void UStartWidget_KMK::NativeTick ( const FGeometry& MyGeometry , float InDeltaTime )
{
	Super::NativeTick(MyGeometry,InDeltaTime);
	// 콘서트 예약시, 수용인원 설정 부분임
	// 수용인원이 작성될때마다 실시간으로  Text_Price의 값이 변경되어야 하기에 tick에서 사용함
	if (StageScalePanel->Visibility == ESlateVisibility::SelfHitTestInvisible)
	{
		// 수용 인원을 작성하는 경우
		if (!EditText_ScaleNum->GetText ( ).IsEmpty ( ))
		{
			// 수용 인원을 작성하는 text를 가져와 string값으로 변경하고
			FString s = EditText_ScaleNum->GetText().ToString();
			// atoi을 사용하여 string값을 변경함 : a = 수용인원 * 우리가 설정한 가격(concertPrice)
			int a = FCString::Atoi(*s) * concertPrice;

			a += 100000; //기본금
			// FString을 int32로 변환

			// 시간 차이 계산
			int32 timeDifference = FCString::Atoi ( *EditText_H->GetText ( ).ToString ( ) );
			a += timeDifference * 100000; //시간당 금액
			// 최종 가격을 작성
			Text_Price->SetText(FText::AsNumber(a));
		}
	}
	// ai 티켓이 생성되고 있는 경우 : ai서버와 통신 중인 경우
	// 티켓이 생성 완료 되기 전까지 로딩바가 나오게 만듦
	if (!bCreateTicket)
	{
		// matNum : 임시의 float값, 초반에 만들어놨던 dynamic material의 값을 변경하기 위한 값
		matNum += 0.005f;
		ChangeLoadMat( matNum);
	}

}

#pragma region BackFunction
// 뒤로가기 버튼에 바인딩된 함수
void UStartWidget_KMK::GoBack ( )
{
	// 만들어 놓은 공연장 actor를 삭제
	selectManager->DeleteStage ( );
	// 4개의 선택지로 돌아옴
    StartSwitcher->SetActiveWidgetIndex (1);
	// SB_FindStage(스크롤박스) 밑에 생성한 위잿들을 제거하는 함수
	ClearSB( );
	ClearAllText();
	SetPanelVisible ( SetDayPanel , SetTicketPanel , StageChargePanel );
	// 판넬들 초기화
	StageChargePanel->SetVisibility(ESlateVisibility::Hidden );
	StageScalePanel->SetVisibility(ESlateVisibility::Visible );
	StagePayPanel->SetVisibility(ESlateVisibility::Hidden );
	PayPopUpPanel->SetVisibility(ESlateVisibility::Hidden);
	
	selectNum = 0;
}
#pragma endregion


#pragma region LoginPanel

void UStartWidget_KMK::OnMyLogin ( )
{
	const FString Id = EditText_ID ? EditText_ID->GetText ( ).ToString ( ) : TEXT ( "" );
	const FString Pw = EditText_PW ? EditText_PW->GetText ( ).ToString ( ) : TEXT ( "" );

	if (IsOfflineMode ( ))
	{
		EnterOfflineLogin ( );
		return;
	}

	if (!Id.IsEmpty ( ) && !Pw.IsEmpty ( ))
	{
		if (httpActor)
		{
			httpActor->ReqLogin ( Id , Pw );
			httpActor->ReqCheckAllOpenConcert ( );
		}
	}
}

void UStartWidget_KMK::OnFailLogin ( )
{
	FailLoginPanel->SetVisibility(ESlateVisibility::Hidden);
}

void UStartWidget_KMK::OnHomepage ( )
{
	UKismetSystemLibrary::LaunchURL ( "http://tripleyoung.synology.me:3334" );
}

void UStartWidget_KMK::OnSignInPage ( )
{
	UKismetSystemLibrary::LaunchURL ( "http://tripleyoung.synology.me:3334/ko/signin" );
}

#pragma endregion

#pragma region FourButtPanel
void UStartWidget_KMK::ChangeMyProfile ( )
{
	FString UserName = TEXT ( "Guest" );
	int32 Cash = 0;

	if (gi)
	{
		Cash = gi->myCash;

		FLoginInfo Info = gi->GetMyInfo ( );
		if (!Info.userName.IsEmpty ( ))
		{
			UserName = Info.userName;
		}

		if (Image_Profile && Info.texture)
		{
			Image_Profile->SetBrushFromTexture ( Info.texture );
		}
	}

	if (Text_MyCash)
	{
		Text_MyCash->SetText ( FText::AsNumber ( Cash ) );
	}

	if (Text_MyNick)
	{
		Text_MyNick->SetText ( FText::FromString ( UserName ) );
	}
}

void UStartWidget_KMK::SetButtEnable ( bool bEnable /*= false*/ )
{
	// 공연 시작 버튼 활성화 여부 => 서버에서 데이터 값 확인 후에 활성화 되게 만듦
	Butt_StartConcert->SetIsEnabled(bEnable);
	isButtEanble = bEnable;
}

// 무대 꾸미기 레벨로 이동
void UStartWidget_KMK::CreateStagePanel ( )
{
	UGameplayStatics::OpenLevel(GetWorld( ) , TEXT("JJH_SetupMap" ));
}
// 공연 일정 잡는 판넬로 변경
void UStartWidget_KMK::SettingStagePanel ( )
{
	if (StartSwitcher)
	{
		StartSwitcher->SetActiveWidgetIndex ( 3 );
	}

	if (httpActor && !IsOfflineMode ( ))
	{
		httpActor->ReqCheckStage ( this );
	}
	else
	{
		ClearSB ( );
	}

	if (Butt_UserStage) Butt_UserStage->SetVisibility ( ESlateVisibility::Visible );
	if (Butt_MyStage)   Butt_MyStage->SetVisibility ( ESlateVisibility::Visible );
	if (Butt_Star)      Butt_Star->SetVisibility ( ESlateVisibility::Visible );
}

// 공연 시작 : 세션 생성
void UStartWidget_KMK::StartConcertPanel ( )
{
	if (!gi)
	{
		return;
	}

	gi->playerMeshNum = -1;

	if (gi->concerInfo.name.IsEmpty ( ))
	{
		gi->concerInfo = concertInfo;
	}

	if (gi->concerInfo.name.IsEmpty ( ))
	{
		gi->concerInfo.name = TEXT ( "OfflineConcert" );
	}

	if (gi->concerInfo.peopleScale <= 0)
	{
		gi->concerInfo.peopleScale = 1;
	}

	UGameplayStatics::OpenLevel ( GetWorld ( ) , FName ( TEXT ( "BetaMain" ) ) );
	//gi->CreateMySession ( gi->concerInfo.name , gi->concerInfo.peopleScale );
}

void UStartWidget_KMK::ComeInStagePanel ( )
{	
	
	Butt_UserStage->SetVisibility(ESlateVisibility::Hidden);
	Butt_MyStage->SetVisibility(ESlateVisibility::Hidden);
	Butt_Star->SetVisibility(ESlateVisibility::Hidden);

	FindRoom();

}

#pragma endregion

#pragma region Select Stage

void UStartWidget_KMK::PressUserStageButt ( )
{
	ClearSB ( );

	if (httpActor && !IsOfflineMode ( ))
	{
		httpActor->ReqCheckStage ( this );
	}
}

void UStartWidget_KMK::PressMyStageButt ( )
{
	ClearSB ( );

	if (httpActor && !IsOfflineMode ( ))
	{
		httpActor->ReqCheckMyStage ( this );
	}
}

void UStartWidget_KMK::CreateStageWidget (const struct FStageInfo& stageInfo, UTexture2D* image )
{
	// 이펙트를 고르기 위한
	auto* wid = Cast<URoomWidget_KMK> ( CreateWidget ( GetWorld ( ) , roomWidgetFact ) );
	if (wid)
	{
		wid->SetStageText ( stageInfo, image );
		gi->sm = selectManager;
		SB_FindStage->AddChild(wid);
	}
}

void UStartWidget_KMK::ClearSB ( )
{
	// child가 있다면
	if (SB_FindStage->GetChildrenCount ( ) > 0)
	{
		// 삭제함
		SB_FindStage->ClearChildren();
	}
}

#pragma endregion


#pragma region Setting StagePanel
// 콘서트 설정 부분
void UStartWidget_KMK::PressSelectButt ( )
{
	// 처음 next 버튼을 누르면
	if (selectNum == 0)
	{
		// 콘서트 설정을 완료 하지 않은 경우, 팝업에 밑에 내용이 뜨게 만듦
		Text_Effect1->SetText(FText::FromString(TEXT("공연 설정을 완료해주세요")));
		bool bNext = BEditTextEmpty ( );
		if(!bNext) 
		{
			return;
		}
		// 콘서트 설정이 완료 되면 티켓 예약으로 변경됨
		SetTitleText ( TEXT ( "티켓" ) );
		// 필요없는 panel을 hidden처리 하고 필요한 panel을 visible처리함 : 맨처음에 들어가는 panel이 visible됨
		SetPanelVisible( SetTicketPanel , SetDayPanel, StageChargePanel );
		// int 값 올리기
		selectNum++;
	}
	// 다음 next 버튼을 누르면
	else if (selectNum == 1)
	{
		// 콘서트 설정을 완료 하지 않은 경우, 팝업에 밑에 내용이 뜨게 만듦
		Text_Effect1->SetText(FText::FromString(TEXT("VIP 가격 설정을 완료해주세요")));
		if(EditText_Price->GetText().IsEmpty()) 
		{
			EffectPopUp1->SetVisibility(ESlateVisibility::Visible);
			UGameplayStatics::PlaySound2D ( GetWorld ( ) , PopUpSFV );
			return;
		}
		// conecrtInfo라는 구조체 내에 ticketPrice을 int값으로 저장함
		concertInfo.ticketPrice = FCString::Atoi(*EditText_Price->GetText().ToString());
		// 다음 창의 text값을 이펙트 설정으로 변경함
		SetTitleText ( TEXT ( "이펙트 설정" ) );
		// 필요없는 panel을 hidden처리 하고 필요한 panel을 visible처리함
		SetPanelVisible ( StageChargePanel , SetTicketPanel , SetDayPanel );
		selectNum++;
	}
	else
	{
		// 마지막 설정 창으로 감
		SetPanelVisible ( StageChargePanel , SetDayPanel , SetTicketPanel  );
		// 설정했던 text값들을 초기화함
		ClearAllText( );
		// 최종 결제창이 뜨게 만듦
		StageChargePanel->SetVisibility(ESlateVisibility::Visible);
		selectNum = 0;
	}

}


void UStartWidget_KMK::SetImageProfile ( class UTexture2D* texture )
{
	Image_Profile->SetBrushFromTexture(texture);
}

void UStartWidget_KMK::ChangeLoadMat ( float num )
{
	// matNum이 1이 아니라면 들어오는 매개변수에 따라 loadMatInst를 변경해줌
	if(matNum < 1)loadMatInst->SetScalarParameterValue ( TEXT ( "Gauge" ) , num );
	// 아니라면 초기 셋팅해줌
	else matNum = 0;
}

void UStartWidget_KMK::SetTitleText ( const FString& title )
{
	// 타이틀 변경
	if (Text_Title)
	{
		Text_Title->SetText ( FText::FromString ( title ) );
	}
}

bool UStartWidget_KMK::BEditTextEmpty ( )
{
	// 모든 텍스트가 채워진 경우에만 if문 밑에 있는 것들이 사용됨 : 최종 결과창에 있는 TEXT값들 변경
	if (!EditText_StageName->GetText ( ).IsEmpty ( ) 
	&& !EditText_Year->GetText ( ).IsEmpty ( ) && !EditText_Mon->GetText ( ).IsEmpty ( ) && !EditText_Day->GetText ( ).IsEmpty ( )
	&& !EditText_SHour->GetText ( ).IsEmpty ( ) && !EditText_SMin->GetText ( ).IsEmpty ( )
	&& !EditText_H->GetText ( ).IsEmpty ( ) && !EditText_M->GetText ( ).IsEmpty ( ))
	{
		// 1. Concert 이름 설정
		Text_FinalName->SetText(EditText_StageName->GetText());
		Text_FinalStageName->SetText(EditText_StageName->GetText());
		// concertInfo라는 구조체에 정보값 입력
		concertInfo.name = EditText_StageName->GetText().ToString();

		// 2. 콘서트 날짜 설정
		// int값인지 확인
		bool bEditDigit 
            = EditTextDigit ( EditText_Year->GetText ( ).ToString ( ) ) && EditTextDigit ( EditText_Mon->GetText ( ).ToString ( ) )
            && EditTextDigit ( EditText_Day->GetText ( ).ToString ( ) ) && EditTextDigit ( EditText_SHour->GetText ( ).ToString ( ) )
            && EditTextDigit ( EditText_SMin->GetText ( ).ToString ( ) ) && EditTextDigit ( EditText_H->GetText ( ).ToString ( ) )
            && EditTextDigit ( EditText_M->GetText ( ).ToString ( ) );
		// int가 아닌경우 반환
		if(!bEditDigit) return  false;
		// int인 경우, concertInfo에 데이터 값 설정
		Text_FinalYear->SetText(EditText_Year->GetText());
		Text_FinalMon->SetText(EditText_Mon->GetText());
		Text_FinalDay->SetText(EditText_Day->GetText());

		FString year = TEXT("20") + EditText_Year->GetText ( ).ToString ( );
		FString mon = ChangeString(EditText_Mon->GetText().ToString());
		FString day = ChangeString(EditText_Day->GetText().ToString());
		
		// conecrtDate 설정함
		concertInfo.concertDate = year + TEXT("-") + mon + TEXT("-") + day;

		// 3. 콘서트 시작 시간 설정
		Text_StartHour->SetText(EditText_SHour->GetText());
		Text_StartMin->SetText(EditText_SMin->GetText());
		FString sHour =  ChangeString(EditText_SHour->GetText().ToString());
		FString sMin = ChangeString(EditText_SMin->GetText().ToString());
		// 구조체에 콘서트 시작 시간 설정
		concertInfo.startTime = sHour + TEXT(":" ) + sMin + TEXT(":" ) + TEXT("00");

		// 4. 콘서트 끝나는 시간 설정
		FString sH = EditText_SHour->GetText().ToString();
		FString sM = EditText_SMin->GetText().ToString();
		FString bH = EditText_H->GetText().ToString();
		FString bM = EditText_M->GetText().ToString();

		int h = FCString::Atoi(*sH) + FCString::Atoi(*bH);
		int m = FCString::Atoi(*sM) + FCString::Atoi(*bM);

		sH = ChangeString(FString::FromInt(h));
		sM = ChangeString(FString::FromInt(m));
		// 구조체에 끝나는 시간 설정
		concertInfo.endTime = sH + TEXT ( ":" ) + sM + TEXT ( ":" ) + TEXT ( "00" );

		return true;
	}
	else 
	{
		EffectPopUp1->SetVisibility(ESlateVisibility::Visible);
		UGameplayStatics::PlaySound2D ( GetWorld ( ) , PopUpSFV );
		return false;
	}
}

// 사용자가 9월이라고 작성시 09로 변경하는 함수
FString UStartWidget_KMK::ChangeString ( const FString& editText )
{
    FString s = editText;
    if (s.Len ( ) == 1)
    {
        s = TEXT ( "0" ) + s;
    }
	return s;
}

// AI에서 돌린 사진 값을 위젯에 넣음
void UStartWidget_KMK::ChangeImageStage ( UTexture2D* texture )
{
	Image_Stage->SetBrushFromTexture(texture);
	Image_SetStage->SetBrushFromTexture(texture);
}

// 데이터값이 int인지 확인하는 함수
bool UStartWidget_KMK::EditTextDigit ( const FString& editText )
{
	for (char c : editText)
	{
		if (!isdigit ( c ))
		{
			return false;
		}
	}
	return true;
}
// 티켓 생성부분 : 로직 변경 예정
void UStartWidget_KMK::PressCreateTicket ( )
{
	TMap<FString , FString> data;
	data.Add ( TEXT ( "prompt" ) , EditMultiText_Ticket ? EditMultiText_Ticket->GetText ( ).ToString ( ) : TEXT ( "" ) );

	if (Text_Price)
	{
		Text_Price->SetText ( FText::GetEmpty ( ) );
	}

	if (Image_Coin)
	{
		Image_Coin->SetVisibility ( ESlateVisibility::Hidden );
	}

	if (Image_FinalStageImage && Image_FinalStageImage->Brush.GetResourceObject ( ))
	{
		Image_FinalStageImage->Brush.SetResourceObject ( nullptr );
	}

	if (Image_Load)
	{
		Image_Load->SetVisibility ( ESlateVisibility::Visible );
	}

	if (httpActor && !IsOfflineMode ( ))
	{
		httpActor->ReqTicket ( data );
	}
	else
	{
		bCreateTicket = true;

		if (Image_Load)
		{
			Image_Load->SetVisibility ( ESlateVisibility::Hidden );
		}

		SetTicketButton ( );
	}
}
// 티켓이 생성되면 관련 image를 ticekt으로 변경함
void UStartWidget_KMK::CreateTicketMaterial ( UTexture2D* texture)
{
	Image_FinalStageImage->SetBrushFromTexture(texture);
	Image_FinalConcert->SetBrushFromTexture(texture);
}

// ai측에 보낼 이미지 파일 업로드
void UStartWidget_KMK::PressUpload ( )
{
    TArray<FString> SelectedFiles;
    FString FileTypes = TEXT("이미지 파일 (*.png;*.jpg;*.jpeg)|*.png;*.jpg;*.jpeg|모든 파일 (*.*)|*.*");

    // 파일 탐색기를 열고 사용자에게 파일 선택을 요청
    bool bFileSelected =  OpenFileExample(SelectedFiles, TEXT("업로드할 이미지를 선택하세요"), FileTypes, true);

    if (bFileSelected && SelectedFiles.Num() > 0)
    {
        for (const FString& FilePath : SelectedFiles)
        {
			// 파일이 올라오면 서버로 이미지를 보내 생성된 링크를 받아야함
            UE_LOG(LogTemp, Warning, TEXT("Selected File: %s"), *FilePath);
            FString FileName = FPaths::GetCleanFilename(FilePath); 
			// httpActor에 있는 ai용 multiform 링크 접속
			httpActor->ReqMultipartCapturedWithAI(FilePath, TEXT("https://singular-swine-deeply.ngrok-free.app/upload" ) );
			
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("File Selected Failed!!!"))
    }
}
void UStartWidget_KMK::SetTicketButton ( )
{
	Butt_CreateTicket->SetIsEnabled ( true );
	//Butt_CreateTicket1->SetIsEnabled ( true );
}

// 로컬 컴퓨터 파일을 열기 위한 함수임
bool UStartWidget_KMK::OpenFileExample(TArray<FString>& FileNames, FString DialogueTitle, FString FileTypes, bool multiselect)
{

   IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
   bool bOpened = false;
   FString DefaultPath = FPaths::ProjectContentDir(); // 기본 경로를 프로젝트 콘텐츠 폴더로 설정

   if (DesktopPlatform)
   {
      uint32 SelectionFlag = multiselect ? EFileDialogFlags::Multiple : EFileDialogFlags::None;
      bOpened = DesktopPlatform->OpenFileDialog(
         NULL,
         DialogueTitle,
         DefaultPath,
         TEXT(""),
         FileTypes,
         SelectionFlag,
         FileNames
      );
   }
   return bOpened;
}
// 예약했던 부분을 초기화함
void UStartWidget_KMK::ClearAllText ( )
{
	
	EditText_StageName->SetText( FText::GetEmpty ( ) );
	EditText_Year->SetText( FText::GetEmpty ( ) );
	EditText_Mon->SetText( FText::GetEmpty ( ) );
	EditText_Day->SetText( FText::GetEmpty ( ) );
	EditText_SHour->SetText( FText::GetEmpty ( ) );
	EditText_SMin->SetText( FText::GetEmpty ( ) );
	EditText_H->SetText( FText::GetEmpty ( ) );
	EditText_M->SetText( FText::GetEmpty ( ) );

	EditText_Price->SetText( FText::GetEmpty ( ) );
	EditMultiText_Ticket->SetText( FText::GetEmpty ( ) );

	concertInfo.appearedVFX = particleNum;
	concertInfo.feverVFX = feverNum;

	particleNum = 0;
	feverNum = 0;
	
	bCreateTicket =false;
}

// 필요한 패널만 visible시키는 함수
void UStartWidget_KMK::SetPanelVisible ( class UCanvasPanel* visiblePanel , class UCanvasPanel* hiddenPanel0 , class UCanvasPanel* hiddenPanel1  )
{
	hiddenPanel0->SetVisibility ( ESlateVisibility::Hidden );
	hiddenPanel1->SetVisibility ( ESlateVisibility::Hidden );
	visiblePanel->SetVisibility ( ESlateVisibility::Visible );
}
// 이펙트 설정 확인 함수
void UStartWidget_KMK::PressAppearEffect ( )
{
	bFever = false;
}
// 이펙트 설정 확인 함수
void UStartWidget_KMK::PressFeverEffect ( )
{
	bFever = true;
}
// 이펙트 설정 완료 버튼
void UStartWidget_KMK::PressSelect1Butt ( )
{
	// 이펙트 설정을 하지 않은 경우엔
	if (particleNum < 0 || feverNum < 0)
	{
		// 팝업이 뜨게 만듦
		EffectPopUp->SetVisibility(ESlateVisibility::Visible);
		UGameplayStatics::PlaySound2D ( GetWorld ( ) , PopUpSFV );
		return;
	}
	// conecrtInfo 구조체에 이펙트 값 넣기
	concertInfo.appearedVFX = particleNum;
	concertInfo.feverVFX = feverNum;
	// 다음 설정창으로 이동
	StartSwitcher->SetActiveWidgetIndex(2);
}

#pragma region Final Setting
// 수용인원 설정 관련 함수
void UStartWidget_KMK::PressNextButt ( )
{
	// 수용 인원 설정이 완료 안된 경우
	if (EditText_ScaleNum->GetText ( ).IsEmpty ( ))
	{
		// 팝업이 뜨면서 설정 완료해달라는 메세지 전달
		Text_Effect1->SetText(FText::FromString(TEXT("수용인원 설정을 완료해주세요")));
		EffectPopUp->SetVisibility(ESlateVisibility::Visible);
		UGameplayStatics::PlaySound2D ( GetWorld ( ) , PopUpSFV );
		return;
	}
	// concertInfo에 수용인원 설정을 int값으로 저장하고
	concertInfo.peopleScale = FCString::Atoi(*EditText_ScaleNum->GetText().ToString());
	// 다음 예약 창으로 변경함
	StagePayPanel->SetVisibility ( ESlateVisibility::Visible );
	Text_Pay->SetVisibility ( ESlateVisibility::Visible );
	Butt_PayMoney->SetVisibility ( ESlateVisibility::Visible );
	Image_Coin->SetVisibility ( ESlateVisibility::Visible );
	StageScalePanel->SetVisibility ( ESlateVisibility::Hidden );
	Text_FinalCount->SetText ( EditText_ScaleNum->GetText() );
	Text_FinalPay->SetText ( Text_Price->GetText() );
	FString s = Text_Price->GetText().ToString();
	s = s.Replace ( TEXT ( "," ) , TEXT ( "" ) );
	s = s.Replace ( TEXT ( " " ) , TEXT ( "" ) );
	int32 price = FCString::Atoi ( *s );
	// 내가 가진 캐쉬에서 차감
	if (gi->myCash < price)
	{
		// 결제에 실패했습니다.
		Text_Effect1->SetText ( FText::FromString ( TEXT ( "충전된 금액이 부족합니다." ) ) );
		EffectPopUp->SetVisibility ( ESlateVisibility::Visible );
		UGameplayStatics::PlaySound2D ( GetWorld ( ) , PopUpSFV );
		return;
	}
	gi->myCash -= price;
	Butt_Next->SetVisibility ( ESlateVisibility::Hidden );
	//Butt_CreateTicket1->SetVisibility(ESlateVisibility::Visible);
	Text_Price->SetText(FText::GetEmpty ( ) );
	EditText_ScaleNum->SetText ( FText::GetEmpty ( ) );
}


// 최종 결제를 누르게 되면
void UStartWidget_KMK::PressMoneyPay ( )
{
	if (!bCreateTicket)
	{
		if (Text_Effect1)    Text_Effect1->SetVisibility ( ESlateVisibility::Hidden );
		if (MultiText_PopUp) MultiText_PopUp->SetVisibility ( ESlateVisibility::Visible );
		if (EffectPopUp1)    EffectPopUp1->SetVisibility ( ESlateVisibility::Visible );
		return;
	}

	if (Text_Effect1)    Text_Effect1->SetVisibility ( ESlateVisibility::Visible );
	if (MultiText_PopUp) MultiText_PopUp->SetVisibility ( ESlateVisibility::Hidden );
	if (EffectPopUp1)    EffectPopUp1->SetVisibility ( ESlateVisibility::Hidden );
	if (Image_Load)      Image_Load->SetVisibility ( ESlateVisibility::Hidden );

	concertInfo.stageId = gi ? gi->stageNum : -1;

	if (httpActor && !IsOfflineMode ( ))
	{
		httpActor->ReqSetMyConcert ( concertInfo );
	}
	else
	{
		if (gi)
		{
			gi->concerInfo = concertInfo;
		}
		PressOkayButt ( );
	}
}

// 최종 결제창에서 okay를 누르면
void UStartWidget_KMK::PressOkayButt ( )
{
	if (gi)
	{
		gi->concerInfo = concertInfo;
	}

	if (Text_MyCash)
	{
		Text_MyCash->SetText ( FText::AsNumber ( gi ? gi->myCash : 0 ) );
	}

	ClearSB ( );

	if (StartSwitcher)
	{
		StartSwitcher->SetActiveWidgetIndex ( 1 );
	}

	ResetWidget ( );

	if (httpActor && !IsOfflineMode ( ))
	{
		httpActor->ReqCheckMyConcert ( );
	}
}

// 초기값으로 변경하는 부분 
void UStartWidget_KMK::ResetWidget ( )
{
	SetTitleText ( TEXT ( "공연 설정" ) );
	SetPanelVisible ( SetDayPanel , SetTicketPanel , StageChargePanel );
	
	StageChargePanel->SetVisibility(ESlateVisibility::Hidden);
	StageScalePanel->SetVisibility ( ESlateVisibility::Visible );
	StagePayPanel->SetVisibility ( ESlateVisibility::Hidden );
	Text_Pay->SetVisibility ( ESlateVisibility::Hidden );
	Butt_PayMoney->SetVisibility ( ESlateVisibility::Hidden );
	Image_Coin->SetVisibility ( ESlateVisibility::Hidden );
	PayPopUpPanel->SetVisibility ( ESlateVisibility::Hidden );
	Butt_Next->SetVisibility ( ESlateVisibility::Visible );
	Butt_CreateTicket1->SetVisibility(ESlateVisibility::Hidden);
}

#pragma endregion

#pragma endregion

#pragma region Entry Panel
// vip 접속
void UStartWidget_KMK::PressYesButt ( )
{
	if (gi && gi->roomNum >= 0)
	{
		// 콘서트 입장시, 해당
		httpActor->ReqCheckIdStage(gi->GetConcertInfo().stageId );
		
		// vip로 입장시에 joinRoom에 1도 전달해야함
		if (TEXT_VIP->GetText ( ).ToString ( ).Contains ( TEXT ( "VIP" ) ))
        {
			gi->JoinRoom(gi->roomNum , 1);
			// vip 티켓 획득하는 부분
			httpActor->ReqCollcetionConcert(gi->concerInfo.concertId);
			// httpActor->ReqCollcetionConcert ( 5 );
        }
		else
		{
			// 아니오를 누르면 vip말고 일반 입장함
			gi->JoinRoom(gi->roomNum );

		}
		//ChangeAudienceMesh(0);
	}
}
// 일반 접속
void UStartWidget_KMK::PressNormalEntry ( )
{
	TEXT_VIP->SetText(FText::FromString(TEXT("일반 입장하시겠습니까?" )));
	Text_TicketPrice->SetText(FText::AsNumber(0));
	MultiText_VIP->SetText(FText::FromString(TEXT("VIP 입장을 이용해보세요!\nVIP 입장시 더 많은 혜택을 누릴 수 있습니다" )));
	VIPPopUpPanel->SetVisibility(ESlateVisibility::Visible);
	UGameplayStatics::PlaySound2D ( GetWorld ( ) , PopUpSFV );
}
void UStartWidget_KMK::PressNoButt ( )
{
	VIPPopUpPanel->SetVisibility(ESlateVisibility::Hidden);
}

// 세션 확인할 수 있는 UI 띄우기
void UStartWidget_KMK::PressVipEntry ( )
{
	TEXT_VIP->SetText(FText::FromString(TEXT("VIP로 입장하시겠습니까?" )));
	Text_TicketPrice->SetText(FText::AsNumber(gi->mySessionInfo.ticketPrice));
	gi->myCash -= gi->mySessionInfo.ticketPrice;
	MultiText_VIP->SetText(FText::FromString(TEXT("VIP 입장시 앞열 관람, 티켓 콜렉션 수집이 가능하며,\n커스텀 응원봉으로 입장 등의 혜택이 있습니다" )));
	VIPPopUpPanel->SetVisibility(ESlateVisibility::Visible);
	UGameplayStatics::PlaySound2D ( GetWorld ( ) , PopUpSFV );
}

// 세션 찾기
void UStartWidget_KMK::FindRoom( )
{
	StartSwitcher->SetActiveWidgetIndex ( 3 );

	ClearChild();
	// 세션찾기
	if (gi)
	{
		gi->FindOtherSession();
	}
}
// 세션을 찾으면 그리드 밑에 생성함
void UStartWidget_KMK::CreateRoomWidget (const struct FRoomInfo& info )
{
	// 세션 관련 위잿 생성
	auto* wid = Cast<URoomWidget_KMK>(CreateWidget(GetWorld(), roomWidgetFact));
	if (wid)
	{
		wid->SetImageAndText(info);
		roomCount++;
		SB_FindStage->AddChild(wid);
	}
}
// 그리드 밑에 있는 child 삭제
void UStartWidget_KMK::ClearChild ( )
{
	if (SB_FindStage->GetChildrenCount ( ) > 0)
	{
		roomCount = 0;
		SB_FindStage->ClearChildren();
	}
}
// 로딩바를 hidden처리
void UStartWidget_KMK::SetLoadImage ( )
{
	Image_Load->SetVisibility(ESlateVisibility::Hidden);
}

#pragma endregion

bool UStartWidget_KMK::IsOfflineMode ( ) const
{
	return ( gi && gi->IsOfflineMode ( ) ) || ( httpActor == nullptr );
}

void UStartWidget_KMK::EnterOfflineLogin ( )
{
	if (gi)
	{
		gi->SetupOfflineDefaults ( );
	}

	if (StartSwitcher)
	{
		StartSwitcher->SetActiveWidgetIndex ( 1 );
	}

	ChangeMyProfile ( );
	SetButtEnable ( true );
}