# SDCARD_LOG — SD Card Logging

Provides persistent application logging to files stored on the SD card. The module manages log file creation and lifecycle, allowing application diagnostics and runtime events to be retained across reboots.

## Overview

SDCARD_LOG is a lightweight logging backend built on top of the SDCARD module. It initializes a logging destination on the mounted SD card and manages log output for the lifetime of the application.

The logging subsystem must be initialized after the SD card has been successfully mounted.

## API

```c
esp_err_t sdcard_log_init(const char *dir_path);
void      sdcard_log_dispose(void);
```

## Initialization

```c
sdcard_log_init("logs");
```

Initializes the logging subsystem and configures the target directory used for log storage.

The specified directory must exist or be creatable on the mounted SD card. Log files are created within this directory as required by the implementation.

`sdcard_log_init()` returns `ESP_OK` when the logging backend is ready to accept log messages.

## Shutdown

```c
sdcard_log_dispose();
```

Stops the logging subsystem and releases resources associated with log processing.

The logging task and queue are destroyed immediately. Any log messages still pending in the queue may be discarded.
## Typical Usage

```c
sdcard_init();

sdcard_log_init("logs");

// application runtime

sdcard_log_dispose();
sdcard_dispose();
```

## Dependencies

* SDCARD