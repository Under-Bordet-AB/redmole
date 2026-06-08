#ifndef SNTP_H
#define SNTP_H

/**
 * @file
 * @brief SNTP time synchronization module.
 *
 * Thin wrapper around esp_sntp that keeps the system clock synchronized with
 * pool.ntp.org. The system timezone is set to CET/CEST on start. Call
 * sntp_sync_start() when the network comes up and sntp_sync_stop() when it
 * goes down.
 */

/**
 * @brief Start the SNTP client and begin asynchronous clock synchronization.
 *
 * Configures the system timezone to CET/CEST, then starts esp_sntp in poll
 * mode against pool.ntp.org. Returns immediately; the system clock is set
 * asynchronously after the first NTP response. The stack re-syncs
 * automatically every hour without further calls.
 *
 * @see sntp_sync_stop
 */
void sntp_sync_start(void);

/**
 * @brief Stop the SNTP client.
 *
 * @see sntp_sync_start
 */
void sntp_sync_stop(void);

#endif // SNTP_H
