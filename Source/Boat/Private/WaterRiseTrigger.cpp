#include "WaterRiseTrigger.h"

#include "Boat/BasePlayer.h"

#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"

#include "Engine/Engine.h"
#include "Engine/World.h"

#include "WaterBodyComponent.h"
#include "WaterBodyOceanActor.h"
#include "WaterBodyOceanComponent.h"
#include "WaterMeshComponent.h"
#include "WaterSubsystem.h"
#include "WaterZoneActor.h"

AWaterRiseTrigger::AWaterRiseTrigger()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;

    // =========================
    // Trigger
    // =========================

    TriggerBox =
        CreateDefaultSubobject<UBoxComponent>(
            TEXT("TriggerBox"));

    SetRootComponent(TriggerBox);

    // 全体サイズ：
    // 600 × 600 × 400cm
    TriggerBox->InitBoxExtent(
        FVector(
            300.0f,
            300.0f,
            200.0f));

    TriggerBox->SetMobility(
        EComponentMobility::Movable);

    /*
     * 船を物理的には止めず、
     * 範囲への侵入だけ検知する。
     */
    TriggerBox->SetCollisionEnabled(
        ECollisionEnabled::QueryOnly);

    TriggerBox->SetCollisionObjectType(
        ECC_WorldDynamic);

    TriggerBox->SetCollisionResponseToAllChannels(
        ECR_Ignore);

    /*
     * BasePlayerのBoatCollisionが
     * ECC_Pawnであることを前提とする。
     */
    TriggerBox->SetCollisionResponseToChannel(
        ECC_Pawn,
        ECR_Overlap);

    TriggerBox->SetGenerateOverlapEvents(true);

    TriggerBox->OnComponentBeginOverlap.AddDynamic(
        this,
        &AWaterRiseTrigger::OnTriggerBeginOverlap);
}

void AWaterRiseTrigger::BeginPlay()
{
    Super::BeginPlay();

    if (!CacheWaterActors())
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                8200,
                8.0f,
                FColor::Red,
                TEXT(
                    "WaterRiseTrigger initialization failed. "
                    "Check Output Log."));
        }

        return;
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT(
            "WaterRiseTrigger READY | "
            "Ocean=%s | OceanZ=%.1f | "
            "WaterZone=%s | ZoneZ=%.1f"),
        *GetNameSafe(OceanActor),
        OceanActor->GetActorLocation().Z,
        *GetNameSafe(WaterZoneActor),
        WaterZoneActor->GetActorLocation().Z);

    if (bShowDebug && GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            8200,
            5.0f,
            FColor::Green,
            FString::Printf(
                TEXT(
                    "WaterRiseTrigger READY\n"
                    "Ocean Z: %.1f\n"
                    "WaterZone Z: %.1f"),
                OceanActor->GetActorLocation().Z,
                WaterZoneActor->GetActorLocation().Z));
    }
}

bool AWaterRiseTrigger::CacheWaterActors()
{
    UWorld* World = GetWorld();

    if (!IsValid(World))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT(
                "WaterRiseTrigger: "
                "World was not found."));

        return false;
    }

    // =========================
    // WaterSubsystem
    // =========================

    WaterSubsystem =
        World->GetSubsystem<UWaterSubsystem>();

    if (!IsValid(WaterSubsystem))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT(
                "WaterRiseTrigger: "
                "WaterSubsystem was not found."));

        return false;
    }

    // =========================
    // Ocean component
    // =========================

    const TWeakObjectPtr<UWaterBodyComponent>
        OceanBodyWeak =
        WaterSubsystem->GetOceanBodyComponent();

    UWaterBodyComponent* OceanBodyComponent =
        OceanBodyWeak.Get();

    if (!IsValid(OceanBodyComponent))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT(
                "WaterRiseTrigger: "
                "Ocean body component was not found."));

        return false;
    }

    OceanComponent =
        Cast<UWaterBodyOceanComponent>(
            OceanBodyComponent);

    if (!IsValid(OceanComponent))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT(
                "WaterRiseTrigger: "
                "Ocean body is not "
                "UWaterBodyOceanComponent."));

        return false;
    }

    // =========================
    // Ocean actor
    // =========================

    OceanActor =
        Cast<AWaterBodyOcean>(
            OceanComponent->GetOwner());

    if (!IsValid(OceanActor))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT(
                "WaterRiseTrigger: "
                "WaterBodyOcean actor was not found."));

        return false;
    }

    // =========================
    // WaterZone
    // =========================

    WaterZoneActor =
        OceanComponent->GetWaterZone();

    if (!IsValid(WaterZoneActor))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT(
                "WaterRiseTrigger: "
                "WaterZone was not found."));

        return false;
    }

    // =========================
    // WaterMeshComponent
    // =========================

    WaterMeshComponent =
        WaterZoneActor->GetWaterMeshComponent();

    if (!IsValid(WaterMeshComponent))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT(
                "WaterRiseTrigger: "
                "WaterMeshComponent was not found."));

        return false;
    }

    /*
     * WaterMeshComponentがWaterZoneの移動を
     * 継承するようにする。
     */
    WaterMeshComponent->SetAbsolute(
        false,
        false,
        false);

    // =========================
    // Mobility
    // =========================

    if (USceneComponent* OceanRoot =
        OceanActor->GetRootComponent())
    {
        OceanRoot->SetMobility(
            EComponentMobility::Movable);
    }
    else
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT(
                "WaterRiseTrigger: "
                "Ocean root component was not found."));

        return false;
    }

    OceanComponent->SetMobility(
        EComponentMobility::Movable);

    if (USceneComponent* WaterZoneRoot =
        WaterZoneActor->GetRootComponent())
    {
        WaterZoneRoot->SetMobility(
            EComponentMobility::Movable);
    }
    else
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT(
                "WaterRiseTrigger: "
                "WaterZone root component was not found."));

        return false;
    }

    WaterMeshComponent->SetMobility(
        EComponentMobility::Movable);

    return true;
}

void AWaterRiseTrigger::OnTriggerBeginOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComponent,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (!IsValid(OtherActor))
    {
        return;
    }

    // プレイヤーの船以外では作動しない
    ABasePlayer* BoatPlayer =
        Cast<ABasePlayer>(OtherActor);

    if (!IsValid(BoatPlayer))
    {
        return;
    }

    if (bIsRising)
    {
        return;
    }

    if (bTriggerOnce && bHasTriggered)
    {
        return;
    }

    if (bShowDebug && GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            8201,
            3.0f,
            FColor::Yellow,
            TEXT(
                "Water rise trigger activated."));
    }

    StartWaterRise();
}

void AWaterRiseTrigger::StartWaterRise()
{
    if (
        !IsValid(OceanActor) ||
        !IsValid(OceanComponent) ||
        !IsValid(WaterZoneActor))
    {
        if (!CacheWaterActors())
        {
            UE_LOG(
                LogTemp,
                Error,
                TEXT(
                    "WaterRiseTrigger: "
                    "Water actors are not ready."));

            return;
        }
    }

    StartOceanLocation =
        OceanActor->GetActorLocation();

    TargetOceanLocation =
        StartOceanLocation;

    TargetOceanLocation.Z +=
        RiseHeight;

    ElapsedRiseTime = 0.0f;
    WaterVisualRefreshElapsed = 0.0f;

    bIsRising = true;
    bHasTriggered = true;

    SetActorTickEnabled(true);

    UE_LOG(
        LogTemp,
        Warning,
        TEXT(
            "Ocean rise started | "
            "Ocean Z: %.1f -> %.1f"),
        StartOceanLocation.Z,
        TargetOceanLocation.Z);
}

void AWaterRiseTrigger::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (
        !bIsRising ||
        !IsValid(OceanActor) ||
        !IsValid(OceanComponent) ||
        !IsValid(WaterZoneActor) ||
        !FMath::IsFinite(DeltaTime) ||
        DeltaTime <= 0.0f)
    {
        return;
    }

    ElapsedRiseTime += DeltaTime;
    WaterVisualRefreshElapsed += DeltaTime;

    const float SafeDuration =
        FMath::Max(
            RiseDuration,
            KINDA_SMALL_NUMBER);

    const float Alpha =
        FMath::Clamp(
            ElapsedRiseTime / SafeDuration,
            0.0f,
            1.0f);

    float MoveAlpha = Alpha;

    if (bUseSmoothInterpolation)
    {
        MoveAlpha =
            FMath::InterpEaseInOut(
                0.0f,
                1.0f,
                Alpha,
                2.0f);
    }

    // =========================
    // WaterBodyOceanだけを移動
    // =========================

    const FVector NewOceanLocation =
        FMath::Lerp(
            StartOceanLocation,
            TargetOceanLocation,
            MoveAlpha);

    const bool bOceanMoved =
        OceanActor->SetActorLocation(
            NewOceanLocation,
            false,
            nullptr,
            ETeleportType::TeleportPhysics);

    /*
     * WaterZoneは移動させない。
     *
     * WaterZoneは描画範囲を管理し、
     * 水面高度はWaterBodyのRenderDataから取得する。
     */

    const float SafeRefreshInterval =
        FMath::Max(
            WaterVisualRefreshInterval,
            0.02f);

    if (
        WaterVisualRefreshElapsed >=
        SafeRefreshInterval &&
        Alpha < 1.0f)
    {
        WaterVisualRefreshElapsed = 0.0f;

        RefreshOceanVisual();
    }

    if (bShowDebug && GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            8202,
            0.0f,
            bOceanMoved
            ? FColor::Cyan
            : FColor::Red,
            FString::Printf(
                TEXT(
                    "OCEAN RISING\n"
                    "Ocean Z: %.1f / %.1f\n"
                    "Moved: %s\n"
                    "Refresh: %.2f sec\n"
                    "Progress: %.0f%%"),
                OceanActor
                ->GetActorLocation().Z,
                TargetOceanLocation.Z,
                bOceanMoved
                ? TEXT("YES")
                : TEXT("NO"),
                SafeRefreshInterval,
                Alpha * 100.0f));
    }

    if (Alpha >= 1.0f)
    {
        FinishWaterRise();
    }
}

void AWaterRiseTrigger::FinishWaterRise()
{
    if (
        !IsValid(OceanActor) ||
        !IsValid(OceanComponent) ||
        !IsValid(WaterZoneActor))
    {
        bIsRising = false;

        SetActorTickEnabled(false);

        return;
    }

    // 最終位置を確実に設定
    OceanActor->SetActorLocation(
        TargetOceanLocation,
        false,
        nullptr,
        ETeleportType::TeleportPhysics);

    /*
     * 最後にWaterBodyの描画データと
     * Water Meshを確実に更新する。
     */
    RefreshOceanVisual();

    bIsRising = false;

    SetActorTickEnabled(false);

    UE_LOG(
        LogTemp,
        Warning,
        TEXT(
            "Ocean rise completed | "
            "Ocean Z: %.1f"),
        OceanActor->GetActorLocation().Z);

    if (bShowDebug && GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            8203,
            5.0f,
            FColor::Green,
            TEXT("Ocean rise completed."));
    }
}

void AWaterRiseTrigger::RefreshOceanVisual()
{
    if (
        !IsValid(OceanActor) ||
        !IsValid(OceanComponent) ||
        !IsValid(WaterZoneActor))
    {
        return;
    }

    /*
     * ActorとComponentのTransformを
     * Render Thread側へ反映する。
     */
    OceanActor->UpdateComponentTransforms();
    OceanActor->MarkComponentsRenderStateDirty();

    /*
     * WaterBodyの位置が変更されたことを通知する。
     *
     * これによりWaterBodyRenderData内の
     * SurfaceBaseHeightなどが更新される。
     */
    FOnWaterBodyChangedParams ChangedParams;

    ChangedParams.bShapeOrPositionChanged = true;
    ChangedParams.bUserTriggered = true;
    ChangedParams.bWeightmapSettingsChanged = false;

    OceanComponent->UpdateAll(
        ChangedParams);

    /*
     * WaterZoneが所有するWater Meshと
     * Water Info Textureを更新する。
     */
    const EWaterZoneRebuildFlags RebuildFlags =
        EWaterZoneRebuildFlags::UpdateWaterMesh |
        EWaterZoneRebuildFlags::UpdateWaterInfoTexture;

    WaterZoneActor->MarkForRebuild(
        RebuildFlags,
        this);

    WaterZoneActor->MarkComponentsRenderStateDirty();
}