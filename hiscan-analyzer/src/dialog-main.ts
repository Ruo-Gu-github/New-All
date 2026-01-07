import { createApp } from 'vue';
import ElementPlus from 'element-plus';
import 'element-plus/dist/index.css';
import RigMarkDialog from './components/dialogs/RigMarkDialog.vue';

const app = createApp(RigMarkDialog);
app.use(ElementPlus);
app.mount('#app');
