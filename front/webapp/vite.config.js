import { fileURLToPath, URL } from 'node:url'

import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import packageJson from './package.json';

import viteCompression from 'vite-plugin-compression';

// https://vitejs.dev/config/


export default defineConfig({
  plugins: [
    vue(),viteCompression({ deleteOriginFile: true})
  ],
  resolve: {
    alias: {
      '@': fileURLToPath(new URL('./src', import.meta.url))
    }
  },

  // --- make package.json accessible in the app using __APP_VERSION__. vite will add this to the global properties or 
  //     replace the string during production build. See main.js how we use this to stop it in the global config
  //     https://stackoverflow.com/questions/67194082/how-can-i-display-the-current-app-version-from-package-json-to-the-user-using-vi
  //     https://github.com/vuejs/vue-next-webpack-preview/issues/15

  define:  {
    __APP_VERSION__: JSON.stringify(packageJson)
  },

  // --- this here redirects the api calls of the npm dev server to one running instance 
  //     of an esplogger device (which happens to be at this ip in my network)

  server: {
    proxy: {
      '/api': 'http://192.168.1.126:80',
      '/upload': 'http://192.168.1.126:80',    },
  },
})
