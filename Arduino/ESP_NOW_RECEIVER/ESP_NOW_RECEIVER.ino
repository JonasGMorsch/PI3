#include <esp_now.h>
#include <WiFi.h>
extern "C"
{
#include <esp_wifi.h>
}

#define CHANNEL 9

// The structure must match the receiver structure
typedef struct struct_message
{
  uint32_t adc_filtered_value;
  float adc_mediam;
} struct_message;


// Create a struct_message called myData
struct_message myData;

// Init ESP Now with fallback
void InitESPNow()
{
  WiFi.disconnect();
  if (esp_now_init() != ESP_OK)
    ESP.restart();
}

// config AP SSID
void configDeviceAP() {
  String Prefix = "Master:";
  String Mac = WiFi.macAddress();
  String SSID = Prefix + Mac;
  String Password = "123456789";
  //bool result = WiFi.softAP(SSID.c_str(), Password.c_str(), CHANNEL, 0);


  //  if (!result) {
  //    Serial.println("AP Config failed.");
  //  } else {
  //    Serial.println("AP Config Success. Broadcasting with AP: " + String(SSID));
  //  }
}

// callback when data  is recv from Master
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len)
{

  memcpy(&myData, data, data_len);

  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  //Serial.print("Last Packet Recv from: "); Serial.println(macStr);
  //  Serial.print("Received Data: ");
  //  Serial.print(myData.a);
  //  Serial.println();
  //  Serial.print(myData.adc_data);
  //  Serial.println();

  Serial.print("adc_top:2000");
  Serial.print("\t");
  Serial.print("adc_value:");
  Serial.print(myData.adc_filtered_value);
  Serial.print("\t");
  Serial.print("adc_mediam:");
  Serial.print(myData.adc_mediam);
  Serial.print("\t");
  Serial.println("adc_bottom:0");
}


hw_timer_t * cap_timer = NULL;

void setup()
{
  Serial.begin(2000000);
  //Set device in STA mode to begin with


  WiFi.mode(WIFI_AP);
  // configure device AP mode
  //configDeviceAP();
  WiFi.softAP("Master", "sdafagdgdfasdfgfjry", CHANNEL, 0);
  // This is the mac address of the Slave in AP Mode
  Serial.print("AP MAC: "); Serial.println(WiFi.softAPmacAddress());
  // Init ESPNow with a fallback logic
  InitESPNow();
  // Once ESPNow is successfully Init, we will register for Send CB to
  // get recv packer info.
  esp_now_register_recv_cb(OnDataRecv);


}

void loop()
{


}
