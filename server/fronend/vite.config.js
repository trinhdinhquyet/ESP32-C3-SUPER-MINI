import { fileURLToPath, URL } from 'node:url'
import { defineConfig, loadEnv } from 'vite'
import vue from '@vitejs/plugin-vue'

// https://vite.dev/config/
export default defineConfig(({ mode }) => {
  const env = loadEnv(mode, process.cwd(), '')

  return {
    define: {
      global: 'globalThis'
    },
    optimizeDeps: {
      esbuildOptions: {
        define: {
          global: 'globalThis'
        }
      }
    },
    plugins: [
      vue(),
    ],
    resolve: {
      alias: {
        '@': fileURLToPath(new URL('./src', import.meta.url))
      },
    },
    build: {
      rollupOptions: {
        input: {
          main: fileURLToPath(new URL('./index.html', import.meta.url))
        }
      }
    },
    server: {
      host: '0.0.0.0', // Cho phép truy cập từ bên ngoài
      port: 3005, // Port mặc định
      open: true, // Tự động mở browser
      proxy: {
        '/api': {
          target: env.VITE_API_BASE_URL,
          changeOrigin: true,
          rewrite: (path) => {
            // Machine list API - giữ nguyên path
            if (path.startsWith('/api/machine-list')) {
              return path // Giữ nguyên /api/machine-list
            }
            // Server time - giữ nguyên path (API endpoint mới)
            if (path.startsWith('/api/server-time')) {
              return path // Giữ nguyên /api/server-time
            }
            // ESP32 Heart Rate API - giữ nguyên path
            if (path.startsWith('/api/esp32')) {
              return path // Giữ nguyên /api/esp32
            }
            // Machine models API - map to /dwm-Dashboard/machine-models
            if (path.startsWith('/api/machine-models')) {
              return '/dwm-Dashboard/machine-models'
            }
            // Dashboard APIs - map to /dwm-Dashboard
            return path.replace(/^\/api/, '/dwm-Dashboard')
          }
        }
      }
    }
  }
})
