<script setup>

import { RouterLink, RouterView } from 'vue-router'

import { mdiMenu } from '@mdi/js'
import { mdiHomeCircle } from '@mdi/js'
import { mdiAccountSettings } from '@mdi/js'
import { mdiHelpCircle } from '@mdi/js'


const items = 
        [
          { title: 'Home', icon: mdiHomeCircle, to: '/' },
          { title: 'Config', icon: mdiAccountSettings, to:'config' },
          { title: 'About', icon: mdiHelpCircle, to:'about' }
        ];
          
const iconMenu = mdiMenu;

</script>

<template>
  <v-app id="esploggerapp">

    <v-app-bar
      color="grey accent-4"
      app
      dark
      short
      clipped-left
    >
      <v-app-bar-nav-icon @click.stop="drawer = !drawer"><v-icon>{{ iconMenu }}</v-icon></v-app-bar-nav-icon>
      <v-toolbar-title>way2.net ESP Logger</v-toolbar-title>
    </v-app-bar>

    <v-navigation-drawer app  permanent>
      <v-list density="compact" nav>
        <v-list-item
          v-for="item in items"
          :title="item.title"
          :to="item.to"
          :prepend-icon="item.icon"
        >
        </v-list-item>

      </v-list>
    </v-navigation-drawer>
   
 
    

      <v-main class="d-flex align-center justify-center" >

        <router-view></router-view>
    </v-main>
   
    <v-footer color="grey accent-4" app fixed>
      <span class="white--text"> &copy; 2024 way2.net Services</span>
    </v-footer>
  </v-app>
</template>


<style >

</style>
