#include <esp_now.h>
#include <WiFi.h>
extern "C"
{
#include <esp_wifi.h>
}

// The structure must match the receiver structure
typedef struct struct_message
{
  uint32_t peak_hold_value;
  float adc_mediam;
} struct_message;

// Create a struct_message called myData
struct_message myData;

esp_now_peer_info_t receiver;

/////////////////////////////////////////////////////////////////////
const int Cap_GPIO = 32;
const int Cap_GPIO_Driver = 33;

bool hit = false;

#define RETUN_TIP 34
bool return_tip_coupled = false;

uint32_t return_tip_coupled_millis;

uint32_t return_tip_task;

uint32_t send_signal_task;
uint32_t return_tip_coupled_count;


volatile uint32_t guard_touched_millis;
volatile double adc_mediam = 1000;
volatile uint32_t peak_hold_value;
hw_timer_t * adc_timer = NULL;

volatile bool print = false;



//hw_timer_t * cap_timer = NULL;


////////////////////////////////////////////////////////////////////////////



void IRAM_ATTR ADCTimer()
{
  if (digitalRead(RETUN_TIP)) //needs to be before adc_task, it takes ~100uS to reach HIGH state.
    return_tip_coupled_count++;
  else
    return_tip_coupled_count = 0;

  //////////////// acd_value ////////////////
  pinMode(Cap_GPIO, INPUT);
  uint32_t adc_value = analogRead(Cap_GPIO);
  pinMode(Cap_GPIO, OUTPUT);
  digitalWrite(Cap_GPIO, HIGH);


  const register double lowpass_coefficient = 0.05;
  switch (return_tip_coupled_count)
  {
    case 0 :
      peak_hold_value = adc_value;

      if (adc_value > adc_mediam)
        adc_mediam += lowpass_coefficient;
      else
        adc_mediam -= lowpass_coefficient;
      break;

    case 1 ... 2:
      peak_hold_value = 0;
      break;

    case 3 ... 9:
      if (adc_value > peak_hold_value)
        peak_hold_value = adc_value;
      break;


    //    case 1 ... 2:
    //      peak_hold_value = 0;
    //      break;
    //    case 3 ... 9:
    //      peak_hold_value += adc_value;
    //      break;
    //
    //    case 10:
    //      peak_hold_value = peak_hold_value / 7;

    default :
      if (peak_hold_value > adc_mediam)
        guard_touched_millis = millis();

      if (millis() - guard_touched_millis > 40)
        hit = true;

      break;
  }


  print = true;
}


//void IRAM_ATTR ADCTimer()
//{
//  if (digitalRead(RETUN_TIP)) //needs to be before adc_task, it takes ~100uS to reach HIGH state.
//    return_tip_coupled_count++;
//  else
//    return_tip_coupled_count = 0;
//
//  //////////////// acd_value ////////////////
//  pinMode(Cap_GPIO, INPUT);
//  uint32_t adc_value = analogRead(Cap_GPIO);
//  pinMode(Cap_GPIO, OUTPUT);
//  digitalWrite(Cap_GPIO, HIGH);
//
//  //////////////// PEAK HOLD /////////////////
//  if (return_tip_coupled_count > 3)
//  {
//    if (adc_value > peak_hold_value)
//      peak_hold_value = adc_value;
//  }
//  else
//    peak_hold_value = adc_value;
//  ////////////////////////////////////////////
//
//
//  //////////// FAKE LOW PASS FILTER ////////////
//  if (return_tip_coupled_count == 0)
//  {
//    const register double lowpass_coefficient = 0.1;
//    if (adc_value > adc_mediam)
//      adc_mediam += lowpass_coefficient;
//    else
//      adc_mediam -= lowpass_coefficient;
//  }
//  /////////////////////////////////////////////
//
//  if (peak_hold_value > adc_mediam)
//    guard_touched_millis = millis();
//
//  if (return_tip_coupled_count > 10 && millis() - guard_touched_millis > 40)
//    hit = true;
//
//  print = true;
//}


void InitESPNow()
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  if (esp_now_init() != ESP_OK)
    ESP.restart();
}

void SenderLoop()
{
  static uint32_t receiver_not_found = true;
  if (receiver_not_found)
  {
    int8_t scanResults = WiFi.scanNetworks();
    for (int i = 0; i < scanResults; ++i)
    {
      if (WiFi.SSID(i).indexOf("Master") == 0)
      {
        esp_wifi_set_channel(WiFi.channel(i), WIFI_SECOND_CHAN_NONE);
        memcpy(receiver.peer_addr, WiFi.BSSID(i), 6);
        receiver.encrypt = false; // no encryption
        esp_now_add_peer(&receiver);
        receiver_not_found = false; // Pair success
      }
    }
  }
}

void setup()
{
  InitESPNow();
  Serial.begin(2000000);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  pinMode(RETUN_TIP, INPUT);

  adc_timer = timerBegin(0, 1, true);
  timerAttachInterrupt(adc_timer, &ADCTimer, true);
  timerAlarmWrite(adc_timer, 8000, true);
  timerAlarmEnable(adc_timer);


  //  cap_timer = timerBegin(0, 1, true);
  //  timerRead(cap_timer);

  //static uint64_t timer_value;
  //timer_get_counter_value(0,1,timer_value);



  //analogSetSamples(3);
  //analogSetCycles(8);
  //analogReadResolution(resolution)
  //analogSetWidth(width)
  //analogSetWidth(1);
  //analogSetClockDiv(100);

  //analogSetAttenuation(ADC_11db);
  //analogSetAttenuation(ADC_6db);
  //analogSetAttenuation(ADC_2_5db);
  //analogSetAttenuation(ADC_0db);
}


void loop()
{

  if (print)
  {
    myData.adc_mediam = adc_mediam;
    myData.peak_hold_value = peak_hold_value;
    esp_now_send(receiver.peer_addr, (uint8_t *) &myData, sizeof(myData));
    SenderLoop();
    print = false;
  }

  static uint32_t hit_millis;
  if (hit)
  {
    digitalWrite(LED_BUILTIN, LOW);
    hit_millis = millis();
    hit = false;
  }

  if ( digitalRead(LED_BUILTIN) == 0 && millis() - hit_millis > 100 )
  {
    digitalWrite(LED_BUILTIN, HIGH);
  }


  //  if (print)
  //  {
  //    Serial.print("adc_filtered_value:");
  //    Serial.print(adc_filtered_value);
  //    Serial.print("\t");
  //    Serial.print("adc_mediam:");
  //    Serial.println(adc_mediam);
  //    print = false;
  //  }
}
