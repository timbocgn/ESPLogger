<script setup>

import { getCurrentInstance,ref } from "vue"
import axios from 'axios'

var items = ref()

// ---- this function is called periodically to get the log file contents

function updateData()
{
  axios.get("/api/v1/log/idx-0/cnt-0").then(ret_value => {
    
    // --- just copy the array of log items we received via json directly to the items ref instance
    //     v-table is updated automatically by vue.js

    items.value = ret_value.data.log_entries
 
})
} 

// --- we will use this to get the backend version asynchronously 

var backend_version = ref()
var backend_version_loaded = ref(false)

// --- we get the app version from the global properties - see main.js for explanation

var app_version = ref()
app_version = getCurrentInstance().appContext.app.config.globalProperties.appVersion

// --- ask the backend using the version API for the backend version strings
//     the promise updates the backend_version variable and vue does the rest

axios.get("/api/v1/version").then(ret_value => {
  backend_version.value = ret_value.data
  backend_version_loaded.value = true
})

// --- setup a timer for the log file update

var timer
clearInterval(timer);
timer = setInterval(updateData , 1000);

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
                <template v-if="getCurrentInstance().appContext.app.config.globalProperties.appVersion">
                    <tr>
                      <td class="grey--text">web_app_version</td> 
                      <td>{{ app_version.version }}</td>                 
                  </tr>
                  <tr>
                      <td class="grey--text">vuetify_version</td> 
                      <td>{{ app_version.dependencies.vuetify.substring(1) }}</td>
                  </tr>
                </template>
                
                <tr>
                  <td class="grey--text">vue_js_version</td> 
                  <td>{{ getCurrentInstance().appContext.app.version }}</td>
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

    <v-row> 
      <v-col>
        <v-card title="Logfile"> 
          <v-card-text>
            <v-data-table :items="items" density='compact' items-per-page="-1">

            </v-data-table>
          </v-card-text>

        </v-card>
      </v-col>
    </v-row>

  </v-container>
</template>


