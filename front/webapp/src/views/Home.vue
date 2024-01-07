<template>
  <v-container fluid full-height>
    <v-row> 
      <v-col>
        <template v-if="values.length != 0">

              <template v-for="(item,index) in values" :key="index">

                <v-card> 
                  <v-card-title>Sensor {{ index+1 }}</v-card-title>
                  <v-card-subtitle>{{ item.SensorType }}</v-card-subtitle>
                  <v-card-text>
                    <v-table density="compact">

                      <thead><tr>
                      <th class="text-left" style="width:33%">Item</th>
                      <th class="text-left" style="width:33%">Value</th> 
                      <th class="text-left" style="width:33%">Unit</th>
                      </tr></thead>

                      <tbody>
                      <template v-for="(my_val,val_index) in item" :key="val_index">

                      <tr>
                        <td class="grey--text">{{my_val.text}}</td> 
                        <td>{{my_val.value}}</td>
                        <td class="grey--text">{{my_val.unit}}</td>                         
                      </tr>
                      

                      </template></tbody>

                    </v-table>
                  </v-card-text>
                </v-card>

              </template>
        </template>

        <template v-else>

              <v-card title="Wait....." loading> 
                <v-card-text>
                  <p>Loading sensor data. Please wait.</p>
                </v-card-text>
              </v-card>

        </template>        
      </v-col>
    </v-row>
  </v-container>
</template>

<script>

import axios from 'axios'

export default {
  data() {
    return {
      sensorcnt: null,
      values: [],
      loaded: [],
      timer: null
    };
  },
  
  // ---- cleanup the timer object

  destroyed()
  {
    clearInterval(this.timer);
  },

  // ---- define some custom methods

  methods: 
  {
    // ---- this one calls the AJAX functions and updates 

    updateData: function() 
    {
           axios
          .get("/api/v1/sensorcnt")
          .then(cnt_call_result => {

            this.sensorcnt  = cnt_call_result.data.cnt;

            var i;
            for (i = 0; i < this.sensorcnt; i++) 
            { 
                //console.log("Send request %d",i)
                var urlidx = i+1;
    
                
                     axios.get("/api/v1/air/" + urlidx.toString(),{ id: i})
                          .then(sensor_call_result => {
                                          this.values[sensor_call_result.config.id] = sensor_call_result.data;
                                          this.loaded[sensor_call_result.config.id] = true;
                                          this.$forceUpdate();

                                          //console.log("Got Values %d",sensor_call_result.config.id);   

                                          //console.log(sensor_call_result);
                                      }
                                )
                          .catch(error => {
                                          console.log(error);
                                        }
                                );
            }
          })
          .catch(error => {
            console.log(error);
          });    
    }
  }, 

  // ---- setup the timer

  mounted() 
  {
      clearInterval(this.timer);
      this.timer = setInterval(this.updateData , 1000);
  }
};
</script>
