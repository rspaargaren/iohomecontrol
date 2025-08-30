/*
   Syslog helper for forwarding log messages to a remote server
*/

#ifndef SYSLOG_HELPER_H
#define SYSLOG_HELPER_H

#include <Arduino.h>

void initSyslog();
void sendSyslog(const String &msg);

#endif // SYSLOG_HELPER_H

