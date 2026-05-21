import { reactive, ref } from 'vue'
import { Client } from '@stomp/stompjs'
import SockJS from 'sockjs-client/dist/sockjs'

const normalize = (s) => String(s ?? '').trim()

/**
 * Đồng bộ trạng thái đèn Andon (RF433) với DWM qua work_center_id ≡ machine_id.
 * STOMP /topic/andon từ backend-ANDON + poll /active_alerts mỗi 30s.
 */
export function useAndonLedAlerts() {
  const ledByWorkCenter = reactive({})
  const connected = ref(false)

  let client = null
  let pollTimer = null

  const andonBase = import.meta.env.VITE_ANDON_BASE_URL
  const enabled = typeof andonBase === 'string' && andonBase.length > 0

  function channelKey(idLed) {
    const s = String(idLed ?? '').trim()
    return s === '2' ? '2' : '1'
  }

  function setChannel(wcId, idLed, status) {
    const k = normalize(wcId)
    if (!k || !idLed) return
    const ch = channelKey(idLed)
    if (!ledByWorkCenter[k]) ledByWorkCenter[k] = {}
    if (status === 'clear' || status === null || status === undefined) {
      delete ledByWorkCenter[k][ch]
      if (Object.keys(ledByWorkCenter[k]).length === 0) {
        delete ledByWorkCenter[k]
      }
    } else {
      ledByWorkCenter[k][ch] = status
    }
  }

  function applyPayload(data) {
    if (!data) return
    const wc = data.work_center_id != null ? normalize(data.work_center_id) : ''
    const idLed = data.id_led != null ? String(data.id_led).trim() : ''
    if (!wc || !idLed) return

    if (data.start_time) {
      const st = data.status === 'timeout' ? 'timeout' : 'active'
      setChannel(wc, idLed, st)
      return
    }
    // Chỉ tắt khi backend gửi explicit null (RF433 lần 2). Bỏ qua message không có start_time (vd. API chỉ bật MP3).
    if (Object.prototype.hasOwnProperty.call(data, 'start_time') && data.start_time == null) {
      setChannel(wc, idLed, 'clear')
    }
  }

  async function fetchActiveAlerts() {
    if (!enabled) return
    try {
      const base = andonBase.replace(/\/$/, '')
      const res = await fetch(`${base}/active_alerts`, {
        headers: { Accept: 'application/json' }
      })
      const json = await res.json()
      if (json.status !== 'success' || !Array.isArray(json.data)) return

      for (const k of Object.keys(ledByWorkCenter)) {
        delete ledByWorkCenter[k]
      }
      for (const row of json.data) {
        applyPayload({
          work_center_id: row.work_center_id,
          id_led: row.id_led,
          start_time: row.start_time,
          status: row.status
        })
      }
    } catch {
      // ignore network errors
    }
  }

  function connect() {
    if (!enabled) return
    const wsUrl = `${andonBase.replace(/\/$/, '')}/ws`
    try {
      const stomp = new Client({
        webSocketFactory: () => new SockJS(wsUrl),
        reconnectDelay: 4000,
        heartbeatIncoming: 4000,
        heartbeatOutgoing: 4000,
        onConnect: () => {
          connected.value = true
          stomp.subscribe('/topic/andon', (msg) => {
            try {
              const body = JSON.parse(msg.body)
              if (body.type === 'andon_alert' && body.data) {
                applyPayload(body.data)
              }
            } catch {
              // ignore
            }
          })
        },
        onDisconnect: () => {
          connected.value = false
        },
        onStompError: () => {
          connected.value = false
        },
        onWebSocketError: () => {
          connected.value = false
        }
      })
      client = stomp
      stomp.activate()
    } catch {
      connected.value = false
    }
  }

  function start() {
    if (!enabled) return
    fetchActiveAlerts()
    connect()
    pollTimer = window.setInterval(fetchActiveAlerts, 30000)
  }

  function stop() {
    if (pollTimer != null) {
      clearInterval(pollTimer)
      pollTimer = null
    }
    if (client?.active) {
      try {
        client.deactivate()
      } catch {
        // ignore
      }
    }
    client = null
    connected.value = false
  }

  /**
   * @param {string} machineIdRaw — phần machine_id (trước [CODE]) hoặc full value
   */
  function getLedsForMachineKey(machineIdRaw) {
    const k = normalize(machineIdRaw)
    if (!k) return null
    const st = ledByWorkCenter[k]
    if (!st) {
      return { led1: 'off', led2: 'off' }
    }
    return {
      led1: st['1'] || 'off',
      led2: st['2'] || 'off'
    }
  }

  return {
    enabled,
    connected,
    ledByWorkCenter,
    getLedsForMachineKey,
    start,
    stop,
    fetchActiveAlerts
  }
}
