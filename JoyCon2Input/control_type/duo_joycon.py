from controllers.JoyconL import JoyConLeft
from controllers.JoyconR import JoyConRight
from dsu_server import controller_update
from config import Config

import pyvjoy
import socket
import time


# ---------------------------------------------------------------------------
# Joy-Con / vJoy initialization
# ---------------------------------------------------------------------------

joyconLeft = JoyConLeft()
joyconRight = JoyConRight()

config = Config().getConfig()
vjoy = pyvjoy.VJoyDevice(1)

# UDP output for Unreal Engine.
UDP_HOST = "127.0.0.1"
UDP_PORT = 7777
UDP_INPUT_GAIN = 0.02
udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)


# ---------------------------------------------------------------------------
# Dual optical-mouse sensor -> separate vJoy axes
#
# Left Joy-Con 2 mouse Y  -> vJoy Z axis
# Right Joy-Con 2 mouse Y -> vJoy RZ axis
#
# vJoy uses approximately 1..32768, with the center around 16384.
# UE RawInput normally receives this as approximately 0.0..1.0.
# ---------------------------------------------------------------------------

VJOY_MIN = 1
VJOY_CENTER = 16384
VJOY_MAX = 32768

# Increase this when the paddle input is too weak.
# Decrease it when the axis reaches its limit too easily.
PADDLE_GAIN = 12

# Small sensor changes at or below this value are ignored.
PADDLE_DEADZONE = 1

# Keep a non-zero sensor movement on the vJoy axis long enough for UE5
# to sample it during a game frame. The Joy-Con reports much faster than
# a typical 60 FPS game loop, so returning to center immediately can make
# UE5 miss the movement.
PADDLE_HOLD_SECONDS = 0.12

# Set to False after confirming that both sensors work.
DEBUG_PRINT = True
DEBUG_INTERVAL_SECONDS = 0.05


mouse_state = {
    "Left": {
        "x": 0,
        "y": 0,
        "initialized": False,
        "surface": False,
        "delta_y": 0,
        "held_delta_y": 0,
        "last_motion_time": 0.0,
        "axis": VJOY_CENTER,
    },
    "Right": {
        "x": 0,
        "y": 0,
        "initialized": False,
        "surface": False,
        "delta_y": 0,
        "held_delta_y": 0,
        "last_motion_time": 0.0,
        "axis": VJOY_CENTER,
    },
}

last_debug_time = 0.0
last_udp_left = 0.0
last_udp_right = 0.0


Controls = {
    "Left": {
        "ZL": 1,
        "L": 3,
        "L3": 19,
        "Right": 9,
        "Down": 10,
        "Up": 11,
        "Left": 12,
        "Minus": 14,
        "SLL": 17,
        "SRL": 18,
        "Capture": 22,
    },
    "Right": {
        "ZR": 2,
        "R": 4,
        "R3": 20,
        "A": 5,
        "B": 6,
        "X": 7,
        "Y": 8,
        "Plus": 13,
        "SRR": 15,
        "SLR": 16,
        "Home": 21,
        "GameChat": 23,
    },
}


def clamp(value, minimum, maximum):
    return max(minimum, min(value, maximum))


def wrapped_delta_16(current, previous):
    """
    Calculate a signed delta while handling a signed/unsigned 16-bit wrap.
    The original Joy2Win mouse implementation uses the same normalization.
    """
    return (int(current) - int(previous) + 32768) % 65536 - 32768


def update_paddle_axis(controller_side, joycon):
    state = mouse_state[controller_side]
    now = time.monotonic()

    # Joy2Win treats distance 00/01 as the sensor being close to a surface.
    is_on_surface = joycon.mouse["distance"] in ("00", "01")
    state["surface"] = is_on_surface

    if not is_on_surface:
        # Reinitialize next time the Joy-Con touches the desk so an old
        # coordinate does not produce one large false movement.
        state["initialized"] = False
        state["delta_y"] = 0
        state["held_delta_y"] = 0
        state["last_motion_time"] = 0.0
        axis_value = VJOY_CENTER
    else:
        current_x = int(joycon.mouse["X"])
        current_y = int(joycon.mouse["Y"])

        if not state["initialized"]:
            state["x"] = current_x
            state["y"] = current_y
            state["initialized"] = True
            state["delta_y"] = 0
            state["held_delta_y"] = 0
            state["last_motion_time"] = 0.0
            axis_value = VJOY_CENTER
        else:
            delta_y = wrapped_delta_16(current_y, state["y"])

            state["x"] = current_x
            state["y"] = current_y

            if abs(delta_y) <= PADDLE_DEADZONE:
                delta_y = 0

            # A non-zero movement is held briefly. Without this, the axis can
            # return to center between two UE5 frames and the game sees 0.
            if delta_y != 0:
                state["held_delta_y"] = delta_y
                state["last_motion_time"] = now
            elif (
                state["last_motion_time"] <= 0.0
                or now - state["last_motion_time"] >= PADDLE_HOLD_SECONDS
            ):
                state["held_delta_y"] = 0

            state["delta_y"] = state["held_delta_y"]

            axis_value = clamp(
                VJOY_CENTER + state["held_delta_y"] * PADDLE_GAIN,
                VJOY_MIN,
                VJOY_MAX,
            )

    state["axis"] = int(axis_value)

    if controller_side == "Left":
        vjoy.set_axis(pyvjoy.HID_USAGE_Z, state["axis"])
    else:
        vjoy.set_axis(pyvjoy.HID_USAGE_RZ, state["axis"])



def get_udp_value(controller_side, now):
    """
    Return the signed optical-sensor movement as -1.0..1.0.
    The value is held briefly so a 60 FPS game does not miss a fast report.
    """
    state = mouse_state[controller_side]

    if (
        not state["surface"]
        or state["last_motion_time"] <= 0.0
        or now - state["last_motion_time"] >= PADDLE_HOLD_SECONDS
    ):
        return 0.0

    return float(
        clamp(
            state["held_delta_y"] * UDP_INPUT_GAIN,
            -1.0,
            1.0,
        )
    )


def send_udp_values():
    now = time.monotonic()
    left_value = get_udp_value("Left", now)
    right_value = get_udp_value("Right", now)

    packet = f"{left_value:.6f},{right_value:.6f}"
    udp_socket.sendto(packet.encode("ascii"), (UDP_HOST, UDP_PORT))

    return left_value, right_value


def print_debug_values():
    global last_debug_time

    if not DEBUG_PRINT:
        return

    now = time.monotonic()
    if now - last_debug_time < DEBUG_INTERVAL_SECONDS:
        return

    last_debug_time = now

    left = mouse_state["Left"]
    right = mouse_state["Right"]

    print(
        "\r"
        f"Left  surface={'ON ' if left['surface'] else 'OFF'} "
        f"dY={left['delta_y']:+6d} Z={left['axis']:5d} | "
        f"Right surface={'ON ' if right['surface'] else 'OFF'} "
        f"dY={right['delta_y']:+6d} RZ={right['axis']:5d} | "
        f"UDP L={last_udp_left:+.3f} R={last_udp_right:+.3f}",
        end="",
        flush=True,
    )


async def update(controllerSide, joycon):
    # Keep all existing Joy-Con buttons available.
    for side in ("Left", "Right"):
        controller = joyconLeft if side == "Left" else joyconRight

        for button_name, button_number in Controls[side].items():
            pressed = controller.buttons.get(button_name, False)
            vjoy.set_button(button_number, pressed)

    # Keep the original analog-stick mappings.
    vjoy.set_axis(pyvjoy.HID_USAGE_X, joyconLeft.analog_stick["X"])
    vjoy.set_axis(pyvjoy.HID_USAGE_Y, joyconLeft.analog_stick["Y"])
    vjoy.set_axis(pyvjoy.HID_USAGE_RX, joyconRight.analog_stick["X"])
    vjoy.set_axis(pyvjoy.HID_USAGE_RY, joyconRight.analog_stick["Y"])

    # Send each optical mouse sensor to its own vJoy axis.
    update_paddle_axis(controllerSide, joycon)

    # Send both left/right optical sensor values directly to UE5 over UDP.
    global last_udp_left, last_udp_right
    last_udp_left, last_udp_right = send_udp_values()

    if controllerSide == "Right" and config["enable_dsu"] is True:
        await controller_update(
            joyconRight.motionTimestamp,
            joyconRight.accelerometer,
            joyconRight.gyroscope,
        )

    print_debug_values()


async def notify_duo_joycons(client, side, data):
    if side == "Left":
        await joyconLeft.update(data)
        await update(side, joyconLeft)
    elif side == "Right":
        await joyconRight.update(data)
        await update(side, joyconRight)
    else:
        print("Unknown controller side.")

    return client
