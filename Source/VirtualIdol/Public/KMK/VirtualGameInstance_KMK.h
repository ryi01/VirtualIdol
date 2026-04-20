// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "HttpActor_KMK.h"
#include "VirtualGameInstance_KMK.generated.h"
USTRUCT ( BlueprintType )
struct FDummyNames : public FTableRowBase
{
	GENERATED_BODY ( )
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data" )
	FString name;
};
// 세션을 위한 roomInfo 구조체
USTRUCT ( BlueprintType )
struct FRoomInfo
{
	GENERATED_BODY ( )

	UPROPERTY(BlueprintReadOnly)
	FString roomName = TEXT("");
	UPROPERTY(BlueprintReadOnly)
	FString hostName= TEXT("");
	UPROPERTY(BlueprintReadOnly)
	int32 MaxPlayer = 0;
	UPROPERTY(BlueprintReadOnly)
	int32 CurrentPlayer = 0;
	UPROPERTY(BlueprintReadOnly)
	int32 pingMS = 0;
	UPROPERTY( )
	int32 ticketPrice = -1;
	UPROPERTY(BlueprintReadOnly )
	class UTexture2D* texture = NULL;
	UPROPERTY( )
	int32 feverNum = -1;

	int32 index = -1;

	FString ToString()
	{
		return FString::Printf(TEXT("%d) [%s] [%s] [%d /  %d] [ping: %d]"), index, *roomName, *hostName, MaxPlayer, CurrentPlayer, pingMS);
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam ( FSearchSignature , const struct FRoomInfo& , infrom );
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam ( FFindSignature , bool , value );

UCLASS()
class VIRTUALIDOL_API UVirtualGameInstance_KMK : public UGameInstance
{
	GENERATED_BODY()
	public : 
	virtual void Init() override;

	// 온라인 세션 인터페이스를 기억하고 싶다.
	IOnlineSessionPtr sessionInterface;

	// 호스트 이름은 서버에서 닉네임 받아올 예정
	FString HostName;
	UPROPERTY(EditAnywhere )
	FString leaveURL = TEXT("/CommonFile/Maps/LV_ALPHA" );
	UPROPERTY( )
	bool bLogin = false;
	UPROPERTY(BlueprintReadWrite )
	bool bPressSession = false;
	UPROPERTY(BlueprintReadWrite )
	bool bPressStage = false;
	void CreateMySession(FString RoomName, int32 PlayerCount);

	void OnMyCreateSessionComplete(FName SessionName, bool bSuccessful);

	// 찾을 방의 목록 => 방을 찾는 친구, 목록 자체가 아닌 목록을 포함한 포인터임
	// STL : TSharedPtr=> MakeShareable
	TSharedPtr<FOnlineSessionSearch> sessionSearch;
	// 방찾기 요청
	void FindOtherSession();
	// 방찾기 응답
	void OnMyFindSessionComplete(bool bSuccessful);


	FSearchSignature OnSearchSignatureCompleteDelegate;
	FFindSignature OnFindRoomCompDelegate;

	// 방입장 요청
	void JoinRoom(int32 ChooseRoomNum, int32 vipNum = 0);

	// 방입장 응답
	void JoinRoomComplete(FName SessionName, EOnJoinSessionCompleteResult::Type EOnJoinSessionCompleteResult);

	// 방퇴장 요청 => 클라이언트가 퇴장함, UI 호출 => 서버로 날리고 > 서버는 멀티로 날림 -> 응답 완료시 OnMyDestroyRoomComplete 불림
	void ExitRoom();

	UFUNCTION(Server, Reliable)
	void ServerRPCExitRoom();

	UFUNCTION(NetMulticast, Reliable)
	void MultiRPCExitRoom();

	// 방퇴장 응답
	void OnMyDestroyRoomComplete(FName RoomName, bool bWasSucessful);

	// 스팀에서는 UTF-8사용, 언리얼은 UTF-16사용 => 한글이 깨짐
	// 이를 해결하기 위함 Base64 인코딩 사용
	FString StringBase64Encode(const FString& str);

	FString StringBase64Decode(const FString& str);

	// 위잿 올리기
	UFUNCTION( )
	void SetStartWidget(class UStartWidget_KMK* startUi );
	UPROPERTY( )
	class UStartWidget_KMK* widget;
	UFUNCTION( )
	void SwitchStartUIWidget(int32 num );
	UFUNCTION( )
	void VisibleStartWidget ( bool bVisible );
	UFUNCTION( )
    void SwitchWidget ( int32 num );
	UFUNCTION ( )
    void PopUpVisible ( );
	UFUNCTION ( )
    void LoginPanel ( );

    UPROPERTY ( )
    int32 playerMeshNum = 0;

#pragma region Token
	UPROPERTY(BlueprintReadWrite )
	FLoginInfo loginInfo;
	UFUNCTION( )
	void SetMyInfo(const struct FLoginInfo& info  );
	UFUNCTION(BlueprintCallable )
	FLoginInfo GetMyInfo( );
	UPROPERTY( )
	FConcertInfo concerInfo;
	UFUNCTION( )
	void SetConcertInfo( const TArray<FConcertInfo> info, class AHttpActor_KMK* http );
	UPROPERTY(EditAnywhere )
	FString start = TEXT("2024-9-14" );
	FString ChangeString ( const FString& editText );
	UFUNCTION (BlueprintCallable)
	FConcertInfo GetConcertInfo(  );

	// HttpActor
	class AHttpActor_KMK* HttpActor;
#pragma endregion
#pragma region Chat

	UFUNCTION(BlueprintCallable )
	void SetWidget(class UAudience_KMK* wid );

	UPROPERTY( )
	class UAudience_KMK* myWidget;
#pragma endregion

	UFUNCTION ( BlueprintCallable )
	FString GetLocalIPAddress ( );

	UFUNCTION ( BlueprintCallable )
	void SendMulticastMessage ( );

	// StageInfo를 저장할 멤버 변수
	UPROPERTY()
	int32 roomNum = 0;
	UPROPERTY()
	FStageInfo myStageInfo;
	UPROPERTY( )
	int32 stageNum = 0;
	UPROPERTY()
	FRoomInfo mySessionInfo;
	
	UFUNCTION (BlueprintCallable)
	void OnJoinSessionButt(  );
	UFUNCTION(BlueprintCallable)
	void OnSetStageButt(  );
		
	UPROPERTY( )
	class AJJH_SelectManager* sm;

	UPROPERTY( )
	class URoomWidget_KMK* roomWidget;
	UPROPERTY( )
	class URoomWidget_KMK* sessionWidget;

	UPROPERTY(EditAnywhere, Category = Cash )
	int32 myCash = 1000000;

	UFUNCTION( )
	void SetMyProfile( );

	UFUNCTION( )
	void ChangeTextureWidget(UTexture2D* texture );

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TSubclassOf<AActor>> effectArray;	
	
	UPROPERTY ( EditAnywhere , BlueprintReadWrite )
	TArray<TSubclassOf<AActor>> effectArrayForEachFloor;
	
	UPROPERTY ( EditAnywhere , BlueprintReadWrite )
	TSubclassOf<AActor> GINowEffect;

	UPROPERTY( )
	FStageInfo concertStageInfo;

	UFUNCTION( )
	void SetConcertStageInfo(FStageInfo& info );

	UFUNCTION( )
	FStageInfo GetConcertStageInfo( );

	UPROPERTY ( )
	TArray<struct FConcertInfo> allConcertInfoArray;

	// 데이터 테이블을 블루프린트에 노출
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Data")
    UDataTable* NamesDataTable;

	UFUNCTION(BlueprintCallable, Category = "Data")
	FString GetRandomName();

	UPROPERTY( )
	FTransform spawnTrans = FTransform(FVector(0 ) );

	UFUNCTION( )
	void MusicReload( );

public:
	UPROPERTY ( EditAnywhere , BlueprintReadWrite , Category = "Offline" )
	bool bOfflineMode = true;   // 서버 없이 실행할 때 true

	UFUNCTION ( BlueprintCallable )
	bool IsOfflineMode ( ) const { return bOfflineMode; }

	FString GetSafeUserName ( ) const;
	void SetupOfflineDefaults ( );
};
