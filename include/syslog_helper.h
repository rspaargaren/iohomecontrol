/*
   Syslog helper for forwarding log messages to a remote server
*/

#ifndef SYSLOG_HELPER_H
#define SYSLOG_HELPER_H

#pragma once
#include <Arduino.h>

// Initialize UDP + target IP from user_config.h
void initSyslog();

// Legacy signature (no severity)
void sendSyslog(const String &msg);

// New signature with explicit severity (0..7)
// 0 emerg, 1 alert, 2 crit, 3 err, 4 warn, 5 notice, 6 info, 7 debug
void sendSyslog(const String &msg, int severity);

#endif // SYSLOG_HELPER_H

