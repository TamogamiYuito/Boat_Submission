#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WaterRiseTrigger.generated.h"

class UBoxComponent;
class UPrimitiveComponent;
class UWaterSubsystem;
class UWaterBodyOceanComponent;
class UWaterMeshComponent;

class AWaterBodyOcean;
class AWaterZone;

UCLASS()
class BOAT_API AWaterRiseTrigger : public AActor
{
    GENERATED_BODY()

public:
    AWaterRiseTrigger();

    virtual void Tick(float DeltaTime) override;

protected:
    virtual void BeginPlay() override;

    // =========================
    // Components
    // =========================

    // 船が入ったことを検知する範囲
    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadOnly,
        Category = "Water Rise")
    TObjectPtr<UBoxComponent> TriggerBox;

    // =========================
    // Water rise settings
    // =========================

    // Oceanを何cm上昇させるか
    // 500 = 5m
    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Water Rise",
        meta = (ClampMin = "0.0"))
    float RiseHeight = 500.0f;

    // 上昇にかける時間
    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Water Rise",
        meta = (ClampMin = "0.01"))
    float RiseDuration = 8.0f;

    // 一度だけ作動する
    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Water Rise")
    bool bTriggerOnce = true;

    // 上昇開始と終了を滑らかにする
    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Water Rise")
    bool bUseSmoothInterpolation = true;

    /*
     * 上昇終了後にWater Info Textureの
     * 更新だけを要求する。
     *
     * 更新時に見た目が一瞬消える場合はオフにする。
     */
    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Water Rise|Rendering")
    bool bRefreshWaterInfoAtEnd = false;

    // デバッグ情報をゲーム画面に表示
    UPROPERTY(
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Water Rise|Debug")
    bool bShowDebug = true;

    // =========================
    // Overlap
    // =========================

    UFUNCTION()
    void OnTriggerBeginOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComponent,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult);

private:
    // =========================
    // Water references
    // =========================

    UPROPERTY(Transient)
    TObjectPtr<UWaterSubsystem> WaterSubsystem;

    UPROPERTY(Transient)
    TObjectPtr<UWaterBodyOceanComponent> OceanComponent;

    UPROPERTY(Transient)
    TObjectPtr<AWaterBodyOcean> OceanActor;

    UPROPERTY(Transient)
    TObjectPtr<AWaterZone> WaterZoneActor;

    UPROPERTY(Transient)
    TObjectPtr<UWaterMeshComponent> WaterMeshComponent;

    // =========================
    // Rise state
    // =========================

    FVector StartOceanLocation =
        FVector::ZeroVector;

    FVector TargetOceanLocation =
        FVector::ZeroVector;

    float ElapsedRiseTime = 0.0f;

    bool bIsRising = false;
    bool bHasTriggered = false;

    // =========================
    // Internal functions
    // =========================

    bool CacheWaterActors();

    void StartWaterRise();

    void FinishWaterRise();

    // Water Meshを更新する間隔。
    // 毎フレーム再構築すると重いため、0.05秒ごとに更新する。
    UPROPERTY(
        EditAnywhere,
        Category = "Water Rise|Rendering",
        meta = (ClampMin = "0.02"))
    float WaterVisualRefreshInterval = 0.05f;

    float WaterVisualRefreshElapsed = 0.0f;

    void RefreshOceanVisual();
};