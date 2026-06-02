# NAC Test
Run `idf.py -C components/nac/test build flash monitor` from the root directory to trigger tests for nac.

## Current test 
1. Tests wifi status and there for state after init
2. Tests that wifi request connect returns ESP_OK if state is: WIFI_STATE_CONNECTED

// Add more tests
