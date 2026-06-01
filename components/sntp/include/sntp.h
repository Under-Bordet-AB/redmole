#ifndef SNTP_H
#define SNTP_H

/*
 * Initializes and configures the SNTP (Simple Network Time Protocol) 
 * service to synchronize the system time with an NTP server. 
 * 
 * When started, esp_sntp periodically re-syncs the clock on its own, by default every hour.
 * To stop, call sntp_sync_stop().
 */
void sntp_sync_start(void);
void sntp_sync_stop(void);

#endif // SNTP_H