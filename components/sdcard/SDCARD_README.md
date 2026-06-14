# SDCARD — SD Card Storage

Provides SD card mounting, filesystem access, and basic file operations using the ESP-IDF FAT filesystem. Supports directory creation, file reading, file writing, and append operations through a simplified API.

## Overview

SDCARD manages the lifecycle of an SD card mounted in SPI mode. It abstracts filesystem setup and exposes common file operations for application modules.

All file paths are expected to be relative to the configured mount point. The module tracks mount status internally and provides a simple initialization check via `sdcard_is_initialized()`.

## API

```c
esp_err_t sdcard_init(void);
void      sdcard_dispose(void);

bool      sdcard_is_initialized(void);

esp_err_t sdcard_mkdir(const char *path);

esp_err_t sdcard_write_file(const char *path, const char *data);
esp_err_t sdcard_append_file(const char *path, const char *data);

esp_err_t sdcard_read_file(
    const char *path,
    char *out_buffer,
    size_t max_len);
```

## Initialization

```c
if (sdcard_init() == ESP_OK)
{
    // SD card mounted and ready
}
```

`sdcard_init()` mounts the SD card and prepares the filesystem for access. The module must be initialized before any file or directory operations are performed.

Use `sdcard_is_initialized()` to verify that the card is currently mounted.

## Directory Creation

```c
sdcard_mkdir("logs");
sdcard_mkdir("config");
```

`sdcard_mkdir()` creates a directory relative to the SD card root. If the directory already exists, the function succeeds without modifying the filesystem.

## Writing Files

```c
sdcard_write_file(
    "config/settings.json",
    json_data);
```

`sdcard_write_file()` creates the target file if it does not exist. If the file already exists, its contents are replaced.

## Appending Files

```c
sdcard_append_file(
    "logs/system.log",
    log_entry);
```

`sdcard_append_file()` appends text data to the end of the target file. The file is created automatically if it does not already exist.

This operation is intended for log files and incremental data recording.

## Reading Files

```c
char buffer[512];

if (sdcard_read_file(
        "config/settings.json",
        buffer,
        sizeof(buffer)) == ESP_OK)
{
    // process data
}
```

The caller provides the destination buffer and maximum buffer size. The module reads file contents into the supplied buffer up to the specified length.

## Mount Status

```c
if (sdcard_is_initialized())
{
    // filesystem available
}
```

Returns `true` when the SD card is successfully mounted and available for filesystem operations.

## Shutdown

```c
sdcard_dispose();
```

Unmounts the filesystem and releases all resources associated with the SD card interface.

After disposal, all file operations will fail until `sdcard_init()` is called again.

## Dependencies

* ESP-IDF FAT filesystem support
* ESP-IDF SD SPI host driver
* ESP-IDF VFS subsystem
