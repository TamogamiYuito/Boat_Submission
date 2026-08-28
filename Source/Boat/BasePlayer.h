#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "BasePlayer.generated.h"

// =========================
// Forward declarations
// =========================

class UBoxComponent;
class UCameraComponent;
class UJoyConUdpReceiverComponent;
class USceneComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class UWaterBodyComponent;

UCLASS()
class BOAT_API ABasePlayer : public APawn
{
    GENERATED_BODY()

public:
    ABasePlayer();

    virtual void Tick(float DeltaTime) override;
    virtual void PawnClientRestart() override;

protected:
    virtual void BeginPlay() override;

    // =========================
    // Boat components
    // =========================

    // 船全体の移動・Sweep判定に使うRoot Collision
    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadOnly,
        Category = "Boat|Collision")
    TObjectPtr<UBoxComponent> BoatCollision;

    // オールを除いた船体の見た目
    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadOnly,
        Category = "Boat")
    TObjectPtr<UStaticMeshComponent> BoatMesh;

    // 左オールの回転中心
    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadOnly,
        Category = "Boat|Oar")
    TObjectPtr<USceneComponent> LeftOarPivot;

    // 左オール本体
    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadOnly,
        Category = "Boat|Oar")
    TObjectPtr<UStaticMeshComponent> LeftOarMesh;

    // 右オールの回転中心
    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadOnly,
        Category = "Boat|Oar")
    TObjectPtr<USceneComponent> RightOarPivot;

    // 右オール本体
    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadOnly,
        Category = "Boat|Oar")
    TObjectPtr<UStaticMeshComponent> RightOarMesh;

    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadOnly,
        Category = "Joy-Con UDP")
    TObjectPtr<UJoyConUdpReceiverComponent> JoyConUdpReceiver;

    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadOnly,
        Category = "Camera")
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadOnly,
        Category = "Camera")
    TObjectPtr<UCameraComponent> FollowCamera;

    // =========================
    // Collision
    // =========================

    // Box Extentは半径。全体サイズはこの値の2倍。
    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Boat|Collision",
        meta = (ClampMin = "1.0"))
    FVector BoatCollisionHalfExtent =
        FVector(210.0f, 100.0f, 45.0f);

    // 回転判定時にBoxを少し縮め、接触中の誤判定を減らす。
    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Boat|Collision",
        meta = (ClampMin = "0.50", ClampMax = "1.00"))
    float RotationCollisionScale = 0.92f;

    // =========================
    // Paddle input
    // =========================

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Paddle")
    float InputMultiplier = 1.0f;

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Paddle",
        meta = (ClampMin = "0.0", ClampMax = "0.95"))
    float PaddleDeadZone = 0.04f;

    // 手前へ引く操作が正になるように反転する。
    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Paddle")
    bool bInvertLeftPaddle = false;

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Paddle")
    bool bInvertRightPaddle = false;

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Boat|Turning")
    bool bInvertTurnDirection = false;

    // =========================
    // Oar animation
    // =========================

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Boat|Oar Animation")
    bool bEnableOarAnimation = true;

    // オールが入力へ追従する速さ
    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Boat|Oar Animation",
        meta = (ClampMin = "0.0"))
    float OarRotationInterpSpeed = 10.0f;

    // 左オールを手前へ引いたときの回転量
    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Boat|Oar Animation")
    FRotator LeftOarPullRotation =
        FRotator(0.0f, 0.0f, -45.0f);

    // 左オールを奥へ押したときの回転量
    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Boat|Oar Animation")
    FRotator LeftOarPushRotation =
        FRotator(8.0f, 40.0f, 0.0f);

    // 右オールを手前へ引いたときの回転量
    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Boat|Oar Animation")
    FRotator RightOarPullRotation =
        FRotator(-10.0f, 55.0f, 0.0f);

    // 右オールを奥へ押したときの回転量
    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Boat|Oar Animation")
    FRotator RightOarPushRotation =
        FRotator(8.0f, -40.0f, 0.0f);


    // オールを回すPivotのローカル軸
// Yawで動かす場合はZ軸
    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Boat|Oar Animation")
    FVector OarLocalRotationAxis =
        FVector(0.0f, 0.0f, 1.0f);

    // 引いたときの角度
    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Boat|Oar Animation")
    float OarPullAngle = 45.0f;

    // 押したときの角度
    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Boat|Oar Animation")
    float OarPushAngle = -35.0f;

    FQuat InitialLeftOarQuat =
        FQuat::Identity;

    FQuat InitialRightOarQuat =
        FQuat::Identity;

    // =========================
    // Boat movement
    // =========================

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Boat|Movement",
        meta = (ClampMin = "0.0"))
    float ForwardAcceleration = 950.0f;

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Boat|Movement",
        meta = (ClampMin = "0.0"))
    float ReverseAcceleration = 650.0f;

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Boat|Movement",
        meta = (ClampMin = "0.0"))
    float MaxForwardSpeed = 900.0f;

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Boat|Movement",
        meta = (ClampMin = "0.0"))
    float MaxReverseSpeed = 400.0f;

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Boat|Movement",
        meta = (ClampMin = "0.0"))
    float PaddleBurstAcceleration = 300.0f;

    // 小さいほど滑り続け、大きいほど早く止まる。
    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Boat|Water",
        meta = (ClampMin = "0.0"))
    float LinearWaterDrag = 0.85f;

    // =========================
    // Turning
    // =========================

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Boat|Turning",
        meta = (ClampMin = "0.0"))
    float TurnAcceleration = 240.0f;

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Boat|Turning",
        meta = (ClampMin = "0.0"))
    float MaxYawSpeed = 120.0f;

    // 小さいほど回転の勢いが長く残る。
    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Boat|Turning",
        meta = (ClampMin = "0.0"))
    float AngularWaterDrag = 4.0f;

    // =========================
    // Camera
    // =========================

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Camera")
    float CameraArmLength = 650.0f;

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Camera")
    FVector CameraSocketOffset =
        FVector(0.0f, 0.0f, 140.0f);

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Camera")
    float NormalFOV = 80.0f;

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Camera")
    float HighSpeedFOV = 100.0f;

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Camera")
    float FOVInterpSpeed = 4.0f;

    // =========================
    // Ocean floating
    // =========================

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Boat|Ocean Floating")
    bool bEnableOceanFloating = true;

    // 開始時に補間せず海面位置へ合わせる。
    // PlayerStartは主にX・Y・Yawを決め、Zは海面から計算される。
    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Boat|Ocean Floating")
    bool bSnapToOceanOnBeginPlay = true;

    // 浮遊点を海面から何cm沈めるか。
    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Boat|Ocean Floating",
        meta = (ClampMin = "0.0"))
    float DesiredSubmersion = 20.0f;

    // 最終的な船全体の高さ調整。
    // 正で上、負で下。
    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Boat|Ocean Floating")
    float BoatFloatOffsetZ = 0.0f;

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Boat|Ocean Floating",
        meta = (ClampMin = "0.0"))
    float FloatHeightInterpSpeed = 3.0f;

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Boat|Ocean Floating",
        meta = (ClampMin = "0.0"))
    float FloatRotationInterpSpeed = 2.5f;

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Boat|Ocean Floating",
        meta = (ClampMin = "0.0", ClampMax = "45.0"))
    float MaxFloatTiltAngle = 10.0f;

    // 浮遊点の前後位置。BoatCollisionのExtentに対する割合。
    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Boat|Ocean Floating",
        meta = (ClampMin = "0.10", ClampMax = "1.00"))
    float BuoyancyLengthRatio = 0.75f;

    // 浮遊点の左右位置。BoatCollisionのExtentに対する割合。
    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Boat|Ocean Floating",
        meta = (ClampMin = "0.10", ClampMax = "1.00"))
    float BuoyancyWidthRatio = 0.75f;

    // 浮遊点をCollision中心から下へ置く割合。
    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Boat|Ocean Floating",
        meta = (ClampMin = "0.00", ClampMax = "1.00"))
    float BuoyancyVerticalRatio = 0.55f;

    // 4点全体をRootローカル座標で微調整する。
    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Boat|Ocean Floating")
    FVector BuoyancyCenterOffset = FVector::ZeroVector;

    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadOnly,
        Category = "Boat|Ocean Floating")
    TArray<FVector> BuoyancyPoints;

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Boat|Ocean Floating")
    bool bDrawBuoyancyDebug = true;

    // =========================
    // Debug
    // =========================

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Debug")
    bool bShowInputDebug = true;

    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Debug")
    bool bLogBoatSizeOnBeginPlay = true;

    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadOnly,
        Category = "Paddle")
    float LeftPaddleRawValue = 0.0f;

    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadOnly,
        Category = "Paddle")
    float RightPaddleRawValue = 0.0f;

    // 引くと正、押すと負、持ち上げていると0。
    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadOnly,
        Category = "Paddle")
    float LeftPaddlePower = 0.0f;

    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadOnly,
        Category = "Paddle")
    float RightPaddlePower = 0.0f;

    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadOnly,
        Category = "Boat|Debug")
    float CurrentThrustInput = 0.0f;

    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadOnly,
        Category = "Boat|Debug")
    float CurrentTurnInput = 0.0f;

    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadOnly,
        Category = "Boat|Debug")
    float CurrentForwardSpeed = 0.0f;

    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadOnly,
        Category = "Boat|Debug")
    float CurrentYawSpeed = 0.0f;

private:
    float BoatYawDegrees = 0.0f;
    bool bFloatingInitialized = false;

    UPROPERTY(Transient)
    TObjectPtr<UWaterBodyComponent> OceanWaterComponent;

    float ConvertToSignedPaddle(
        float RawValue,
        bool bInvertInput) const;

    void CalculatePaddleForces();
    void UpdateBoatMovement(float DeltaTime);
    void ShowInputDebug() const;

    void ConfigureBoatCollision();
    bool TrySetBoatRotation(const FRotator& TargetRotation);

    void CacheOceanWaterBody();
    void BuildBuoyancyPointsFromCollision();

    // 成功して海面位置を適用できた場合はtrue。
    bool UpdateOceanFloating(
        float DeltaTime,
        bool bSnapImmediately);

    bool SampleOceanSurface(
        const FVector& WorldLocation,
        FVector& OutSurfaceLocation,
        FVector& OutSurfaceNormal) const;

    FRotator InitialLeftOarRotation =
    FRotator::ZeroRotator;

FRotator InitialRightOarRotation =
    FRotator::ZeroRotator;

void UpdateOarAnimation(float DeltaTime);

};
