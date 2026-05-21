# TODO - Multi-Device Heart Rate Monitor

## Step 1: Frontend (HeartRateDashboard.vue)
- [x] Add device polling every 5s from `GET /api/esp32/devices`

- [x] Add tab selector UI with Online/Offline indicator

- [x] Add `selectedDevice` state and logic to switch tabs

- [x] Filter SSE `pulse-data` by `data.deviceId === selectedDevice`

- [x] On device change, reset charts and load history via `GET /api/esp32/history?deviceId=...`

- [x] Ensure realtime PPG/trend rendering only updates for selected device

- [x] Cleanup intervals/timeouts and close SSE on unmount

## Step 2: Verification
- [ ] Run backend and frontend
- [ ] Flash 2 ESP32 devices with different deviceId values
- [ ] Confirm each tab shows only its own data
- [ ] Confirm tab status becomes Offline after ~30s without data

