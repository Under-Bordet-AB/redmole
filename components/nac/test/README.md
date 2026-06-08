# NAC Test
Run `idf.py -C components/nac/test build flash monitor` from the root directory to trigger tests for nac.

## Current test 
- Test WiFi state after init function, should be NAC_WIFI_DISCONNECTED
- Test part of WiFi state machine from idle to connecting via request connect, should be NAC_WIFI_CONNECTING
- Test that PSRAM allocation works and returns valid pointer and that AP count is set to 0.
- Test that request scan function returns valid pointer to PSRAM and that AP count is 0 or more.

## Notes on other tests
- It would be nice to test the disconnect function but it will not be possible without a very advanced stub so it is better left to an integration test. We know the sequence of events that should occur when disconnecting.
