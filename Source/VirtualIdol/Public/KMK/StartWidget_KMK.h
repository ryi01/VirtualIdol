
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HttpActor_KMK.h"
#include "StartWidget_KMK.generated.h"

// 내부 프로젝트 시작시 나오는 화면들
// 로그인, 무대 입장 등의 버튼 판넬 등 관리하는 클래스임

UCLASS()
class VIRTUALIDOL_API UStartWidget_KMK : public UUserWidget
{
	GENERATED_BODY()

public:
//이펙트

	UPROPERTY ( EditAnywhere , BlueprintReadWrite )
	TSubclassOf<AActor> NowEffect;

	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	UPROPERTY(meta = (BindWidget))
	class UWidgetSwitcher* StartSwitcher;

	UPROPERTY(EditAnywhere, Category = Price)
	int32 concertPrice = 1000;
	UFUNCTION ()
	void GoBack();
	
	// 인터페이스
	UPROPERTY( )
	class UVirtualGameInstance_KMK* gi;

	// httpactor
	UPROPERTY( EditAnywhere, BlueprintReadWrite)
	class AHttpActor_KMK* httpActor;
	UPROPERTY(EditAnywhere, Category = Http )
	TSubclassOf<class AHttpActor_KMK> httpFact;

	UPROPERTY( )
	class AJJH_SelectManager* selectManager;

	UPROPERTY(EditAnywhere, Category = select )
	TSubclassOf<class AJJH_SelectManager> selectFact;

	// 사운드
	UPROPERTY(EditAnywhere, Category = Sound )
	class USoundBase* PopUpSFV;

#pragma region Login Widget Panel
// ================================================================
// Login Widget Panel
// ================================================================
	// 버튼
	UPROPERTY(meta = (BindWidget))
    class UButton* Butt_Login;

	// 텍스트 블럭
	UPROPERTY(meta = (BindWidget))
	class UEditableText* EditText_ID;
	UPROPERTY(meta = (BindWidget))
	class UEditableText* EditText_PW;
	UPROPERTY(meta = (BindWidget))
    class UButton* Butt_FailLogin;
	UPROPERTY ( meta = ( BindWidget ) )
	class UCanvasPanel* FailLoginPanel;

	UPROPERTY ( meta = ( BindWidget ) )
	class UButton* Button_Homepage_1;
	UPROPERTY ( meta = ( BindWidget ) )
	class UButton* Button_Homepage_2;

	UPROPERTY ( meta = ( BindWidget ) )
	class UButton* Butt_SignIn;
	
	// 버튼 연동 함수
	UFUNCTION ()
	void OnMyLogin();
	UFUNCTION ()
	void OnFailLogin();
	UFUNCTION( )
	void OnHomepage( );
	UFUNCTION( )
	void OnSignInPage( );
#pragma endregion
#pragma region  Four Button Widget Panel
// ================================================================
// Four Button Widget Panel
// ================================================================
	
	// 버튼
	UPROPERTY(meta = (BindWidget))
    class UButton* Butt_CreateStage;
	UPROPERTY(meta = (BindWidget))
	class UButton* Butt_ReadyConcert;
	UPROPERTY(meta = (BindWidget))
	class UButton* Butt_StartConcert;
	UPROPERTY(meta = (BindWidget))
	class UButton* Butt_ComeInStage;

	UFUNCTION( )
	void SetButtEnable(bool bEnable = false);

	UPROPERTY(BlueprintReadWrite )
	bool isButtEanble = false;

	// 버튼 연동 함수
	UFUNCTION()
	void CreateStagePanel();
	UFUNCTION()
	void SettingStagePanel();
	UFUNCTION()
	void StartConcertPanel();
	UFUNCTION()
	void ComeInStagePanel();
#pragma endregion

#pragma region Stage Setting Widget Panel
// ================================================================
// Stage Setting Widget Panel
// ================================================================
	// 추가 기능
	// 텍스트
	UPROPERTY(meta = (BindWidget))
    class UTextBlock* Text_Title;
	UPROPERTY(meta = (BindWidget))
    class UTextBlock* Text_MyNick;	
	UPROPERTY(meta = (BindWidget))
    class UTextBlock* Text_MyCash;	
	UPROPERTY ( meta = ( BindWidget ) )
	class UButton* Butt_Select;
	UPROPERTY ( meta = ( BindWidget ) )
    class UImage* Image_Profile;
	UPROPERTY ( meta = ( BindWidget ) )
    class UImage* Image_Load;
	UPROPERTY ( meta = ( BindWidget ) )
    class UImage* Image_Stage;
	UFUNCTION( )
	void PressSelectButt( );
	UFUNCTION( )
	void SetImageProfile(class UTexture2D* texture );

	UFUNCTION( )
	void ChangeMyProfile( );

	UPROPERTY(EditAnywhere, Category = load )
	UMaterialInstance* loadMatFact;
	UPROPERTY( )
	UMaterialInstanceDynamic* loadMatInst;
	UFUNCTION( )
	void ChangeLoadMat(float num );
	float matNum = 0;
	int32 selectNum = 0;

	UFUNCTION( )
	void SetTitleText(const FString& title );

	UPROPERTY( )
	FConcertInfo concertInfo;
#pragma region Set Day
	// 공연일자 패널
	UPROPERTY ( meta = ( BindWidget ) )
    class UCanvasPanel* SetDayPanel;
	UPROPERTY ( meta = ( BindWidget ) )
    class UEditableText* EditText_StageName;
	UPROPERTY ( meta = ( BindWidget ) )
    class UEditableText* EditText_Year;
	UPROPERTY ( meta = ( BindWidget ) )
    class UEditableText* EditText_Mon;
	UPROPERTY ( meta = ( BindWidget ) )
    class UEditableText* EditText_Day;
	UPROPERTY ( meta = ( BindWidget ) )
    class UEditableText* EditText_SHour;
	UPROPERTY ( meta = ( BindWidget ) )
    class UEditableText* EditText_SMin;
	UPROPERTY ( meta = ( BindWidget ) )
    class UEditableText* EditText_H;
	UPROPERTY ( meta = ( BindWidget ) )
	class UEditableText* EditText_M;

	UFUNCTION( )
	bool BEditTextEmpty( );

	FString ChangeString(const FString& editText);

	UFUNCTION( )
	void ChangeImageStage(UTexture2D* texture );

	bool EditTextDigit(const FString& editText );

#pragma endregion
#pragma region Set Ticket
	// 티켓 만드는 패널
	UPROPERTY ( meta = ( BindWidget ) )
	class UCanvasPanel* SetTicketPanel;
	UPROPERTY ( meta = ( BindWidget ) )
    class UEditableText* EditText_Price;
	UPROPERTY ( meta = ( BindWidget ) )
    class UImage* Image_SetStage;
	UPROPERTY ( meta = ( BindWidget ) )
    class UImage* Image_Coin;
	UPROPERTY ( meta = ( BindWidget ) )
    class UButton* Butt_CreateTicket;
	UPROPERTY ( meta = ( BindWidget ) )
    class UButton* Butt_CreateTicket1;
	UPROPERTY ( meta = ( BindWidget ) )
    class UButton* Butt_Upload;
	UPROPERTY ( meta = ( BindWidget ) )
	class UMultiLineEditableText* EditMultiText_Ticket;
	UFUNCTION( )
	void PressCreateTicket();
	UFUNCTION( )
	void CreateTicketMaterial(UTexture2D* texture);
	UFUNCTION( )
	void PressUpload( );
	UFUNCTION( )
	bool OpenFileExample(TArray<FString>& FileNames, FString DialogueTitle, FString FileTypes, bool multiselect);
	void ClearAllText( );
#pragma endregion
#pragma region Set Effect
	// 이펙트 결정 패널
    UPROPERTY ( meta = ( BindWidget ) )
    class UCanvasPanel* SetEffectPanel;
	UPROPERTY ( meta = ( BindWidget ) )
	class UImage* Image_Effect;
	UPROPERTY ( meta = ( BindWidget ) )
	class UButton* Butt_AppearEffect;
	UPROPERTY ( meta = ( BindWidget ) )
    class UButton* Butt_FeverEffect;	
	UPROPERTY ( meta = ( BindWidget ) )
    class UButton* Butt_Select1;

	UPROPERTY(BlueprintReadWrite )
	int32 particleNum = -1;
	UPROPERTY(BlueprintReadWrite )
	int32 feverNum = -1;
	UPROPERTY(BlueprintReadWrite )
	bool bFever = false;

	UFUNCTION( )
	void SetPanelVisible( class UCanvasPanel* visiblePanel, class UCanvasPanel* hiddenPanel0 , class UCanvasPanel* hiddenPanel1  );

	UFUNCTION( )
	void PressAppearEffect( );
	UFUNCTION( )
	void PressFeverEffect( );
	UFUNCTION( )
	void PressSelect1Butt( );

	UPROPERTY( BlueprintReadWrite , meta = ( BindWidget ) )
    class UCanvasPanel* EffectPopUp;
	UPROPERTY ( BlueprintReadWrite , meta = ( BindWidget ) )
    class UCanvasPanel* EffectPopUp1;
	UPROPERTY ( meta = ( BindWidget ) )
    class UTextBlock* Text_Effect1;	
	UPROPERTY ( meta = ( BindWidget ) )
    class UMultiLineEditableTextBox* MultiText_PopUp;
#pragma endregion

#pragma region Final Set Stage
	// 최종 결제 페널
	UPROPERTY(meta = (BindWidget))
    class UCanvasPanel* StageChargePanel;
	UPROPERTY(meta = (BindWidget))
    class UTextBlock* Text_FinalName;	
	UPROPERTY(meta = (BindWidget))
    class UTextBlock* Text_FinalYear;	
	UPROPERTY(meta = (BindWidget))
    class UTextBlock* Text_FinalMon;
	UPROPERTY(meta = (BindWidget))
    class UTextBlock* Text_FinalDay;	
	UPROPERTY(meta = (BindWidget))
    class UTextBlock* Text_StartHour;	
	UPROPERTY(meta = (BindWidget))
    class UTextBlock* Text_StartMin;	
	UPROPERTY(meta = (BindWidget))
    class UTextBlock* Text_FinalCount;	
	UPROPERTY(meta = (BindWidget))
    class UTextBlock* Text_FinalPay;		
	UPROPERTY ( meta = ( BindWidget ) )
    class UTextBlock* Text_Price;
	UPROPERTY(meta = (BindWidget))
    class UImage* Image_FinalStageImage;
	UPROPERTY(meta = (BindWidget))
    class UImage* Image_FinalConcert;	

	UPROPERTY ( meta = ( BindWidget ) )
	class UButton* Butt_Next;
#pragma region Set Scale
	UPROPERTY ( meta = ( BindWidget ) )
    class UGridPanel* StageScalePanel;	
	UPROPERTY ( meta = ( BindWidget ) )
    class UEditableText* EditText_ScaleNum;

	UFUNCTION( )
	void PressNextButt( );

#pragma endregion
#pragma region Set Price
	UPROPERTY ( meta = ( BindWidget ) )
	class UGridPanel* StagePayPanel;
	UPROPERTY ( meta = ( BindWidget ) )
	class UTextBlock* Text_Pay;	
	UPROPERTY ( meta = ( BindWidget ) )
    class UButton* Butt_PayMoney;	
	UFUNCTION( )
	void SetTicketButton( );

	UFUNCTION( )
	void PressMoneyPay( );
	UPROPERTY( )
	bool bPayMoney = false;

	UPROPERTY ( meta = ( BindWidget ) )
	class UCanvasPanel* PayPopUpPanel;
	UPROPERTY ( meta = ( BindWidget ) )
	class UButton* Butt_Okay;
	UPROPERTY ( meta = ( BindWidget ) )
	class UTextBlock* Text_FinalStageName;

	UFUNCTION( )
	void PressOkayButt( );

	void ResetWidget( );
#pragma endregion


#pragma endregion

#pragma endregion

#pragma region Select Stage
	// 전체 무대
	TArray<struct FStageInfo> allStageInfoArray;
	// 버튼
	UPROPERTY(meta = (BindWidget))
    class UButton* Butt_UserStage;
	UPROPERTY ( meta = ( BindWidget ) )
    class UButton* Butt_MyStage;
	UPROPERTY ( meta = ( BindWidget ) )
    class UButton* Butt_Star;
	// 그리드
	UPROPERTY ( meta = ( BindWidget ) )
    class UGridPanel* FindRoomGrid;	
	UPROPERTY ( meta = ( BindWidget ) )
    class UScrollBox* SB_FindStage;
	// 추가함수
	UFUNCTION()
	void PressUserStageButt( );
	UFUNCTION()
	void PressMyStageButt( );
	UFUNCTION()
	void CreateStageWidget( const struct FStageInfo& stageInfo, UTexture2D* image );
	UFUNCTION( )
	void ClearSB( );
#pragma endregion
#pragma region EntryPanel Widget Panel
// ================================================================
// EntryPanel Widget Panel
// ================================================================
	UPROPERTY(meta = (BindWidget))
	class UCanvasPanel* VIPPopUpPanel;
	// 버튼
	UPROPERTY(meta = (BindWidget))
	class UButton* Butt_Yes;
	UPROPERTY(meta = (BindWidget))
    class UButton* Butt_NormalEntry;	
	UPROPERTY(meta = (BindWidget))
    class UButton* Butt_No;	
	UPROPERTY(meta = (BindWidget))
    class UButton* Butt_VipEntry;	
	UPROPERTY ( meta = ( BindWidget ) )
    class UButton* Butt_Back;
	UPROPERTY ( meta = ( BindWidget ) )
    class UButton* Butt_Back1;
	UPROPERTY ( meta = ( BindWidget ) )
	class UButton* Butt_Back2;
	UPROPERTY ( meta = ( BindWidget ) )
	class UButton* Butt_Back3;

	UPROPERTY(meta = (BindWidget))
    class UTextBlock* TEXT_VIP;
	UPROPERTY(meta = (BindWidget))
    class UTextBlock* Text_TicketPrice;
	UPROPERTY ( meta = ( BindWidget ) )
    class UMultiLineEditableTextBox* MultiText_VIP;
	// 버튼 연동 함수
    UFUNCTION ( )
	void PressYesButt( );
	UFUNCTION ( )
	void PressNoButt( );
    UFUNCTION ( )
    void PressVipEntry ( );
	UFUNCTION()
	void PressNormalEntry();

	// 추가함수
	UFUNCTION()
	void FindRoom();
	UFUNCTION()
	void CreateRoomWidget(const struct FRoomInfo& info);
	UPROPERTY(EditAnywhere, Category="Room" )
	TSubclassOf<class UUserWidget> roomWidgetFact;

	int32 roomCount = 0;

	UFUNCTION ()
	void ClearChild( );
	
	class UTexture2D* t;
	UPROPERTY()
	int32 roomNum = -1;
	
#pragma endregion
#pragma region FindRoom & Select Stage
	UPROPERTY( )
	bool bCreateTicket = false;
	UFUNCTION( )
	void SetLoadImage( );

	UPROPERTY(EditAnywhere, BlueprintReadWrite )
	FTransform spawnTrans = FTransform(FVector(0 ) );
#pragma endregion

protected:
	bool IsOfflineMode ( ) const;
	void EnterOfflineLogin ( );
};
