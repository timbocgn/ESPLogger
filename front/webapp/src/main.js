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
import { aliases, mdi } from 'vuetify/iconsets/mdi-svg'
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

// ---- this copies information from the package.json to the global properties of the app object
//      set vite.config.js where these info comes from and About.vue how to access it

app.config.globalProperties.appVersion = __APP_VERSION__

//console.log("App Version Information")
//console.log(__APP_VERSION__)
