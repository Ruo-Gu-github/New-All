import { createApp, nextTick } from 'vue'
import ElementPlus from 'element-plus'
import 'element-plus/dist/index.css'
import './style.css'
import App from './App.vue'

const app = createApp(App)

app.use(ElementPlus)

app.mount('#app')

nextTick(() => {
  window.ipcRenderer?.on('main-process-message', (_event, message) => {
    console.log(message)
  })
})
