/**
 * @file can.h
 * @ingroup lib
 * @brief CAN 2.0 driver
 *
 * Provides CAN 2.0 communication using FlexCAN0 at 500 kbps with 80% sample point.
 * Supports standard (11-bit) message IDs only.
 *
 * Pin configuration:
 * - CAN TX:  PTC3
 * - CAN RX:  PTC2
 * - CAN STB: PTD7 (transceiver standby control)
 *
 * This module introduces FlexCAN mailbox configuration. Students need to
 * understand both the global CAN controller setup and the per-mailbox setup
 * for TX/RX behaviour.
 */

#ifndef LIB_CAN_H
#define LIB_CAN_H

#include <stdint.h>
#include <stdbool.h>

/** @brief CAN message ID for current temperature/humidity (TX, 4 bytes) */
#define CAN_MSG_ID_ENV_CURRENT  0x100

/** @brief CAN message ID for min/max temperature/humidity (TX, 8 bytes) */
#define CAN_MSG_ID_ENV_MINMAX   0x101

/** @brief CAN message ID for min/max temperature/humidity reset command (RX) */
#define CAN_MSG_ID_ENV_RESET    0x102

/** @brief CAN message ID for text messages (RX, max 8 bytes with bit 7 = newline) */
#define CAN_MSG_ID_TEXT     0x103

/**
 * @brief Initialize CAN hardware
 *
 * Configures FlexCAN0 for CAN 2.0 at 500 kbps with 80% sample point.
 * Takes the CAN transceiver out of standby.
 * Configures TX mailboxes for current and min/max messages,
 * and RX mailboxes with filters for reset (1 MB) and text (10 MBs) messages.
 *
 * @note Must be called after clock_init()
 *
 * High-level implementation outline:
 * 1. Enable clock gates for the CAN TX/RX/STB pin PORTs.
 * 2. Configure TX and RX pins to the FlexCAN alternate function.
 * 3. Configure the transceiver standby pin as GPIO output and drive it to the
 *    active state so the external transceiver leaves standby.
 * 4. Enable the clock gate for `FlexCAN0` and select its clock source.
 * 5. Put the CAN controller into freeze/halt mode before reconfiguration.
 * 6. Configure global controller settings such as mailbox count, individual
 *    masking, self-reception policy, and bit timing.
 * 7. Initialize mailbox RAM and then configure:
 *    - TX mailboxes as inactive
 *    - RX mailboxes as empty with the desired IDs and masks
 * 8. Clear pending mailbox flags.
 * 9. Leave freeze mode and wait until the controller reports ready.
 *
 * What to read in the reference manual:
 * - PCC and PORT chapters: pin mux and clocking
 * - GPIO chapter: transceiver standby pin control
 * - FlexCAN chapter: freeze mode, bit timing, mailbox RAM layout, RX masks,
 *   interrupt/status flags
 */
void can_init(void);

/**
 * @brief Send current temperature/humidity CAN message
 *
 * Sends a 4-byte message with raw 16-bit temperature and humidity values.
 * If the TX mailbox is busy, the previous transmission is aborted.
 *
 * @param raw_temp Raw 16-bit temperature value from sensor
 * @param raw_hum Raw 16-bit humidity value from sensor
 *
 * High-level implementation outline:
 * 1. Pack the application data into the payload byte layout expected by the
 *    protocol.
 * 2. If the chosen TX mailbox is currently busy, abort or otherwise resolve
 *    the previous transmission according to your design.
 * 3. Write the message ID, payload words, and DLC into the mailbox.
 * 4. Activate the mailbox for transmission by writing the appropriate mailbox
 *    code last.
 *
 * What to read in the reference manual:
 * - FlexCAN chapter: TX mailbox format, CODE field, ID field, data words
 */
void can_send_env_current(uint16_t raw_temp, uint16_t raw_hum);

/**
 * @brief Send min/max temperature/humidity CAN message
 *
 * Sends an 8-byte message with raw 16-bit min/max temperature and humidity values.
 * If the TX mailbox is busy, the previous transmission is aborted.
 *
 * @param raw_temp_min Raw 16-bit minimum temperature
 * @param raw_temp_max Raw 16-bit maximum temperature
 * @param raw_hum_min Raw 16-bit minimum humidity
 * @param raw_hum_max Raw 16-bit maximum humidity
 *
 * High-level implementation outline:
 * 1. Pack four 16-bit values into the protocol payload layout.
 * 2. Use the selected TX mailbox to send the frame using the same mailbox
 *    sequence as other transmit functions: resolve busy state, write ID/data,
 *    then trigger TX by writing the CODE/DLC field.
 *
 * What to read in the reference manual:
 * - FlexCAN chapter: mailbox data layout and transmit activation
 */
void can_send_env_minmax(uint16_t raw_temp_min, uint16_t raw_temp_max,
                         uint16_t raw_hum_min, uint16_t raw_hum_max);

/**
 * @brief Check for a received reset command
 *
 * Checks the reset RX mailbox for a received message.
 *
 * @return true if a reset command was received, false otherwise
 *
 * High-level implementation outline:
 * 1. Check whether the RX mailbox flag indicates a newly received frame.
 * 2. Read and validate the mailbox status.
 * 3. Complete the FlexCAN mailbox unlock/acknowledge sequence required after
 *    reading an RX mailbox.
 * 4. Clear the mailbox flag and report whether a frame was present.
 *
 * What to read in the reference manual:
 * - FlexCAN chapter: RX mailbox state, timer read for unlocking, `IFLAG1`
 */
bool can_receive_env_reset(void);

/**
 * @brief Check for a received text message
 *
 * Checks the text RX mailboxes for a received message. If available,
 * copies up to 8 bytes of payload into the provided buffer.
 *
 * @param data Buffer for received data (must be at least 8 bytes)
 * @param dlc Pointer to store the data length code
 * @return true if a text message was received, false otherwise
 *
 * High-level implementation outline:
 * 1. Inspect the set of RX mailboxes reserved for text messages.
 * 2. Decide which received message to return first if more than one mailbox is
 *    full (for example by comparing timestamps).
 * 3. Read payload bytes and DLC from that mailbox.
 * 4. Complete the required mailbox unlock/flag-clear sequence.
 *
 * What to read in the reference manual:
 * - FlexCAN chapter: RX mailbox read sequence, timestamp field, `IFLAG1`
 */
bool can_receive_env_text(uint8_t data[8], uint8_t *dlc);

#endif
