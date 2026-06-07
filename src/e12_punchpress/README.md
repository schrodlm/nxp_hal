# E12 Punchpress

In this exercise you are supposed to implement a controller for a punchpress
machine. You do not get a prepared `main.c`. Instead, you are to implement it
on your own both the necessary HAL and application logic.

In this exercise, the controller runs on the S32K118 microcontroller. The punchpress
is simulated on the attached STM32F4 DISCOVERY board. 

The state of the punchpress is visualized through client connected to the simulator
running on the DISCOVERY board via micro-usb cable.

The punchpress is a two-axis machine with two quadrature encoders and four
border switches. Your controller talks to the punchpress through:

- PWM motor outputs
- encoder inputs
- border-switch inputs
- CAN communication

## Goal

Implement a controller that handles the three punchpress commands over CAN:

- `HOME`
- `MOVE_TO`
- `PUNCH_AT`

and executes them in such a way that the punchpress does not enter FAIL mode.
This means not moving the head while the punch is down and not moving the
head outside the machine area.

Conceptually the setup looks as follows:

```text
host PC running DISCOCAN CLI / TUI / web interface
        |
        | USB (micro-USB port on the DISCOVERY board)
        v
   DISCOCAN board (DISCOVERY board with our custom firmware)
   - sends command frames over CAN
   - simulates punchpress mechanics
   - generates encoder/border signals
        ^
        | CAN + signal-level interface
        v
   your S32 controller
   - reads encoders and borders
   - drives X/Y motors by PWM
   - sends punch and busy status by CAN
```

The host sends punchpress commands on CAN ID `0x202`. Your controller
reports its busy state on `0x203`. The simulator sends status on `0x200`.
Your controller requests punch down/up on `0x201`.

For more host-side detail, see
[`discocan/client`](https://github.com/d3scomp/discocan/tree/main/client).

## S32-side signal mapping

Use these S32 pins:

### PWM motor outputs

- X motor PWM: `PTD0`, `FTM0_CH2`
- Y motor PWM: `PTB4`, `FTM0_CH4`

### Encoder inputs

- X encoder A: `PTE10`
- X encoder B: `PTE5`
- Y encoder A: `PTE4`
- Y encoder B: `PTE3`

### Border inputs

- bottom border: `PTD1`
- top border: `PTE11`
- left border: `PTB13`
- right border: `PTD4`

### CAN

- CAN TX: `PTC3`
- CAN RX: `PTC2`
- CAN STB: `PTD7`

## CAN protocol

All frames use 11-bit standard CAN IDs at 500 kbit/s (sample point at 80%).

### `0x200` - simulator status (RX on your controller)

DLC = 1, sent periodically by the simulator.

Status bits:

- bit 0: top border
- bit 1: bottom border
- bit 2: left border
- bit 3: right border
- bit 4: `HEAD_UP`
- bit 5: `FAIL`

Notes:

- border bits duplicate the dedicated border GPIOs
- `HEAD_UP = 0` means the punch is down or still retracting
- `FAIL` is latched by the simulator and requires simmulator reset

### `0x201` - punch request (TX from your controller)

DLC = 1.

- `data[0] = 0x00`: release punch
- `data[0] = 0x01`: press punch

Important:

- when the punch is down, the simulator expects the head to remain still
- after releasing the punch, the head needs about **200 ms** to retract
- use `HEAD_UP` from `0x200` to know when it is safe to move again

### `0x202` - command from host (RX on your controller)

DLC >= 5.

- byte 0: command
  - `0x00` = `HOME`
  - `0x01` = `MOVE_TO`
  - `0x02` = `PUNCH_AT`
- bytes 1-2: X target, big-endian `uint16_t`
- bytes 3-4: Y target, big-endian `uint16_t`

Coordinates are in **encoder ticks**.

### `0x203` - controller status (TX from your controller)

DLC = 1.

- bit 0: `BUSY`

Send this periodically, for example every **100 ms**.

The host uses this frame to know whether your controller is still executing a
command.

## Coordinate system and work area

Encoder resolution is:

- 10 ticks = 1 mm

After homing, encoder coordinate `(0, 0)` must correspond to the
bottom-left corner of the safe zone.

The safe zone is:

- X: `0 .. 20000` ticks
- Y: `0 .. 15000` ticks

There is still a 100 mm border region outside the safe zone, but your command
targets should stay inside the safe zone. Clamping commanded targets to the
safe-zone limits is a reasonable precaution.

## How to implement the three commands

### 1. `HOME`

Purpose:

- establish a reproducible encoder origin `(0, 0)`

Recommended strategy:

1. drive both axes in the negative direction
2. continue until:
   - left border becomes active
   - bottom border becomes active
3. once a border becomes active on an axis, reverse that axis only
4. move that axis forward until the corresponding border becomes inactive
5. at the moment the border becomes inactive, reset that axis encoder to zero
6. when both axes have been zeroed, homing is complete

Why this works:

- the border signals are active **inside the 100 mm border strip**
- by backing out until the border deactivates, you stop exactly at the inner
  edge of the border strip, which is the safe-zone corner

Notes:

- this command should set `BUSY=1` while homing is in progress
- do not use the punch during homing
- stop both axes at the end

### 2. `MOVE_TO`

Purpose:

- move the head to the target coordinate without punching

Recommended strategy:

1. ensure punch is released
2. set `BUSY=1`
3. read current encoder position continuously
4. compute position error for both axes
5. generate X/Y motor effort from the error
6. stop only when:
   - position error is small enough, and
   - the head is no longer moving significantly
7. set motor effort to zero and clear busy

Practical advice:

- a simple proportional controller may work, but expect overshoot and trouble
  near zero speed
- a better approach is a PD or similar damped position controller
- use output saturation
- it is a good idea to define:
  - a position tolerance
  - a velocity tolerance

Important safety rules:

- keep punch released during the entire move
- do not finish the command while the head is still sliding
- do not drive the head outside the machine area

### 3. `PUNCH_AT`

Purpose:

- move to the target, then perform a punch safely

Recommended strategy:

1. execute the same move phase as for `MOVE_TO`
2. once at target, command zero motor effort on both axes
3. verify the head is effectively at rest
4. send punch request `0x201 = 0x01` and keep doing it at periodic intervals until you see HEAD_UP = 0
5. send punch release `0x201 = 0x00`
6. wait until simulator status reports `HEAD_UP = 1`
7. only then clear busy and allow the next command

If the head moves while punch is down, the simulator latches FAIL.

## Peripheral configuration guidance

This section is intentionally high-level. You should still study the
reference manual and derive the exact register writes yourself.

### PWM outputs for the two axes

Use `FTM0` in edge-aligned PWM mode.

High-level steps:

1. enable clocks for the PWM pin ports and for `FTM0`
2. set `PTD0` and `PTB4` to the FTM alternate function
3. stop `FTM0` while configuring it
4. configure the PWM period
5. configure channels 2 and 4 for edge-aligned high-true PWM
6. start the timer

Recommended operating point:

- PWM frequency around 1 kHz

The simulator interprets duty cycle approximately as:

- about `4.5%` = full reverse
- `50%` = zero effort
- about `95.5%` = full forward

It is convenient to represent commanded motor effort as:

- `-10000 .. +10000`

and convert that linearly to PWM duty cycle.

### Encoder inputs

You need to decode the two quadrature encoders to know the current position of the head.
This needs to be done in software using GPIO and PORT (external) interrupts

1. configure the four encoder pins as GPIO inputs
2. enable interrupt detection on both edges
3. in the IRQ handler, read the two-bit state of the axis
4. compare previous state and new state
5. update the signed encoder count from a transition table

Because encoder inputs are timing-sensitive, keep the IRQ handler short.

### Border inputs

Configure the four border pins as GPIO inputs and read them directly.

You will use them mainly for homing.

The border signals are active high when the head is inside the corresponding
border strip.

### CAN

Use `FlexCAN0`.

High-level steps:

1. enable clocks for the CAN pin ports and for `FlexCAN0`
2. configure TX/RX pins to the CAN alternate function
3. configure `PTD7` as GPIO output and drive it to leave transceiver standby
4. enter CAN freeze/halt mode
5. configure bit timing for 500 kbit/s and sample point at 80%
6. configure at least:
   - one RX mailbox for `0x200`
   - one RX mailbox for `0x202`
   - one TX mailbox for `0x201`
   - one TX mailbox for `0x203`
7. clear mailbox flags
8. leave freeze mode


