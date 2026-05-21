<template>
  <div class="dashboard-wrapper">
    <!-- Header -->
    <header class="glass-header">
      <div class="logo-area">
        <svg class="pulse-icon animate-pulse" viewBox="0 0 24 24" fill="currentColor">
          <path d="M12 21.35l-1.45-1.32C5.4 15.36 2 12.28 2 8.5 2 5.42 4.42 3 7.5 3c1.74 0 3.41.81 4.5 2.09C13.09 3.81 14.76 3 16.5 3 19.58 3 22 5.42 22 8.5c0 3.78-3.4 6.86-8.55 11.54L12 21.35z"/>
        </svg>
        <h1>Hệ Thống Theo Dõi Sức Khỏe Multi-Device</h1>
      </div>

      <div class="status-badge" :class="isSSEConnected ? 'connected' : 'disconnected'">
        <span class="status-dot"></span>
        <span class="status-text">{{ isSSEConnected ? 'HỆ THỐNG ONLINE' : 'HỆ THỐNG OFFLINE' }}</span>
      </div>
    </header>

    <!-- Main Content Grid -->
    <main class="dashboard-grid">
      <!-- Left Sidebar: Device List -->
      <section class="grid-card device-list-card glass-card">
        <div class="card-header">
          <h2 class="cyan-glow">Danh Sách Thiết Bị</h2>
          <span class="card-subtitle">Chọn thiết bị để xem dữ liệu đo lịch sử và trực tiếp</span>
        </div>

        <div class="device-list-container">
          <div v-if="devices.length === 0" class="no-devices-msg">
            Chưa phát hiện thiết bị nào...
          </div>
          <div
            v-for="d in devices"
            :key="d.deviceId"
            class="device-item"
            :class="{ active: d.deviceId === selectedDevice }"
            @click="selectDevice(d.deviceId)"
          >
            <div class="device-info-main">
              <svg class="device-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <rect x="2" y="2" width="20" height="20" rx="4" />
                <rect x="6" y="6" width="12" height="12" rx="2" />
                <path d="M9 12h6M12 9v6" />
              </svg>
              <div class="device-meta">
                <span class="device-name">{{ d.deviceId }}</span>
                <span class="device-time">Cập nhật: {{ formatTimeOnly(d.lastSeen) }}</span>
              </div>
            </div>
            
            <span class="status-indicator" :class="d.online ? 'online' : 'offline'">
              <span class="indicator-dot"></span>
              {{ d.online ? 'Online' : 'Offline' }}
            </span>
          </div>
        </div>
      </section>

      <!-- Right Column: Detail Area -->
      <div class="detail-container">
        <div v-if="!selectedDevice" class="no-selection-card glass-card">
          <div class="placeholder-content">
            <svg class="placeholder-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
              <circle cx="12" cy="12" r="10" />
              <path d="M12 16v-4M12 8h.01" />
            </svg>
            <h3>Vui lòng chọn thiết bị</h3>
            <p>Chọn một thiết bị từ danh sách bên trái để theo dõi nhịp tim và nồng độ SpO2.</p>
          </div>
        </div>

        <div v-else class="selected-device-view">
          <!-- Active Device Header -->
          <div class="active-device-header glass-card">
            <div class="header-left">
              <span class="device-title-label">Thiết bị đang xem:</span>
              <h2 class="cyan-glow">{{ selectedDevice }}</h2>
            </div>
            <div class="header-right">
              <span class="last-active-label" v-if="selectedDeviceLastSeen">
                Dữ liệu cuối: <strong class="val-white">{{ formatFullTimestamp(selectedDeviceLastSeen) }}</strong>
              </span>
              <span class="status-indicator-badge" :class="isSelectedDeviceOnline ? 'online' : 'offline'">
                {{ isSelectedDeviceOnline ? 'Trực tuyến (Live)' : 'Ngoại tuyến (History)' }}
              </span>
            </div>
          </div>

          <!-- Metrics Row -->
          <div class="metrics-row">
            <!-- Heart Rate Card -->
            <div class="grid-card metric-card hr-card glass-card">
              <div class="heart-pulse-container">
                <svg 
                  class="heart-svg" 
                  :class="{ 'beating': hasFinger && bpm > 0 }" 
                  :style="heartBeatStyle" 
                  viewBox="0 0 24 24" 
                  fill="currentColor"
                >
                  <path d="M12 21.35l-1.45-1.32C5.4 15.36 2 12.28 2 8.5 2 5.42 4.42 3 7.5 3c1.74 0 3.41.81 4.5 2.09C13.09 3.81 14.76 3 16.5 3 19.58 3 22 5.42 22 8.5c0 3.78-3.4 6.86-8.55 11.54L12 21.35z"/>
                </svg>
                <div class="glowing-ring"></div>
              </div>
              
              <div class="metric-data">
                <span class="metric-label">NHỊP TIM</span>
                <div class="metric-value red-glow">
                  <span class="value-num">{{ bpm > 0 ? bpm : '--' }}</span>
                  <span class="value-unit">BPM</span>
                </div>
                <span class="metric-status" :class="bpmStatusClass">
                  {{ bpmStatusText }}
                </span>
              </div>
            </div>

            <!-- SpO2 Card -->
            <div class="grid-card metric-card spo2-card glass-card">
              <div class="spo2-progress-container">
                <svg class="progress-ring" viewBox="0 0 100 100">
                  <circle class="ring-bg" cx="50" cy="50" r="40" />
                  <circle 
                    class="ring-bar" 
                    cx="50" 
                    cy="50" 
                    r="40" 
                    :style="spo2RingStyle"
                  />
                </svg>
                <div class="progress-inner">
                  <span class="progress-icon">O₂</span>
                </div>
              </div>

              <div class="metric-data">
                <span class="metric-label">NỒNG ĐỘ OXY (SpO2)</span>
                <div class="metric-value emerald-glow">
                  <span class="value-num">{{ spo2 > 0 ? spo2 : '--' }}</span>
                  <span class="value-unit">%</span>
                </div>
                <span class="metric-status" :class="spo2StatusClass">
                  {{ spo2StatusText }}
                </span>
              </div>
            </div>
          </div>

          <!-- History Chart Card -->
          <section class="grid-card trend-section glass-card">
            <div class="card-header">
              <h2 class="emerald-glow">Biểu Đồ Lịch Sử Nhịp Tim & SpO2</h2>
              <span class="card-subtitle">Hiển thị lịch sử dữ liệu đo dựa trên mốc thời gian lưu trong database</span>
            </div>
            <div class="chart-container">
              <div id="trend-chart" ref="trendChartRef"></div>
            </div>
          </section>
        </div>
      </div>
    </main>

    <!-- Footer Stats -->
    <footer class="glass-footer">
      <div class="footer-item">
        <span class="footer-label">Hệ thống:</span>
        <span class="footer-val">Đầu cuối ESP32-C3 & MAX30102</span>
      </div>
      <div class="footer-item">
        <span class="footer-label">Cơ sở dữ liệu:</span>
        <span class="footer-val val-green">Kết nối ổn định</span>
      </div>
      <div class="footer-item">
        <span class="footer-label">Tổng số thiết bị:</span>
        <span class="footer-val">{{ devices.length }}</span>
      </div>
    </footer>
  </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted, computed } from 'vue'
import * as echarts from 'echarts'

const isSSEConnected = ref(false)
const hasFinger = ref(false)
const bpm = ref(0)
const spo2 = ref(0)

const devices = ref([]) // [{ deviceId: string, online: boolean, lastSeen: string }]
const selectedDevice = ref('')
const selectedDeviceLastSeen = ref(null)

const trendChartRef = ref(null)
let trendChart = null

const BASE_API = '' // Handled by dev proxy or relative paths

let devicesPollInterval = null
let selectedDeviceLoadGuard = 0
let sseSource = null

// Check if selected device is currently online
const isSelectedDeviceOnline = computed(() => {
  const d = devices.value.find(dev => dev.deviceId === selectedDevice.value)
  return d ? d.online : false
})

// Format helpers
const formatTimeOnly = (tsString) => {
  if (!tsString) return '—'
  const d = new Date(tsString)
  if (isNaN(d.getTime())) return tsString
  const pad = (n) => String(n).padStart(2, '0')
  return `${pad(d.getHours())}:${pad(d.getMinutes())}:${pad(d.getSeconds())}`
}

const formatFullTimestamp = (tsString) => {
  if (!tsString) return '—'
  const d = new Date(tsString)
  if (isNaN(d.getTime())) return tsString
  const pad = (n) => String(n).padStart(2, '0')
  return `${pad(d.getHours())}:${pad(d.getMinutes())}:${pad(d.getSeconds())} ${pad(d.getDate())}/${pad(d.getMonth() + 1)}/${d.getFullYear()}`
}

// Fetch all devices from database and in-memory states
const fetchDevices = async () => {
  try {
    const res = await fetch(`${BASE_API}/api/esp32/devices`)
    if (!res.ok) return
    const list = await res.json() // [{ deviceId, online, lastSeen }]

    devices.value = list

    // Auto-select first device if none is selected
    if (!selectedDevice.value && list.length > 0) {
      selectDevice(list[0].deviceId)
    }
  } catch (e) {
    console.error('Failed to fetch devices', e)
  }
}

// Load history for selected device
const loadHistoryForDevice = async (deviceId) => {
  const guard = ++selectedDeviceLoadGuard
  try {
    const res = await fetch(`${BASE_API}/api/esp32/history?deviceId=${encodeURIComponent(deviceId)}`)
    if (!res.ok) throw new Error('history fetch failed')

    const history = await res.json() // chronologically ordered (oldest to newest)
    
    if (guard !== selectedDeviceLoadGuard) return

    // If device is offline, populate current metric displays with the latest record
    if (history.length > 0) {
      const latest = history[history.length - 1]
      bpm.value = latest.heartRate || 0
      spo2.value = latest.spo2 || 0
      hasFinger.value = latest.heartRate > 0
      selectedDeviceLastSeen.value = latest.timestamp
    } else {
      bpm.value = 0
      spo2.value = 0
      hasFinger.value = false
      selectedDeviceLastSeen.value = null
    }

    // Render trend chart with history
    renderTrendChart(history)
  } catch (e) {
    console.error('Failed to load history for device', deviceId, e)
  }
}

const renderTrendChart = (history) => {
  if (!trendChartRef.value) return

  if (!trendChart) {
    trendChart = echarts.init(trendChartRef.value)
  }

  let timestamps = []
  let heartRates = []
  let spo2s = []

  history.forEach(item => {
    timestamps.push(formatTimeOnly(item.timestamp))
    heartRates.push(item.heartRate)
    spo2s.push(item.spo2)
  })

  // Fill in empty visuals if no records exist
  if (timestamps.length === 0) {
    timestamps = Array(20).fill('')
    heartRates = Array(20).fill(null)
    spo2s = Array(20).fill(null)
  }

  const option = {
    backgroundColor: 'transparent',
    grid: {
      left: '4%',
      right: '4%',
      top: '15%',
      bottom: '12%',
      containLabel: true
    },
    legend: {
      textStyle: { color: '#f3f4f6' },
      data: ['Nhịp Tim (BPM)', 'Nồng độ O₂ (SpO2 %)']
    },
    tooltip: {
      trigger: 'axis',
      backgroundColor: 'rgba(15, 23, 42, 0.85)',
      borderColor: 'rgba(255, 255, 255, 0.1)',
      textStyle: { color: '#f3f4f6' }
    },
    xAxis: {
      type: 'category',
      data: timestamps,
      axisLine: { lineStyle: { color: 'rgba(255, 255, 255, 0.1)' } },
      axisLabel: { color: '#9ca3af', rotate: 15 }
    },
    yAxis: [
      {
        type: 'value',
        name: 'BPM',
        min: 40,
        max: 150,
        nameTextStyle: { color: '#ef4444' },
        axisLabel: { color: '#9ca3af' },
        splitLine: { lineStyle: { color: 'rgba(255, 255, 255, 0.05)' } }
      },
      {
        type: 'value',
        name: 'SpO2 (%)',
        min: 0,
        max: 100,
        nameTextStyle: { color: '#10b981' },
        axisLabel: { color: '#9ca3af' },
        splitLine: { show: false }
      }
    ],
    series: [
      {
        name: 'Nhịp Tim (BPM)',
        type: 'line',
        data: heartRates,
        smooth: true,
        showSymbol: true,
        symbolSize: 6,
        itemStyle: { color: '#ef4444' },
        lineStyle: { width: 3 },
        areaStyle: {
          color: new echarts.graphic.LinearGradient(0, 0, 0, 1, [
            { offset: 0, color: 'rgba(239, 68, 68, 0.2)' },
            { offset: 1, color: 'rgba(239, 68, 68, 0.0)' }
          ])
        }
      },
      {
        name: 'Nồng độ O₂ (SpO2 %)',
        type: 'line',
        yAxisIndex: 1,
        data: spo2s,
        smooth: true,
        showSymbol: true,
        symbolSize: 6,
        itemStyle: { color: '#10b981' },
        lineStyle: { width: 3 },
        areaStyle: {
          color: new echarts.graphic.LinearGradient(0, 0, 0, 1, [
            { offset: 0, color: 'rgba(16, 185, 129, 0.2)' },
            { offset: 1, color: 'rgba(16, 185, 129, 0.0)' }
          ])
        }
      }
    ]
  }

  trendChart.setOption(option, true)
}

const selectDevice = async (deviceId) => {
  if (!deviceId || deviceId === selectedDevice.value) return
  selectedDevice.value = deviceId
  
  // Clear local values initially
  bpm.value = 0
  spo2.value = 0
  hasFinger.value = false
  selectedDeviceLastSeen.value = null

  await loadHistoryForDevice(deviceId)
}

// SSE Connection for Live Data Stream
const connectSSE = () => {
  if (sseSource) {
    sseSource.close()
  }

  console.log("Connecting to Spring Boot SSE stream...")
  sseSource = new EventSource(`${BASE_API}/api/esp32/stream`)

  sseSource.addEventListener('init', (e) => {
    console.log("SSE Connection Initialized:", e.data)
    isSSEConnected.value = true
  })

  sseSource.addEventListener('pulse-data', (e) => {
    try {
      const data = JSON.parse(e.data)

      // Only update UI if the event matches the currently selected device
      if (!selectedDevice.value || data.deviceId !== selectedDevice.value) {
        return
      }

      // Read calculations
      bpm.value = data.heartRate || 0
      spo2.value = data.spo2 || 0
      hasFinger.value = bpm.value > 0
      selectedDeviceLastSeen.value = new Date().toISOString()

      // Append live point directly to trend chart if valid
      if (bpm.value > 0 && trendChart) {
        const currentOptions = trendChart.getOption()
        const times = currentOptions.xAxis[0].data
        const rates = currentOptions.series[0].data
        const oxygens = currentOptions.series[1].data

        const nowStr = formatTimeOnly(new Date())
        
        times.push(nowStr)
        rates.push(bpm.value)
        oxygens.push(spo2.value)

        // Maintain size limit of 30 for scrolling real-time updates
        if (times.length > 30) {
          times.shift()
          rates.shift()
          oxygens.shift()
        }

        trendChart.setOption({
          xAxis: { data: times },
          series: [
            { data: rates },
            { data: oxygens }
          ]
        })
      }
    } catch (err) {
      console.error("Failed to parse SSE payload", err)
    }
  })

  sseSource.onerror = (err) => {
    console.error("SSE connection error, attempting reconnect in 5 seconds...", err)
    isSSEConnected.value = false
    setTimeout(connectSSE, 5000)
  }
}

// Heartbeat CSS binding
const heartBeatStyle = computed(() => {
  if (!hasFinger.value || bpm.value <= 0) return {}
  const duration = 60 / bpm.value
  return {
    animationDuration: `${duration}s`
  }
})

// SpO2 ring gauge style calculation
const spo2RingStyle = computed(() => {
  const percentage = spo2.value > 0 ? spo2.value : 0
  const circumference = 2 * Math.PI * 40
  const strokeDashoffset = circumference - (percentage / 100) * circumference
  return {
    strokeDasharray: `${circumference}`,
    strokeDashoffset: `${strokeDashoffset}`
  }
})

// Evaluators
const bpmStatusText = computed(() => {
  if (bpm.value === 0) return 'Đang chờ ngón tay...'
  if (bpm.value < 60) return 'Nhịp tim chậm'
  if (bpm.value > 100) return 'Nhịp tim nhanh'
  return 'Bình thường'
})

const bpmStatusClass = computed(() => {
  if (bpm.value === 0) return 'status-gray'
  if (bpm.value < 60 || bpm.value > 100) return 'status-warning'
  return 'status-success'
})

const spo2StatusText = computed(() => {
  if (spo2.value === 0) return 'Đang chờ ngón tay...'
  if (spo2.value >= 95) return 'Tốt'
  if (spo2.value >= 90) return 'Thiếu oxy nhẹ'
  return 'Nguy hiểm!'
})

const spo2StatusClass = computed(() => {
  if (spo2.value === 0) return 'status-gray'
  if (spo2.value >= 95) return 'status-success'
  if (spo2.value >= 90) return 'status-warning'
  return 'status-danger'
})

const handleResize = () => {
  if (trendChart) trendChart.resize()
}

onMounted(() => {
  // Load Outfit font
  const link = document.createElement('link')
  link.rel = 'stylesheet'
  link.href = 'https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;500;600;700;800&display=swap'
  document.head.appendChild(link)

  connectSSE()
  fetchDevices()

  devicesPollInterval = setInterval(fetchDevices, 5000)
  window.addEventListener('resize', handleResize)
})

onUnmounted(() => {
  window.removeEventListener('resize', handleResize)
  if (devicesPollInterval) clearInterval(devicesPollInterval)
  if (sseSource) sseSource.close()
  if (trendChart) trendChart.dispose()
})
</script>

<style scoped>
/* Translucent Glassmorphic Style */
.dashboard-wrapper {
  padding: 2rem;
  max-width: 1600px;
  margin: 0 auto;
  min-height: 100vh;
  display: flex;
  flex-direction: column;
  gap: 1.5rem;
  box-sizing: border-box;
}

.glass-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 1.25rem 2rem;
  background: rgba(17, 24, 39, 0.7);
  backdrop-filter: blur(12px);
  border: 1px solid rgba(255, 255, 255, 0.08);
  border-radius: 16px;
  box-shadow: 0 8px 32px 0 rgba(0, 0, 0, 0.3);
}

.logo-area {
  display: flex;
  align-items: center;
  gap: 0.75rem;
}

.pulse-icon {
  width: 2.25rem;
  height: 2.25rem;
  color: #ef4444;
  filter: drop-shadow(0 0 8px rgba(239, 68, 68, 0.6));
}

.animate-pulse {
  animation: pulse 1.5s cubic-bezier(0.4, 0, 0.6, 1) infinite;
}

@keyframes pulse {
  0%, 100% { transform: scale(1); opacity: 1; }
  50% { transform: scale(1.1); opacity: 0.85; }
}

.glass-header h1 {
  margin: 0;
  font-size: 1.5rem;
  font-weight: 700;
  letter-spacing: -0.025em;
  background: linear-gradient(135deg, #ffffff 30%, #a5f3fc 100%);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
}

.status-badge {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  padding: 0.5rem 1rem;
  border-radius: 30px;
  font-size: 0.75rem;
  font-weight: 700;
  letter-spacing: 0.05em;
  border: 1px solid transparent;
}

.status-badge.connected {
  background: rgba(16, 185, 129, 0.1);
  border-color: rgba(16, 185, 129, 0.25);
  color: #10b981;
}

.status-badge.connected .status-dot {
  background: #10b981;
  box-shadow: 0 0 10px #10b981;
  animation: beacon 1.5s infinite;
}

.status-badge.disconnected {
  background: rgba(245, 158, 11, 0.1);
  border-color: rgba(245, 158, 11, 0.25);
  color: #f59e0b;
}

.status-badge.disconnected .status-dot {
  background: #f59e0b;
  box-shadow: 0 0 10px #f59e0b;
}

.status-dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
}

@keyframes beacon {
  0% { transform: scale(1); opacity: 1; }
  100% { transform: scale(2.2); opacity: 0; }
}

/* Grid & Panels Layout */
.dashboard-grid {
  display: grid;
  grid-template-columns: 320px 1fr;
  gap: 1.5rem;
  align-items: start;
}

.glass-card {
  background: rgba(17, 25, 40, 0.6);
  backdrop-filter: blur(16px) saturate(120%);
  -webkit-backdrop-filter: blur(16px) saturate(120%);
  border: 1px solid rgba(255, 255, 255, 0.08);
  border-radius: 20px;
  box-shadow: 0 16px 40px 0 rgba(0, 0, 0, 0.4);
  padding: 1.5rem;
  box-sizing: border-box;
}

/* Sidebar device-list panel */
.device-list-card {
  display: flex;
  flex-direction: column;
  max-height: calc(100vh - 180px);
}

.device-list-container {
  display: flex;
  flex-direction: column;
  gap: 0.75rem;
  margin-top: 1rem;
  overflow-y: auto;
  padding-right: 0.25rem;
}

.device-list-container::-webkit-scrollbar {
  width: 4px;
}

.device-list-container::-webkit-scrollbar-thumb {
  background: rgba(255, 255, 255, 0.1);
  border-radius: 10px;
}

.device-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 1rem;
  border-radius: 12px;
  background: rgba(255, 255, 255, 0.03);
  border: 1px solid rgba(255, 255, 255, 0.05);
  cursor: pointer;
  transition: all 0.2s ease;
}

.device-item:hover {
  background: rgba(255, 255, 255, 0.07);
  border-color: rgba(6, 182, 212, 0.2);
  transform: translateY(-2px);
}

.device-item.active {
  background: rgba(6, 182, 212, 0.1);
  border-color: rgba(6, 182, 212, 0.45);
  box-shadow: 0 0 12px rgba(6, 182, 212, 0.15);
}

.device-info-main {
  display: flex;
  align-items: center;
  gap: 0.75rem;
}

.device-icon {
  width: 24px;
  height: 24px;
  color: #9ca3af;
  transition: color 0.2s ease;
}

.device-item.active .device-icon {
  color: #22d3ee;
  filter: drop-shadow(0 0 4px rgba(6, 182, 212, 0.4));
}

.device-meta {
  display: flex;
  flex-direction: column;
}

.device-name {
  font-weight: 700;
  font-size: 0.95rem;
  color: #f3f4f6;
}

.device-time {
  font-size: 0.7rem;
  color: #9ca3af;
  margin-top: 0.15rem;
}

.status-indicator {
  display: inline-flex;
  align-items: center;
  gap: 0.35rem;
  font-size: 0.75rem;
  font-weight: 600;
  padding: 0.2rem 0.5rem;
  border-radius: 20px;
}

.status-indicator.online {
  background: rgba(16, 185, 129, 0.1);
  color: #34d399;
}

.status-indicator.online .indicator-dot {
  width: 6px;
  height: 6px;
  background: #10b981;
  border-radius: 50%;
  box-shadow: 0 0 6px #10b981;
}

.status-indicator.offline {
  background: rgba(255, 255, 255, 0.05);
  color: #9ca3af;
}

.status-indicator.offline .indicator-dot {
  width: 6px;
  height: 6px;
  background: #6b7280;
  border-radius: 50%;
}

.no-devices-msg {
  text-align: center;
  color: #9ca3af;
  font-size: 0.85rem;
  padding: 2rem 0;
}

/* Detail Area */
.detail-container {
  display: flex;
  flex-direction: column;
  gap: 1.5rem;
}

.no-selection-card {
  min-height: 400px;
  display: flex;
  justify-content: center;
  align-items: center;
  text-align: center;
}

.placeholder-content {
  max-width: 380px;
}

.placeholder-icon {
  width: 60px;
  height: 60px;
  color: #4b5563;
  margin-bottom: 1rem;
}

.no-selection-card h3 {
  margin: 0 0 0.5rem 0;
  font-size: 1.25rem;
  color: #f3f4f6;
}

.no-selection-card p {
  margin: 0;
  font-size: 0.9rem;
  color: #9ca3af;
  line-height: 1.5;
}

/* Selected Device Area */
.selected-device-view {
  display: flex;
  flex-direction: column;
  gap: 1.5rem;
}

.active-device-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 1rem 1.5rem;
}

.header-left {
  display: flex;
  align-items: baseline;
  gap: 0.5rem;
}

.header-left h2 {
  margin: 0;
  font-size: 1.6rem;
  font-weight: 800;
}

.device-title-label {
  font-size: 0.85rem;
  color: #9ca3af;
  font-weight: 500;
}

.header-right {
  display: flex;
  align-items: center;
  gap: 1rem;
}

.last-active-label {
  font-size: 0.8rem;
  color: #9ca3af;
}

.val-white {
  color: #ffffff;
}

.status-indicator-badge {
  font-size: 0.75rem;
  font-weight: 700;
  padding: 0.35rem 0.75rem;
  border-radius: 20px;
}

.status-indicator-badge.online {
  background: rgba(16, 185, 129, 0.15);
  border: 1px solid rgba(16, 185, 129, 0.3);
  color: #34d399;
  text-shadow: 0 0 10px rgba(52, 211, 153, 0.2);
}

.status-indicator-badge.offline {
  background: rgba(255, 255, 255, 0.05);
  border: 1px solid rgba(255, 255, 255, 0.1);
  color: #9ca3af;
}

/* Metrics Row */
.metrics-row {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 1.5rem;
}

.metric-card {
  display: flex;
  flex-direction: row;
  align-items: center;
  gap: 1.5rem;
  min-height: 156px;
  position: relative;
  overflow: hidden;
}

.metric-card::before {
  content: '';
  position: absolute;
  top: 0;
  left: 0;
  width: 4px;
  height: 100%;
}

.hr-card::before {
  background: #ef4444;
}

.spo2-card::before {
  background: #10b981;
}

.heart-pulse-container {
  display: flex;
  justify-content: center;
  align-items: center;
  width: 70px;
  height: 70px;
  position: relative;
}

.heart-svg {
  width: 38px;
  height: 38px;
  color: #ef4444;
  z-index: 2;
  filter: drop-shadow(0 0 12px rgba(239, 68, 68, 0.7));
}

.heart-svg.beating {
  animation: beat 1s infinite ease-out;
}

.glowing-ring {
  position: absolute;
  width: 100%;
  height: 100%;
  border-radius: 50%;
  background: rgba(239, 68, 68, 0.08);
  border: 1px solid rgba(239, 68, 68, 0.15);
  animation: pulse-ring 2s infinite cubic-bezier(0.215, 0.610, 0.355, 1);
}

@keyframes beat {
  0% { transform: scale(1); }
  25% { transform: scale(1.15); }
  40% { transform: scale(1.02); }
  60% { transform: scale(1.22); }
  100% { transform: scale(1); }
}

@keyframes pulse-ring {
  0% { transform: scale(0.65); opacity: 1; }
  100% { transform: scale(1.3); opacity: 0; }
}

.spo2-progress-container {
  display: flex;
  justify-content: center;
  align-items: center;
  width: 70px;
  height: 70px;
  position: relative;
}

.progress-ring {
  width: 100%;
  height: 100%;
  transform: rotate(-90deg);
}

.ring-bg {
  fill: none;
  stroke: rgba(255, 255, 255, 0.05);
  stroke-width: 7;
}

.ring-bar {
  fill: none;
  stroke: #10b981;
  stroke-width: 7;
  stroke-linecap: round;
  transition: stroke-dashoffset 0.6s ease;
  filter: drop-shadow(0 0 6px rgba(16, 185, 129, 0.7));
}

.progress-inner {
  position: absolute;
  display: flex;
  justify-content: center;
  align-items: center;
  font-size: 0.95rem;
  font-weight: 800;
  color: #10b981;
}

.metric-data {
  display: flex;
  flex-direction: column;
  gap: 0.25rem;
}

.metric-label {
  font-size: 0.75rem;
  font-weight: 700;
  letter-spacing: 0.08em;
  color: #9ca3af;
}

.metric-value {
  display: flex;
  align-items: baseline;
  gap: 0.25rem;
}

.value-num {
  font-size: 2.75rem;
  font-weight: 800;
  letter-spacing: -0.03em;
  line-height: 1;
}

.value-unit {
  font-size: 1rem;
  font-weight: 700;
  color: #9ca3af;
}

.red-glow {
  color: #ffffff;
  text-shadow: 0 0 20px rgba(239, 68, 68, 0.4);
}

.emerald-glow {
  color: #ffffff;
  text-shadow: 0 0 20px rgba(16, 185, 129, 0.4);
}

.metric-status {
  font-size: 0.8rem;
  font-weight: 600;
  padding: 0.15rem 0.6rem;
  border-radius: 4px;
  display: inline-block;
  align-self: flex-start;
}

.status-success {
  background: rgba(16, 185, 129, 0.15);
  color: #34d399;
}

.status-warning {
  background: rgba(245, 158, 11, 0.15);
  color: #fbbf24;
}

.status-danger {
  background: rgba(239, 68, 68, 0.15);
  color: #fca5a5;
  animation: blink-danger 1s infinite alternate;
}

@keyframes blink-danger {
  0% { opacity: 0.75; }
  100% { opacity: 1; filter: drop-shadow(0 0 10px rgba(239, 68, 68, 0.6)); }
}

.status-gray {
  background: rgba(255, 255, 255, 0.05);
  color: #9ca3af;
}

/* History Trend Section */
.trend-section {
  min-height: 380px;
}

.card-header {
  margin-bottom: 1.25rem;
}

.card-header h2 {
  margin: 0;
  font-size: 1.2rem;
  font-weight: 700;
  letter-spacing: -0.01em;
}

.cyan-glow {
  color: #a5f3fc;
  text-shadow: 0 0 15px rgba(6, 182, 212, 0.35);
}

.emerald-glow {
  color: #a7f3d0;
  text-shadow: 0 0 15px rgba(16, 185, 129, 0.35);
}

.card-subtitle {
  font-size: 0.8rem;
  color: #9ca3af;
  margin-top: 0.25rem;
  display: block;
}

.chart-container {
  flex-grow: 1;
  position: relative;
  min-height: 280px;
}

#trend-chart {
  width: 100%;
  height: 100%;
  min-height: 280px;
}

/* Glassmorphic Footer */
.glass-footer {
  display: flex;
  justify-content: space-around;
  align-items: center;
  flex-wrap: wrap;
  padding: 1rem 2rem;
  background: rgba(17, 24, 39, 0.7);
  backdrop-filter: blur(12px);
  border: 1px solid rgba(255, 255, 255, 0.08);
  border-radius: 16px;
  box-shadow: 0 8px 32px 0 rgba(0, 0, 0, 0.3);
  margin-top: auto;
  gap: 1rem;
}

.footer-item {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  font-size: 0.85rem;
}

.footer-label {
  color: #9ca3af;
  font-weight: 500;
}

.footer-val {
  font-weight: 700;
  color: #ffffff;
}

.val-green {
  color: #10b981;
}

/* Responsiveness */
@media (max-width: 1024px) {
  .dashboard-grid {
    grid-template-columns: 1fr;
  }
}

@media (max-width: 640px) {
  .dashboard-wrapper {
    padding: 1rem;
  }
  .metrics-row {
    grid-template-columns: 1fr;
  }
  .glass-header {
    flex-direction: column;
    gap: 1rem;
    align-items: flex-start;
  }
  .status-badge {
    align-self: flex-start;
  }
}
</style>
