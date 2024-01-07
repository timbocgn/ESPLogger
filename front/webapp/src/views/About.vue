<script setup>

import { getCurrentInstance,ref } from "vue"
import axios from 'axios'

var backend_version = ref()
var backend_version_loaded = ref(false)

axios.get("/api/v1/version").then(ret_value => {
  backend_version.value = ret_value.data
  backend_version_loaded.value = true
})

</script>

<template>
  <v-container fluid full-height>
    <v-row> 
      <v-col>
        <v-card title="About way2.net ESP Logger"> 
          <v-card-text>
            <p>Welcome to the ESP32 based versatile logging device from way2.net services.</p>

            <p>It works in conjunction with the VINDRIKTNING air sensor from IKEA which can be hacked to become an 
              IoT by this or by an SHT1x sensor which reports temperature and humidity.</p>

            <p>It features a bootstrapping mechanism opening a local wireless hotspot for connecting it to your
              favorite wireless lan and has also an MQTT provider build in, which you can use to log the data.
            </p>
            <p>You also can leverage the API calls to configure the device and get the sensor data to your favorite 
              application.</p>

            <v-divider></v-divider>
            <br>

            <p> This application is provided as public domain - feel free to use in any kind of project. </p>

          </v-card-text>
          <v-card-actions>
            <v-btn href="https://www.way2.net">way2.net Homepage</v-btn>
          </v-card-actions>
        </v-card>
      </v-col>
    </v-row>
    <v-row> 
      <v-col>
        <v-card title="System Information"> 
          <v-card-text>
            <v-table density="compact">

              <thead><tr>
                <th class="text-left" style="width:33%">Item</th>
                <th class="text-left" style="width:33%">Value</th> 
              </tr></thead>

              <tbody>
                <tr>
                  <td class="grey--text">Web App Version</td> 
                  <td>{{ getCurrentInstance().appContext.app.config.globalProperties.appVersion.version }}</td>
                </tr>
                <tr>
                  <td class="grey--text">Web App vue.js Version</td> 
                  <td>{{ getCurrentInstance().appContext.app.version }}</td>
                </tr>
                <tr>
                  <td class="grey--text">Web App vuetify Version</td> 
                  <td>{{ getCurrentInstance().appContext.app.config.globalProperties.appVersion.dependencies.vuetify.substring(1) }}</td>
                </tr>
                
                <template v-if="backend_version_loaded == true">

                  <template v-for="(my_val,val_index) in backend_version" :key="val_index">

                    <tr>
                      <td>{{val_index}}</td>
                      <td class="grey--text">{{my_val}}</td> 
                    </tr>

                  </template>

                </template>

                
              </tbody>

              </v-table>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>
  </v-container>
</template>


