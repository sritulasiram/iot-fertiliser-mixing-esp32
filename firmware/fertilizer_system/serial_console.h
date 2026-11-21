#pragma once
/*
 * serial_console — text commands over the USB serial port (115200 baud).
 * Works with no network at all. Type "help" in the Serial Monitor.
 */
void consoleBegin();
void consoleLoop();   // non-blocking: handles one complete line at a time
