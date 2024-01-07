import './assets/main.css'

import { createApp } from 'vue'
import App from './App.vue'
import router from './router'

//Vue.config.productionTip = false

// ---- some advanced input mask support (npm i vue-the-mask)

import VueTheMask from 'vue-the-mask'

// ---- Vuetify: Our basic app skeleton

import 'vuetify/styles'
import { createVuetify } from 'vuetify'
import { aliases, mdi } from 'vuetify/iconsets/mdi'
import * as components from 'vuetify/components'
import * as directives from 'vuetify/directives'

const vuetify = createVuetify({
  components,
  directives,
  icons: {
    defaultSet: 'mdi',
    aliases,
    sets: {
      mdi,
    },
  }
}
)

// ---- now build the app object

const app = createApp(App)

app.use(router)
app.use(vuetify)
app.use(VueTheMask)
app.mount('#app')
