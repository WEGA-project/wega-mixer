void start_mdns_service()
 {

     esp_err_t err = mdns_init();
     if (err) {
         printf("MDNS Init failed: %d\n", err);
         return;
     }
     mdns_hostname_set("wega-mixer-esp32");
     mdns_instance_name_set("wega-mixer-esp32");
 }


 bool is_closed(uint8_t i){
 
    return !subscription[i].connected();
 
}


void set_sync_client(WiFiClient client){
   
}

void mdns_update(){

}

