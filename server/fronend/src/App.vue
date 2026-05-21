<script setup>
import HeartRateDashboard from './components/HeartRateDashboard.vue'
</script>

<template>
  <div class="app-container">
    <HeartRateDashboard />
  </div>
</template>

<style>
/* Global Reset */
html, body {
  margin: 0;
  padding: 0;
  width: 100%;
  height: 100%;
  background-color: #0b0f19;
  font-family: 'Outfit', 'Inter', system-ui, -apple-system, sans-serif;
  color: #f3f4f6;
  overflow-x: hidden;
}

#app {
  width: 100%;
  height: 100%;
}

.app-container {
  width: 100%;
  min-height: 100vh;
}
</style>
