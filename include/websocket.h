#pragma once
#include <WebSocketsClient.h>


/* Command
 *
 * value: integer value of command (state number, motion, etc)
 * altValue: secondary value ***
 */
struct Command {
    uint32_t value = 0;
    uint32_t altValue = 0;
};

/* websocketInit
 *
 * Initialize the websocket connection, and spawn a task to watch the 
 * connection. Should be called from setup().
 *
 * Effects: Is blocking during WiFi connection, and steals a core
 *
 */
void websocketInit();  // please call me in setup()

/* getCommand
 *
 * Update Command struct with most recent command recieved over the websocket
 * sent with our team's client ID
 *
 * Param:  outCmd - reference to a Command object which will be written to 
 *                  inside of the function
 * Return: bool   - true => outCmd was updated with a new command from the ws
 *                  false => outCmd was not updated
 *
 * Effects: outCmd has the potential to have its fields changed
 */
bool getCommand(Command& outCmd);

// ===================================================================

/* Communication over the websocket:
 *
 * All of our packets are prefixed with CLIENT_ID (MAGICSMOKE67), followed 
 * by a space and then a command string. For example, a valid command is:
 *
 *              'MAGICSMOKE67 set: STATE=1,30'
 *
 * STATE VALUES:
 *  value in packet represents state of machine (0-6), values outside
 *  are undefined by the websocket
 *
 *  0 => Stop
 *  1 => Both motors forward
 *  2 => Both motors backward
 *  3 => Pivot CW
 *  4 => Pivot CCW
 *  5 => Right turn at radius r
 *  6 => Left turn at radius r
 *
 *              'MAGICSMOKE67 set: STATE=1,50'
 *
 *  For motor commands with no secondary argument, use X to fill null
 *
 *              'MAGICSMOKE67 set: STATE=0,X'
 */

// ===================================================================
