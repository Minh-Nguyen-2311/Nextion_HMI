/*
This sketch shows how to use my tachometer made for the Nextion display.
I am not going to use any library to send data to the display.

Connection with Arduino Uno/Nano:
* +5V = 5V
* TX  = none
* RX  = pin 1 (TX)
* GND = GND

If you are going to use an Arduino Mega, you have to edit everything on this sketch that says "Serial"
and replace it with "Serial1" (or whatever number you are using).


Nextion library: https://github.com/itead/ITEADLIB_Arduino_Nextion


This sketch was made for my 4th video tutorial shown here: https://www.youtube.com/watch?v=eDn7LFyoEm8

Made by InterlinkKnight
Last update: 02/22/2018
*/



int TestVariable1 = 0;  // Create a variable to have something to show as a test
int TestVariable2 = 0;  // Create a variable to have something to show as a test
int TestVariable3 = 0;  // Create a variable to have something to show as a test
int TestVariable4 = 0;  // Create a variable to have something to show as a test



int sensorPin = A0;  // Potentiometer pin to simulate an rpm sensor, for testing
int sensorValue;  // Variable to store the value of potentiometer











// Calibration for smoothing RPM:
const int numReadings = 20;     // number of samples for smoothing. The higher, the more smoothing, but slower to react. Default: 20

// Calibration for tachometer deadzone:
const int TachometerDeadzoneSamples = 2;  // amount of samples for dead zone (1 would be no dead zone). Default: 2

// Calibration for tachometer limit:
const int TachLimitAmount = 20;  // how many cycles we are going to wait when tach reach limit value, before showing that value. Default: 20







// Variables for smoothing tachometer:
int readings[numReadings];  // the input
int readIndex = 0;  // the index of the current reading
long total = 0;  // the running total
int average = 0;  // the average

// Variables for deadzone for tachometer:
int TachometerWithDeadzone;

// Variables for tachometer limit:
int TachLimitCounter1 = 0;  // counter to wait for how long we see a limit value for tach, before showing that value
int TachLimitCounter2 = 0;  // counter to wait for how long we see a limit value for tach, before showing that value

// Variable to store the tachometer value after remaped:
int TachometerRemaped;

// Variable to store rpm:
int RealRPM = 0;










void setup() {  // Put your setup code here, to run once:
  
  Serial.begin(9600);  // Start serial comunication at baud=9600


  // I am going to change the Serial baud to a faster rate.
  delay(500);  // This dalay is just in case the nextion display didn't start yet, to be sure it will receive the following command.
  Serial.print("baud=115200");  // Set new baud rate of nextion to 115200, but it's temporal. Next time nextion is power on,
                                // it will retore to default baud of 9600.
                                // To take effect, make sure to reboot the arduino (reseting arduino is not enough).
                                // If you want to change the default baud, send the command as "bauds=115200", instead of "baud=115200".
                                // If you change the default baud, everytime the nextion is power ON is going to have that baud rate, and
                                // would not be necessery to set the baud on the setup anymore.
  Serial.write(0xff);  // We always have to send this three lines after each command sent to nextion.
  Serial.write(0xff);
  Serial.write(0xff);

  Serial.end();  // End the serial comunication of baud=9600

  Serial.begin(115200);  // Start serial comunication at baud=115200



}  // End of setup




void loop() {  // Put your main code here, to run repeatedly:


  delay(20);  // I put this delay because without it, the timer on the display would stop running.
              // Aparently we shouldn't send data to the display too often.




  //////////////// Testing variables:
  // The following it's going to change the variables in sequence as a test for the indicators and warning lights:
  // Ignore this as it has nothing to do with the tachometer itself.
  
  TestVariable1++;  // Add 1 to the TestVariable1

  if(TestVariable1 == 100)
  {
    TestVariable4--;
  }
  if(TestVariable4 < 0)
  {
    TestVariable4 = 100;
  }
  
  if(TestVariable1 > 100){
    TestVariable1 = 0;  // Set variable to 0 to start over

    if(TestVariable3 == 0)
    {
      TestVariable2++;
    }
    else
    {
      TestVariable2--;
    }
  }

  if(TestVariable2 == 8){  // If the variable is 8
    TestVariable3 = 1;
  }

  if(TestVariable2 == 0){  // If the variable is 0
    TestVariable3 = 0;
  }

  //////////////// End of testing variables.












  sensorValue = analogRead(sensorPin);  // Read analog pin where the potentiometer is connected
  RealRPM = map (sensorValue, 0, 1023, 0, 8000);  // Remap pot to simulate an RPM value
  RealRPM = constrain(RealRPM, 0, 8000);  // Constrain the value so it doesn't go below or above the limits


  int TachometerRemapedWithoutSmoothing = map (RealRPM, 0, 8000, 0, 208);  // Remap the raw RPM to match the tachometer value range
  TachometerRemapedWithoutSmoothing = constrain(TachometerRemapedWithoutSmoothing, 0, 208);  // Constrain the value so it doesn't go below or above the limits







  // Smoothing RPM:
  // subtract the last reading:
  total = total - readings[readIndex];
  // read speed:
  readings[readIndex] = RealRPM;  // takes the value we are going to smooth
  // add the reading to the total:
  total = total + readings[readIndex];
  // advance to the next position in the array:
  readIndex = readIndex + 1;

  // if we're at the end of the array...
  if (readIndex >= numReadings) {
    // ...wrap around to the beginning:
    readIndex = 0;
  }
  
  // calculate the average:
  average = total / numReadings;  // the average value it's the smoothed result







  
  TachometerRemaped = map (average, 0, 8000, 0, 208);  // Remap the smoothed RPM to match the tachometer value range
  TachometerRemaped = constrain(TachometerRemaped, 0, 208);  // Constrain the value so it doesn't go below or above the limits

             






  // Deadzone for tachometer:
  // This is another layer of smoothing the tachometer. It adds some dead zone so it doesn't go back and forward to the same
  // adjacent values. This is because RPM sensors have some error range and jump between different values for the same speed.
  TachometerWithDeadzone = TachometerWithDeadzone + ((TachometerRemaped - TachometerWithDeadzone)/TachometerDeadzoneSamples);

  // By putting dead zone on tachometer, the limits can't be reached anymore. Because of this we need to check min and max limits:
  // Min limit:
  if (TachometerRemaped == 0)  // if tachometer is 0
  {
    if(TachLimitCounter1 == TachLimitAmount)  // if we wait long enough and still is 0
    {
      TachometerWithDeadzone = 0;  // show real tach as 0
    }
    else  // since we didn't wait long enough if tachometer it's still 0
    {
      TachLimitCounter1++;  // count to wait if tachometer it's going to remain 0
    }
  }
  else  // since tachometer its not 0
  {
    TachLimitCounter1 = 0;  // reset counter
  }


  // Max limit:
  if (TachometerRemaped == 208)  // if tachometer is 208 (the maximun limit)
  {
    if(TachLimitCounter2 == TachLimitAmount)  // if we wait long enough and still is 208
    {
      TachometerWithDeadzone = 208;  // show real tach as 208
    }
    else  // since we didn't wait long enough if tachometer it's still 208
    {
      TachLimitCounter2++;  // count to wait if tachometer it's going to remain 208
    }
  }
  else  // since tachometer its not 208
  {
    TachLimitCounter2 = 0;  // reset counter
  }













  // Send tachometer value:
  Serial.print("tach.pic=");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.

  // Choose one of the folling two, on what value to send to the tachometer:
  //Serial.print(TachometerWithDeadzone);  // Send RPM smoothed and with deadzone
  Serial.print(TachometerRemapedWithoutSmoothing);  // Send RPM without any smoothing at all

  Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
  Serial.write(0xff);
  Serial.write(0xff);











  //////////////// Testing indicators:
  // The following it's going to change the indicators and warning lights as a test:
  // Ignore this as it has nothing to do with the tachometer itself.
  
  // Send fuel level:
  Serial.print("fuel.val=");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
  Serial.print(TestVariable4);  // This is the value you want to send to that object and atribute mentioned before.
  Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
  Serial.write(0xff);
  Serial.write(0xff);



  
  // Warning light test mode:
  if(TestVariable2 == 0)
  {
    Serial.print("vis check,0");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);

    Serial.print("vis bat,0");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);

    Serial.print("vis oil,0");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);

    Serial.print("temp.val=0");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);
  }


  if(TestVariable2 == 1)
  {
    Serial.print("vis check,1");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);

    Serial.print("vis bat,0");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);

    Serial.print("vis oil,0");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);

    Serial.print("temp.val=25");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);
  }


  if(TestVariable2 == 2)
  {
    Serial.print("vis check,1");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);

    Serial.print("vis bat,1");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);

    Serial.print("vis oil,0");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);

    Serial.print("temp.val=50");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);
  }


  if(TestVariable2 == 3)
  {
    Serial.print("vis check,1");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);

    Serial.print("vis bat,1");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);

    Serial.print("vis oil,1");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);

    Serial.print("temp.val=75");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);
  }


  if(TestVariable2 == 4)
  {
    Serial.print("vis check,0");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);

    Serial.print("vis bat,1");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);

    Serial.print("vis oil,1");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);

    Serial.print("temp.val=100");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);

  }


  if(TestVariable2 == 5)
  {
    Serial.print("vis check,0");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);

    Serial.print("vis bat,0");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);

    Serial.print("vis oil,1");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);

    Serial.print("temp.val=75");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);
  }


  if(TestVariable2 == 6)
  {
    Serial.print("vis check,0");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);

    Serial.print("vis bat,0");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);

    Serial.print("vis oil,0");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);

    Serial.print("temp.val=50");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);
  }


  if(TestVariable2 == 7)
  {
    Serial.print("vis check,0");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);

    Serial.print("vis bat,0");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);

    Serial.print("vis oil,0");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);

    Serial.print("temp.val=25");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);
  }


  if(TestVariable2 == 8)
  {
    Serial.print("vis check,0");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);

    Serial.print("vis bat,0");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);

    Serial.print("vis oil,0");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);

    Serial.print("temp.val=0");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.write(0xff);  // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);
  }

  //////////////// End of testing indicators.




    


}  // End of loop


