#include "BasePlayer.h"

#include "JoyConUdpReceiverComponent.h"

#include "Camera/CameraComponent.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Math/RotationMatrix.h"
#include "Templates/ValueOrError.h"
#include "UObject/UObjectIterator.h"

// Water plugin
#include "WaterBodyComponent.h"
#include "WaterBodyOceanComponent.h"
#include "WaterBodyTypes.h"

ABasePlayer::ABasePlayer()
{
    PrimaryActorTick.bCanEverTick = true;

    // スポーン時に大きなBox Collisionによって
    // PlayerStartから別位置へ自動移動されないようにする。
    SpawnCollisionHandlingMethod =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    // =========================
    // Root collision
    // =========================

    BoatCollision =
        CreateDefaultSubobject<UBoxComponent>(
            TEXT("BoatCollision"));

    SetRootComponent(BoatCollision);

    BoatCollision->InitBoxExtent(
        BoatCollisionHalfExtent);

    BoatCollision->SetMobility(
        EComponentMobility::Movable);

    BoatCollision->SetSimulatePhysics(false);
    BoatCollision->SetEnableGravity(false);

    // 洞窟・岩などとの衝突はRootのBoxが担当する。
    BoatCollision->SetCollisionProfileName(
        TEXT("Pawn"));

    BoatCollision->SetCollisionEnabled(
        ECollisionEnabled::QueryAndPhysics);

    BoatCollision->SetGenerateOverlapEvents(true);
    BoatCollision->SetCanEverAffectNavigation(false);
    BoatCollision->CanCharacterStepUpOn =
        ECB_No;

    // カメラのSpringArmが自分のBoxで縮まないようにする。
    BoatCollision->SetCollisionResponseToChannel(
        ECC_Camera,
        ECR_Ignore);

    // =========================
    // Joy-Con UDP
    // =========================

    JoyConUdpReceiver =
        CreateDefaultSubobject<UJoyConUdpReceiverComponent>(
            TEXT("JoyConUdpReceiver"));

    // =========================
    // Boat visual mesh
    // =========================

    BoatMesh =
        CreateDefaultSubobject<UStaticMeshComponent>(
            TEXT("BoatMesh"));

    BoatMesh->SetupAttachment(BoatCollision);
    BoatMesh->SetMobility(EComponentMobility::Movable);

    // 衝突はBoatCollisionだけに担当させる。
    BoatMesh->SetCollisionEnabled(
        ECollisionEnabled::NoCollision);
    BoatMesh->SetGenerateOverlapEvents(false);
    BoatMesh->SetSimulatePhysics(false);
    BoatMesh->SetEnableGravity(false);

    // =========================
    // Left oar
    // =========================

    LeftOarPivot =
        CreateDefaultSubobject<USceneComponent>(
            TEXT("LeftOarPivot"));

    LeftOarPivot->SetupAttachment(BoatMesh);

    LeftOarMesh =
        CreateDefaultSubobject<UStaticMeshComponent>(
            TEXT("LeftOarMesh"));

    LeftOarMesh->SetupAttachment(LeftOarPivot);
    LeftOarMesh->SetMobility(EComponentMobility::Movable);
    LeftOarMesh->SetCollisionEnabled(
        ECollisionEnabled::NoCollision);
    LeftOarMesh->SetGenerateOverlapEvents(false);


    // =========================
    // Right oar
    // =========================

    RightOarPivot =
        CreateDefaultSubobject<USceneComponent>(
            TEXT("RightOarPivot"));

    RightOarPivot->SetupAttachment(BoatMesh);

    RightOarMesh =
        CreateDefaultSubobject<UStaticMeshComponent>(
            TEXT("RightOarMesh"));

    RightOarMesh->SetupAttachment(RightOarPivot);
    RightOarMesh->SetMobility(EComponentMobility::Movable);
    RightOarMesh->SetCollisionEnabled(
        ECollisionEnabled::NoCollision);
    RightOarMesh->SetGenerateOverlapEvents(false);

    // =========================
    // Camera
    // =========================

    CameraBoom =
        CreateDefaultSubobject<USpringArmComponent>(
            TEXT("CameraBoom"));

    CameraBoom->SetupAttachment(BoatCollision);
    CameraBoom->TargetArmLength = CameraArmLength;
    CameraBoom->SocketOffset = CameraSocketOffset;
    CameraBoom->SetRelativeRotation(
        FRotator(-14.0f, 0.0f, 0.0f));

    CameraBoom->bUsePawnControlRotation = false;
    CameraBoom->bInheritPitch = false;
    CameraBoom->bInheritYaw = true;
    CameraBoom->bInheritRoll = false;

    CameraBoom->bEnableCameraLag = true;
    CameraBoom->CameraLagSpeed = 3.5f;
    CameraBoom->bEnableCameraRotationLag = true;
    CameraBoom->CameraRotationLagSpeed = 5.0f;

    FollowCamera =
        CreateDefaultSubobject<UCameraComponent>(
            TEXT("FollowCamera"));

    FollowCamera->SetupAttachment(
        CameraBoom,
        USpringArmComponent::SocketName);

    FollowCamera->bUsePawnControlRotation = false;
    FollowCamera->SetAutoActivate(true);
}

void ABasePlayer::BeginPlay()
{
    Super::BeginPlay();

    BoatYawDegrees = GetActorRotation().Yaw;
    bFloatingInitialized = false;

    ConfigureBoatCollision();
    BuildBuoyancyPointsFromCollision();
    CacheOceanWaterBody();

    if (LeftOarMesh)
    {
        LeftOarMesh->SetCollisionEnabled(
            ECollisionEnabled::NoCollision);
        LeftOarMesh->SetGenerateOverlapEvents(false);
    }

    if (RightOarMesh)
    {
        RightOarMesh->SetCollisionEnabled(
            ECollisionEnabled::NoCollision);
        RightOarMesh->SetGenerateOverlapEvents(false);
    }

    if (CameraBoom)
    {
        CameraBoom->TargetArmLength = CameraArmLength;
        CameraBoom->SocketOffset = CameraSocketOffset;
    }

    if (LeftOarPivot)
    {
        InitialLeftOarRotation =
            LeftOarPivot->GetRelativeRotation();
    }

    if (RightOarPivot)
    {
        InitialRightOarRotation =
            RightOarPivot->GetRelativeRotation();
    }

    if (BoatMesh)
    {
        BoatMesh->SetCollisionEnabled(
            ECollisionEnabled::NoCollision);
        BoatMesh->SetGenerateOverlapEvents(false);
        BoatMesh->SetSimulatePhysics(false);
        BoatMesh->SetEnableGravity(false);

        if (bLogBoatSizeOnBeginPlay)
        {
            const FVector ActualBoatSize =
                BoatMesh->Bounds.BoxExtent * 2.0f;

            UE_LOG(
                LogTemp,
                Warning,
                TEXT(
                    "Boat Actual Size: "
                    "X=%.1f cm, Y=%.1f cm, Z=%.1f cm"),
                ActualBoatSize.X,
                ActualBoatSize.Y,
                ActualBoatSize.Z);
        }
    }

    /*
     * OceanがBeginPlay時点で取得できる場合は、
     * カメラに映る前に海面へ即座に合わせる。
     *
     * 取得がまだできない場合はTick側で最初の1回だけSnapする。
     */
    if (bEnableOceanFloating &&
        bSnapToOceanOnBeginPlay)
    {
        bFloatingInitialized =
            UpdateOceanFloating(0.0f, true);
    }

    const FName LeftSocketName(
        TEXT("OarPivot_L"));

    const FName RightSocketName(
        TEXT("OarPivot_R"));

    // =========================
    // 左オール支点
    // =========================

    if (
        BoatMesh &&
        LeftOarPivot &&
        BoatMesh->DoesSocketExist(
            LeftSocketName))
    {
        const bool bAttached =
            LeftOarPivot->AttachToComponent(
                BoatMesh,
                FAttachmentTransformRules::
                SnapToTargetNotIncludingScale,
                LeftSocketName);

        if (bAttached)
        {
            // ソケット基準の追加オフセットを消す
            LeftOarPivot->SetRelativeLocation(
                FVector::ZeroVector);

            LeftOarPivot->SetRelativeRotation(
                FRotator::ZeroRotator);

            LeftOarPivot->SetRelativeScale3D(
                FVector::OneVector);
        }
    }
    else
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT(
                "OarPivot_L socket was not found."));
    }

    // =========================
    // 右オール支点
    // =========================

    if (
        BoatMesh &&
        RightOarPivot &&
        BoatMesh->DoesSocketExist(
            RightSocketName))
    {
        const bool bAttached =
            RightOarPivot->AttachToComponent(
                BoatMesh,
                FAttachmentTransformRules::
                SnapToTargetNotIncludingScale,
                RightSocketName);

        if (bAttached)
        {
            RightOarPivot->SetRelativeLocation(
                FVector::ZeroVector);

            RightOarPivot->SetRelativeRotation(
                FRotator::ZeroRotator);

            RightOarPivot->SetRelativeScale3D(
                FVector::OneVector);
        }
    }
    else
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT(
                "OarPivot_R socket was not found."));
    }

    // ソケットへ接続した後の角度を初期角度として保存
    if (LeftOarPivot)
    {
        InitialLeftOarRotation =
            LeftOarPivot->GetRelativeRotation();
    }

    if (RightOarPivot)
    {
        InitialRightOarRotation =
            RightOarPivot->GetRelativeRotation();
    }

    if (LeftOarPivot)
    {
        InitialLeftOarQuat =
            LeftOarPivot
            ->GetRelativeTransform()
            .GetRotation();
    }

    if (RightOarPivot)
    {
        InitialRightOarQuat =
            RightOarPivot
            ->GetRelativeTransform()
            .GetRotation();
    }
}

void ABasePlayer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!FMath::IsFinite(DeltaTime) ||
        DeltaTime <= 0.0f)
    {
        return;
    }

    // =========================
    // Joy-Con input
    // =========================

    if (JoyConUdpReceiver)
    {
        LeftPaddleRawValue =
            JoyConUdpReceiver->GetLeftValue();

        RightPaddleRawValue =
            JoyConUdpReceiver->GetRightValue();

        LeftPaddlePower = ConvertToSignedPaddle(
            LeftPaddleRawValue,
            bInvertLeftPaddle);

        RightPaddlePower = ConvertToSignedPaddle(
            RightPaddleRawValue,
            bInvertRightPaddle);
    }
    else
    {
        LeftPaddleRawValue = 0.0f;
        RightPaddleRawValue = 0.0f;
        LeftPaddlePower = 0.0f;
        RightPaddlePower = 0.0f;
    }

    UpdateOarAnimation(DeltaTime);
    CalculatePaddleForces();
    UpdateBoatMovement(DeltaTime);

    // =========================
    // Ocean floating
    // =========================

    if (bEnableOceanFloating)
    {
        const bool bSnapThisFrame =
            bSnapToOceanOnBeginPlay &&
            !bFloatingInitialized;

        const bool bApplied =
            UpdateOceanFloating(
                DeltaTime,
                bSnapThisFrame);

        if (bApplied)
        {
            bFloatingInitialized = true;
        }
    }

    // =========================
    // Debug / camera
    // =========================

    if (bShowInputDebug)
    {
        ShowInputDebug();
    }

    if (FollowCamera)
    {
        const float SpeedRate = FMath::Clamp(
            FMath::Abs(CurrentForwardSpeed) /
            FMath::Max(MaxForwardSpeed, 1.0f),
            0.0f,
            1.0f);

        const float TargetFOV = FMath::Lerp(
            NormalFOV,
            HighSpeedFOV,
            SpeedRate);

        const float NewFOV = FMath::FInterpTo(
            FollowCamera->FieldOfView,
            TargetFOV,
            DeltaTime,
            FOVInterpSpeed);

        FollowCamera->SetFieldOfView(NewFOV);
    }
}

void ABasePlayer::ConfigureBoatCollision()
{
    if (!BoatCollision)
    {
        return;
    }

    const FVector SafeExtent(
        FMath::Max(BoatCollisionHalfExtent.X, 1.0f),
        FMath::Max(BoatCollisionHalfExtent.Y, 1.0f),
        FMath::Max(BoatCollisionHalfExtent.Z, 1.0f));

    BoatCollision->SetBoxExtent(
        SafeExtent,
        true);

    BoatCollision->SetSimulatePhysics(false);
    BoatCollision->SetEnableGravity(false);
    BoatCollision->SetCollisionProfileName(
        TEXT("Pawn"));
    BoatCollision->SetCollisionEnabled(
        ECollisionEnabled::QueryAndPhysics);
    BoatCollision->SetCollisionResponseToChannel(
        ECC_Camera,
        ECR_Ignore);
}

float ABasePlayer::ConvertToSignedPaddle(
    float RawValue,
    bool bInvertInput) const
{
    float AdjustedValue =
        RawValue * InputMultiplier;

    if (bInvertInput)
    {
        AdjustedValue *= -1.0f;
    }

    AdjustedValue =
        FMath::Clamp(AdjustedValue, -1.0f, 1.0f);

    const float AbsoluteValue =
        FMath::Abs(AdjustedValue);

    if (AbsoluteValue <= PaddleDeadZone)
    {
        return 0.0f;
    }

    const float SafeRange = FMath::Max(
        1.0f - PaddleDeadZone,
        KINDA_SMALL_NUMBER);

    const float Magnitude =
        (AbsoluteValue - PaddleDeadZone) /
        SafeRange;

    return
        FMath::Sign(AdjustedValue) *
        FMath::Clamp(Magnitude, 0.0f, 1.0f);
}

void ABasePlayer::CalculatePaddleForces()
{
    const float Left = LeftPaddlePower;
    const float Right = RightPaddlePower;

    // 左右の合計で前進・後退。
    CurrentThrustInput = FMath::Clamp(
        (Left + Right) * 0.5f,
        -1.0f,
        1.0f);

    // 左右差で旋回。
    // 左だけ引く -> 右旋回
    // 右だけ引く -> 左旋回
    CurrentTurnInput = FMath::Clamp(
        Left - Right,
        -1.0f,
        1.0f);

    if (bInvertTurnDirection)
    {
        CurrentTurnInput *= -1.0f;
    }
}

void ABasePlayer::UpdateBoatMovement(float DeltaTime)
{
    // =========================
    // Forward / reverse acceleration
    // =========================

    if (!FMath::IsNearlyZero(CurrentThrustInput))
    {
        const float BaseAcceleration =
            CurrentThrustInput > 0.0f
            ? ForwardAcceleration
            : ReverseAcceleration;

        const float BurstAcceleration =
            FMath::Abs(CurrentThrustInput) *
            PaddleBurstAcceleration;

        const float TotalAcceleration =
            BaseAcceleration + BurstAcceleration;

        CurrentForwardSpeed +=
            CurrentThrustInput *
            TotalAcceleration *
            DeltaTime;
    }

    // =========================
    // Turning acceleration
    // =========================

    if (!FMath::IsNearlyZero(CurrentTurnInput))
    {
        CurrentYawSpeed +=
            CurrentTurnInput *
            TurnAcceleration *
            DeltaTime;
    }

    // =========================
    // Water drag
    // =========================

    CurrentForwardSpeed *=
        FMath::Exp(-LinearWaterDrag * DeltaTime);

    CurrentYawSpeed *=
        FMath::Exp(-AngularWaterDrag * DeltaTime);

    // =========================
    // Speed limits
    // =========================

    CurrentForwardSpeed = FMath::Clamp(
        CurrentForwardSpeed,
        -MaxReverseSpeed,
        MaxForwardSpeed);

    CurrentYawSpeed = FMath::Clamp(
        CurrentYawSpeed,
        -MaxYawSpeed,
        MaxYawSpeed);

    // =========================
    // Rotation
    // =========================

    const float PreviousYaw = BoatYawDegrees;

    BoatYawDegrees +=
        CurrentYawSpeed * DeltaTime;

    BoatYawDegrees =
        FMath::UnwindDegrees(BoatYawDegrees);

    const FRotator CurrentRotation =
        GetActorRotation();

    const FRotator DesiredRotation(
        CurrentRotation.Pitch,
        BoatYawDegrees,
        CurrentRotation.Roll);

    if (!TrySetBoatRotation(DesiredRotation))
    {
        BoatYawDegrees = PreviousYaw;
        CurrentYawSpeed *= 0.20f;
    }

    // =========================
    // Move in the rotated forward direction
    // =========================

    FVector Forward = GetActorForwardVector();
    Forward.Z = 0.0f;

    if (!Forward.Normalize())
    {
        Forward = FVector::ForwardVector;
    }

    const FVector MoveDelta =
        Forward *
        CurrentForwardSpeed *
        DeltaTime;

    FHitResult MoveHit;

    AddActorWorldOffset(
        MoveDelta,
        true,
        &MoveHit,
        ETeleportType::None);

    if (MoveHit.bBlockingHit)
    {
        CurrentForwardSpeed *= 0.20f;
    }
}

bool ABasePlayer::TrySetBoatRotation(
    const FRotator& TargetRotation)
{
    if (!BoatCollision || !GetWorld())
    {
        return false;
    }

    const float SafeCollisionScale =
        FMath::Clamp(
            RotationCollisionScale,
            0.50f,
            1.00f);

    const FVector TestExtent =
        BoatCollision->GetScaledBoxExtent() *
        SafeCollisionScale;

    const FCollisionShape TestShape =
        FCollisionShape::MakeBox(TestExtent);

    FCollisionQueryParams QueryParams(
        SCENE_QUERY_STAT(BoatRotationCheck),
        false,
        this);

    QueryParams.AddIgnoredActor(this);

    const bool bWouldOverlap =
        GetWorld()->OverlapBlockingTestByChannel(
            BoatCollision->GetComponentLocation(),
            TargetRotation.Quaternion(),
            BoatCollision->GetCollisionObjectType(),
            TestShape,
            QueryParams);

    if (bWouldOverlap)
    {
        return false;
    }

    return SetActorRotation(
        TargetRotation,
        ETeleportType::TeleportPhysics);
}

void ABasePlayer::CacheOceanWaterBody()
{
    OceanWaterComponent = nullptr;

    UWorld* World = GetWorld();

    if (!World)
    {
        return;
    }

    // 現在のWorldに属するWaterBodyOcean Componentを検索する。
    for (TObjectIterator<UWaterBodyOceanComponent> It;
        It;
        ++It)
    {
        UWaterBodyOceanComponent* Candidate = *It;

        if (!IsValid(Candidate) ||
            Candidate->IsTemplate() ||
            Candidate->GetWorld() != World)
        {
            continue;
        }

        OceanWaterComponent = Candidate;

        UE_LOG(
            LogTemp,
            Log,
            TEXT(
                "BasePlayer: WaterBodyOcean found: %s"),
            *Candidate->GetName());

        return;
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT(
            "BasePlayer: WaterBodyOcean was not found yet."));
}

void ABasePlayer::BuildBuoyancyPointsFromCollision()
{
    BuoyancyPoints.Reset();

    if (!BoatCollision)
    {
        return;
    }

    /*
     * BoatMeshのBoundsやPivotは使わない。
     * RootのBoatCollisionを基準にするため、
     * MeshのScale・回転・Pivotに左右されない。
     */
    const FVector Extent =
        BoatCollision->GetUnscaledBoxExtent();

    const float PointX =
        Extent.X *
        FMath::Clamp(
            BuoyancyLengthRatio,
            0.10f,
            1.00f);

    const float PointY =
        Extent.Y *
        FMath::Clamp(
            BuoyancyWidthRatio,
            0.10f,
            1.00f);

    const float PointZ =
        BuoyancyCenterOffset.Z -
        Extent.Z *
        FMath::Clamp(
            BuoyancyVerticalRatio,
            0.00f,
            1.00f);

    const float CenterX =
        BuoyancyCenterOffset.X;

    const float CenterY =
        BuoyancyCenterOffset.Y;

    // 0: 前左
    // 1: 前右
    // 2: 後左
    // 3: 後右
    BuoyancyPoints =
    {
        FVector(
            CenterX + PointX,
            CenterY - PointY,
            PointZ),

        FVector(
            CenterX + PointX,
            CenterY + PointY,
            PointZ),

        FVector(
            CenterX - PointX,
            CenterY - PointY,
            PointZ),

        FVector(
            CenterX - PointX,
            CenterY + PointY,
            PointZ)
    };
}

bool ABasePlayer::SampleOceanSurface(
    const FVector& WorldLocation,
    FVector& OutSurfaceLocation,
    FVector& OutSurfaceNormal) const
{
    OutSurfaceLocation = WorldLocation;
    OutSurfaceNormal = FVector::UpVector;

    if (!IsValid(OceanWaterComponent))
    {
        return false;
    }

    const EWaterBodyQueryFlags QueryFlags =
        EWaterBodyQueryFlags::ComputeLocation |
        EWaterBodyQueryFlags::ComputeNormal |
        EWaterBodyQueryFlags::IncludeWaves;

    const auto QueryResultOrError =
        OceanWaterComponent
        ->TryQueryWaterInfoClosestToWorldLocation(
            WorldLocation,
            QueryFlags,
            TOptional<float>());

    if (!QueryResultOrError.HasValue())
    {
        return false;
    }

    const FWaterBodyQueryResult& QueryResult =
        QueryResultOrError.GetValue();

    if (QueryResult.IsInExclusionVolume())
    {
        return false;
    }

    OutSurfaceLocation =
        QueryResult.GetWaterSurfaceLocation();

    OutSurfaceNormal =
        QueryResult
        .GetWaterSurfaceNormal()
        .GetSafeNormal();

    if (OutSurfaceNormal.IsNearlyZero())
    {
        OutSurfaceNormal = FVector::UpVector;
    }

    return
        !OutSurfaceLocation.ContainsNaN() &&
        !OutSurfaceNormal.ContainsNaN();
}

bool ABasePlayer::UpdateOceanFloating(
    float DeltaTime,
    bool bSnapImmediately)
{
    if (!BoatCollision || !bEnableOceanFloating)
    {
        return false;
    }

    if (!bSnapImmediately &&
        (!FMath::IsFinite(DeltaTime) ||
            DeltaTime <= 0.0f))
    {
        return false;
    }

    if (BuoyancyPoints.Num() != 4)
    {
        BuildBuoyancyPointsFromCollision();

        if (BuoyancyPoints.Num() != 4)
        {
            return false;
        }
    }

    if (!IsValid(OceanWaterComponent))
    {
        CacheOceanWaterBody();

        if (!IsValid(OceanWaterComponent))
        {
            return false;
        }
    }

    const FTransform CollisionTransform =
        BoatCollision->GetComponentTransform();

    FVector BoatWorldPoints[4];
    FVector SurfacePoints[4];
    FVector SurfaceNormals[4];

    float WaterHeightSum = 0.0f;
    float BoatPointHeightSum = 0.0f;
    FVector NormalSum = FVector::ZeroVector;

    for (int32 Index = 0;
        Index < 4;
        ++Index)
    {
        BoatWorldPoints[Index] =
            CollisionTransform.TransformPosition(
                BuoyancyPoints[Index]);

        if (!SampleOceanSurface(
            BoatWorldPoints[Index],
            SurfacePoints[Index],
            SurfaceNormals[Index]))
        {
            return false;
        }

        WaterHeightSum +=
            SurfacePoints[Index].Z;

        BoatPointHeightSum +=
            BoatWorldPoints[Index].Z;

        NormalSum +=
            SurfaceNormals[Index];

        if (bDrawBuoyancyDebug && GetWorld())
        {
            // 赤: BoatCollision側の浮遊点
            DrawDebugSphere(
                GetWorld(),
                BoatWorldPoints[Index],
                8.0f,
                12,
                FColor::Red,
                false,
                0.0f,
                0,
                2.0f);

            // 水色: WaterBodyOceanの海面位置
            DrawDebugSphere(
                GetWorld(),
                SurfacePoints[Index],
                10.0f,
                12,
                FColor::Cyan,
                false,
                0.0f,
                0,
                2.0f);

            DrawDebugLine(
                GetWorld(),
                BoatWorldPoints[Index],
                SurfacePoints[Index],
                FColor::Green,
                false,
                0.0f,
                0,
                1.5f);
        }
    }

    // =========================
    // Height
    // =========================

    const float AverageWaterHeight =
        WaterHeightSum / 4.0f;

    const float AverageBoatPointHeight =
        BoatPointHeightSum / 4.0f;

    /*
     * ここが最終的な高さを決める。
     *
     * BoatFloatOffsetZを変更すると必ず最終Zが変わる。
     * +100なら船全体が100cm上、-100なら100cm下。
     */
    const float TargetPointHeight =
        AverageWaterHeight -
        DesiredSubmersion +
        BoatFloatOffsetZ;

    const float HeightError =
        TargetPointHeight -
        AverageBoatPointHeight;

    const FVector CurrentLocation =
        GetActorLocation();

    FVector TargetLocation =
        CurrentLocation;

    TargetLocation.Z += HeightError;

    FVector NewLocation =
        CurrentLocation;

    if (bSnapImmediately)
    {
        NewLocation.Z = TargetLocation.Z;
    }
    else
    {
        NewLocation.Z = FMath::FInterpTo(
            CurrentLocation.Z,
            TargetLocation.Z,
            DeltaTime,
            FloatHeightInterpSpeed);
    }

    FHitResult FloatHit;

    SetActorLocation(
        NewLocation,
        !bSnapImmediately,
        &FloatHit,
        bSnapImmediately
        ? ETeleportType::TeleportPhysics
        : ETeleportType::None);

    // =========================
    // Wave rotation
    // =========================

    const FVector FrontSurface =
        (SurfacePoints[0] +
            SurfacePoints[1]) *
        0.5f;

    const FVector RearSurface =
        (SurfacePoints[2] +
            SurfacePoints[3]) *
        0.5f;

    FVector SurfaceForward =
        FrontSurface - RearSurface;

    if (!SurfaceForward.Normalize())
    {
        SurfaceForward =
            GetActorForwardVector();

        SurfaceForward.Z = 0.0f;

        if (!SurfaceForward.Normalize())
        {
            SurfaceForward =
                FVector::ForwardVector;
        }
    }

    FVector AverageNormal =
        NormalSum.GetSafeNormal();

    if (AverageNormal.IsNearlyZero())
    {
        AverageNormal = FVector::UpVector;
    }

    FRotator TargetRotation =
        FRotationMatrix::MakeFromXZ(
            SurfaceForward,
            AverageNormal)
        .Rotator();

    // 操作で計算した船首方向を優先する。
    TargetRotation.Yaw = BoatYawDegrees;

    TargetRotation.Pitch = FMath::Clamp(
        TargetRotation.Pitch,
        -MaxFloatTiltAngle,
        MaxFloatTiltAngle);

    TargetRotation.Roll = FMath::Clamp(
        TargetRotation.Roll,
        -MaxFloatTiltAngle,
        MaxFloatTiltAngle);

    const FRotator NewRotation =
        bSnapImmediately
        ? TargetRotation
        : FMath::RInterpTo(
            GetActorRotation(),
            TargetRotation,
            DeltaTime,
            FloatRotationInterpSpeed);

    TrySetBoatRotation(NewRotation);

    return true;
}

void ABasePlayer::PawnClientRestart()
{
    Super::PawnClientRestart();

    if (CameraBoom)
    {
        CameraBoom->bUsePawnControlRotation = false;
        CameraBoom->bInheritPitch = false;
        CameraBoom->bInheritYaw = true;
        CameraBoom->bInheritRoll = false;
    }

    if (FollowCamera)
    {
        FollowCamera->bUsePawnControlRotation = false;
        FollowCamera->SetActive(true);
    }

    if (APlayerController* PlayerController =
        Cast<APlayerController>(GetController()))
    {
        PlayerController->SetViewTarget(this);
    }
}

void ABasePlayer::UpdateOarAnimation(
    float DeltaTime)
{
    if (
        !bEnableOarAnimation ||
        DeltaTime <= 0.0f)
    {
        return;
    }

    FVector RotationAxis =
        OarLocalRotationAxis.GetSafeNormal();

    if (RotationAxis.IsNearlyZero())
    {
        RotationAxis =
            FVector::UpVector;
    }

    const auto CalculateAngle =
        [this](float PaddlePower)
        {
            const float Power =
                FMath::Clamp(
                    PaddlePower,
                    -1.0f,
                    1.0f);

            if (Power >= 0.0f)
            {
                return OarPullAngle * Power;
            }

            return OarPushAngle *
                FMath::Abs(Power);
        };

    const auto ApplyOarRotation =
        [DeltaTime, RotationAxis, this](
            USceneComponent* Pivot,
            const FQuat& InitialRotation,
            float TargetAngle)
        {
            if (!Pivot)
            {
                return;
            }

            const FQuat RotationOffset(
                RotationAxis,
                FMath::DegreesToRadians(
                    TargetAngle));

            /*
             * 初期姿勢から、Pivotのローカル軸を
             * 基準に回転させる。
             */
            FQuat TargetRotation =
                InitialRotation *
                RotationOffset;

            TargetRotation.Normalize();

            const FQuat CurrentRotation =
                Pivot
                ->GetRelativeTransform()
                .GetRotation();

            const float InterpAlpha =
                1.0f -
                FMath::Exp(
                    -OarRotationInterpSpeed *
                    DeltaTime);

            FQuat NewRotation =
                FQuat::Slerp(
                    CurrentRotation,
                    TargetRotation,
                    InterpAlpha);

            NewRotation.Normalize();

            Pivot->SetRelativeRotation(
                NewRotation.Rotator());
        };

    const float LeftAngle =
        CalculateAngle(
            LeftPaddlePower);

    const float RightAngle =
        CalculateAngle(
            RightPaddlePower);

    /*
     * 左右を鏡像として動かす。
     * 入力値は反転せず、見た目の回転角だけ反転する。
     */
    ApplyOarRotation(
        LeftOarPivot,
        InitialLeftOarQuat,
        -LeftAngle);

    ApplyOarRotation(
        RightOarPivot,
        InitialRightOarQuat,
        RightAngle);
}
void ABasePlayer::ShowInputDebug() const
{
    if (!GEngine)
    {
        return;
    }

    const FString OceanState =
        IsValid(OceanWaterComponent)
        ? TEXT("FOUND")
        : TEXT("MISSING");

    const FString FloatState =
        bFloatingInitialized
        ? TEXT("READY")
        : TEXT("WAITING");

    const FString DebugText = FString::Printf(
        TEXT("CONTACT PADDLE MODE\n")
        TEXT("Left: %+0.3f  Right: %+0.3f\n")
        TEXT("Thrust: %+0.3f  Turn: %+0.3f\n")
        TEXT("ForwardSpeed: %+0.1f  YawSpeed: %+0.1f\n")
        TEXT("Location Z: %.1f  FloatOffsetZ: %.1f\n")
        TEXT("Ocean: %s  Float: %s  Points: %d\n")
        TEXT("Root: %s"),
        LeftPaddlePower,
        RightPaddlePower,
        CurrentThrustInput,
        CurrentTurnInput,
        CurrentForwardSpeed,
        CurrentYawSpeed,
        GetActorLocation().Z,
        BoatFloatOffsetZ,
        *OceanState,
        *FloatState,
        BuoyancyPoints.Num(),
        *GetNameSafe(GetRootComponent()));

    GEngine->AddOnScreenDebugMessage(
        7002,
        0.0f,
        FColor::Cyan,
        DebugText);
}
