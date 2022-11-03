
void start_mdns_service(){
  MDNS.begin("mixer");
  MDNS.addService("http", "tcp", 80);
}


bool is_closed(uint8_t i){
    return subscription[i].status() == CLOSED;
}


void set_sync_client(WiFiClient client){
     client.setSync(true);
}

void mdns_update(){
    MDNS.update();  
}
