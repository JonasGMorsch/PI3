
const int Cap_GPIO = 32;

bool hit = false;

#define RETUN_TIP 34
bool return_tip_coupled = false;

uint32_t return_tip_coupled_millis;

uint32_t return_tip_task;

uint32_t send_signal_task;
uint32_t return_tip_coupled_count;


volatile uint32_t guard_touched_millis;
volatile double adc_mediam = 1150;
volatile double adc_filtered_value;
hw_timer_t * adc_timer = NULL;

volatile bool print = false;





void IRAM_ATTR ADCTimer()
{
  if (analogRead(RETUN_TIP) > 2048) //need to be before adc_task, it takes ~100uS to reach HIGH state.
    return_tip_coupled_count++;
  else
    return_tip_coupled_count = 0;

  //digitalWrite(Cap_GPIO, LOW);

  pinMode(Cap_GPIO, INPUT);
  int32_t adc_value = analogRead(Cap_GPIO);

  pinMode(Cap_GPIO, OUTPUT);
  digitalWrite(Cap_GPIO, HIGH);

  adc_filtered_value = ((adc_value - adc_filtered_value) / 1) + adc_filtered_value;

  if (return_tip_coupled_count == 0)
    adc_mediam = ((adc_value - adc_mediam) / 500) + adc_mediam;

  if (adc_filtered_value - adc_mediam > 35)
    guard_touched_millis = millis();

  print = true;
}
void setup()
{

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RETUN_TIP, INPUT);

  Serial.begin(2000000);

  adc_timer = timerBegin(0, 80, true);
  timerAttachInterrupt(adc_timer, &ADCTimer, true);
  timerAlarmWrite(adc_timer, 1000, true);
  timerAlarmEnable(adc_timer);
}
void loop()
{
  //  if (print)
  //  {
  //    Serial.print("adc_filtered_value:");
  //    Serial.print(adc_filtered_value);
  //    Serial.print("\t");
  //    Serial.print("adc_mediam:");
  //    Serial.println(adc_mediam);
  //    print = false;
  //  }

  delay(1000);
  Serial.println(return_tip_coupled_count);



  //  static uint32_t serial_task;
  //  if (( millis() - serial_task) > 50)
  //  {
  //    //Serial.print("adc_filtered_value: ");
  //    Serial.println(adc_filtered_value);
  //    //Serial.print("adc_mediam: ");
  //    //Serial.println(adc_mediam);
  //    //Serial.println(return_tip_coupled_count);
  //    serial_task = millis();
  //  }




  //  static uint32_t send_signal_task;
  //  static uint32_t hit_task;
  //
  //  if (millis() - send_signal_task > 1)
  //  {
  //
  //    //if (millis() - return_tip_coupled_millis > 2  && millis() - return_tip_coupled_millis < 100 && millis() - guard_touched_millis > 200 )
  //    if (return_tip_coupled_count > 10 && millis() - guard_touched_millis > 200 )
  //    {
  //      digitalWrite(LED_BUILTIN, HIGH);
  //      hit = true;
  //      hit_task = millis();
  //    }
  //    send_signal_task = millis();
  //  }
  //
  //  if (hit && ( (millis() - hit_task) > 200) )
  //  {
  //    hit = false;
  //    digitalWrite(LED_BUILTIN, LOW);
  //  }

}
