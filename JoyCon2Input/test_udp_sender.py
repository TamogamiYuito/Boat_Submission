import socket
import time

HOST = "127.0.0.1"
PORT = 7777
SEND_HZ = 60.0

# (label, duration seconds, left value, right value)
PHASES = [
    ("NEUTRAL", 1.0, 0.0, 0.0),
    ("LEFT +0.75", 2.0, 0.75, 0.0),
    ("NEUTRAL", 1.0, 0.0, 0.0),
    ("RIGHT -0.75", 2.0, 0.0, -0.75),
    ("NEUTRAL", 1.0, 0.0, 0.0),
    ("BOTH +0.50", 2.0, 0.50, 0.50),
    ("NEUTRAL", 1.0, 0.0, 0.0),
]


def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    interval = 1.0 / SEND_HZ

    print(f"Sending test packets to {HOST}:{PORT}")
    print("Start Play in UE5 before running this script.")
    print()

    for label, duration, left, right in PHASES:
        print(f"{label}: left={left:+.2f}, right={right:+.2f}")
        end_time = time.perf_counter() + duration

        while time.perf_counter() < end_time:
            packet = f"{left:.6f},{right:.6f}".encode("ascii")
            sock.sendto(packet, (HOST, PORT))
            time.sleep(interval)

    print("UDP test finished.")


if __name__ == "__main__":
    main()
