#include "JoyConUdpReceiverComponent.h"

#include "Common/UdpSocketBuilder.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "SocketSubsystem.h"
#include "Sockets.h"

#include "Engine/Engine.h"
#include "Engine/World.h"

UJoyConUdpReceiverComponent::UJoyConUdpReceiverComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UJoyConUdpReceiverComponent::BeginPlay()
{
    Super::BeginPlay();

    if (!StartReceiver())
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("JoyCon UDP: Failed to bind UDP port %d."),
            ListenPort);
    }
}

void UJoyConUdpReceiverComponent::EndPlay(
    const EEndPlayReason::Type EndPlayReason)
{
    StopReceiver();
    Super::EndPlay(EndPlayReason);
}

void UJoyConUdpReceiverComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    ReadPendingPackets();

    const UWorld* World = GetWorld();
    const double Now = World ? World->GetTimeSeconds() : 0.0;

    if (
        LastPacketTimeSeconds < 0.0
        || Now - LastPacketTimeSeconds > PacketTimeoutSeconds)
    {
        LeftValue = 0.0f;
        RightValue = 0.0f;
        bIsReceiving = false;
    }

    if (bPrintDebug && GEngine)
    {
        const FString Message = FString::Printf(
            TEXT("JoyCon UDP: %s\nLeft: %.3f  Right: %.3f"),
            bIsReceiving ? TEXT("RECEIVING") : TEXT("WAITING"),
            LeftValue,
            RightValue);

        GEngine->AddOnScreenDebugMessage(
            7001,
            0.0f,
            FColor::Yellow,
            Message);
    }
}

bool UJoyConUdpReceiverComponent::StartReceiver()
{
    StopReceiver();

    ReceiverSocket =
        FUdpSocketBuilder(TEXT("JoyConUdpReceiver"))
        .AsNonBlocking()
        .AsReusable()
        .BoundToAddress(FIPv4Address::Any)
        .BoundToPort(ListenPort)
        .WithReceiveBufferSize(2 * 1024 * 1024);

    return ReceiverSocket != nullptr;
}

void UJoyConUdpReceiverComponent::StopReceiver()
{
    if (!ReceiverSocket)
    {
        return;
    }

    ReceiverSocket->Close();

    if (ISocketSubsystem* SocketSubsystem =
            ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM))
    {
        SocketSubsystem->DestroySocket(ReceiverSocket);
    }

    ReceiverSocket = nullptr;
}

void UJoyConUdpReceiverComponent::ReadPendingPackets()
{
    if (!ReceiverSocket)
    {
        return;
    }

    uint32 PendingDataSize = 0;

    while (ReceiverSocket->HasPendingData(PendingDataSize))
    {
        const int32 BufferSize =
            static_cast<int32>(FMath::Min<uint32>(PendingDataSize, 1024u));

        TArray<uint8> Buffer;
        Buffer.SetNumUninitialized(BufferSize + 1);

        int32 BytesRead = 0;
        TSharedRef<FInternetAddr> Sender =
            ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)
            ->CreateInternetAddr();

        const bool bReceived = ReceiverSocket->RecvFrom(
            Buffer.GetData(),
            BufferSize,
            BytesRead,
            *Sender);

        if (!bReceived || BytesRead <= 0)
        {
            continue;
        }

        Buffer[BytesRead] = 0;

        const FString Packet =
            UTF8_TO_TCHAR(
                reinterpret_cast<const char*>(Buffer.GetData()));

        if (ParsePacket(Packet))
        {
            const UWorld* World = GetWorld();
            LastPacketTimeSeconds =
                World ? World->GetTimeSeconds() : 0.0;
            bIsReceiving = true;
        }
    }
}

bool UJoyConUdpReceiverComponent::ParsePacket(const FString& Packet)
{
    TArray<FString> Fields;
    Packet.TrimStartAndEnd().ParseIntoArray(
        Fields,
        TEXT(","),
        true);

    if (Fields.Num() != 2)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("JoyCon UDP: Invalid packet: %s"),
            *Packet);
        return false;
    }

    const float ParsedLeft = FCString::Atof(*Fields[0]);
    const float ParsedRight = FCString::Atof(*Fields[1]);

    LeftValue = FMath::Clamp(ParsedLeft, -1.0f, 1.0f);
    RightValue = FMath::Clamp(ParsedRight, -1.0f, 1.0f);

    return true;
}
