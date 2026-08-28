#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JoyConUdpReceiverComponent.generated.h"

class FSocket;

/*
 * Receives ASCII UDP packets in this format:
 *
 *     leftValue,rightValue
 *
 * Example:
 *     0.250000,-0.500000
 */
UCLASS(ClassGroup=(Input), meta=(BlueprintSpawnableComponent))
class BOAT_API UJoyConUdpReceiverComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UJoyConUdpReceiverComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Joy-Con UDP")
    int32 ListenPort = 7777;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Joy-Con UDP")
    float PacketTimeoutSeconds = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Joy-Con UDP")
    bool bPrintDebug = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Joy-Con UDP")
    float LeftValue = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Joy-Con UDP")
    float RightValue = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Joy-Con UDP")
    bool bIsReceiving = false;

    UFUNCTION(BlueprintPure, Category="Joy-Con UDP")
    float GetLeftValue() const { return LeftValue; }

    UFUNCTION(BlueprintPure, Category="Joy-Con UDP")
    float GetRightValue() const { return RightValue; }

private:
    FSocket* ReceiverSocket = nullptr;
    double LastPacketTimeSeconds = -1.0;

    bool StartReceiver();
    void StopReceiver();
    void ReadPendingPackets();
    bool ParsePacket(const FString& Packet);
};
