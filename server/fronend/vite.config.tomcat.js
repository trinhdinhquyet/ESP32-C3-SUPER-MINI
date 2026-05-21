import { fileURLToPath, URL } from 'node:url'
import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'

// Cấu hình cho Tomcat subdirectory
export default defineConfig({
  plugins: [vue()],
  resolve: {
    alias: {
      '@': fileURLToPath(new URL('./src', import.meta.url))
    },
  },
  // Cấu hình base path cho subdirectory /dwm/
  base: '/dwm/',
  build: {
    outDir: 'dist',
    assetsDir: 'assets',
    sourcemap: false,
    minify: 'esbuild', // Sử dụng esbuild thay vì terser
    rollupOptions: {
      input: {
        main: fileURLToPath(new URL('./index.html', import.meta.url))
      },
      output: {
        manualChunks: {
          vendor: ['vue'],
          charts: ['echarts', 'vue-echarts']
        }
      }
    }
  }
})
