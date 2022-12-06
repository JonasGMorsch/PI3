
void setup() 
{
  Serial.begin(250000);
}

void loop() 
{
  for(int i = 0; i < 360; i += 5) {
    float y1 = 1 * sin(i * M_PI / 180);
    float y2 = 2 * sin((i + 90)* M_PI / 180);
    float y3 = 5 * sin((i + 180)* M_PI / 180);

    Serial.print(y1);
    Serial.print("\t"); // a space ' ' or  tab '\t' character is printed between the two values.
    Serial.print(y2);
    Serial.print("\t"); // a space ' ' or  tab '\t' character is printed between the two values.
    Serial.println(y3); // the last value is followed by a carriage return and a newline characters.

    delay(100);
  }
}
