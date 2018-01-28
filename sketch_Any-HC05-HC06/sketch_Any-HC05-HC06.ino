/*
The posible baudrates are (for HC-06):
1-------1200
2-------2400
3-------4800
4-------9600
5------19200
6------38400
7------57600
8-----115200
9-----230400
A-----460800
B-----921600
C----1382400


*** Important. If you re-set the BT baud rate, you must change the BTserial baud to match what was just set. Steps:

Assuming the BT module baud default is 9600 Baud set BTserial to 9600 and you want to change it to 57600.
1) leave the line:  //BTserial.begin(9600); as you have to been at the current baud setting to be able to send the command to the BT module.
2) uncomment the line:  //BTserial.print("AT+BAUD7"); // Set baudrate to 57600
3) run the sketch.  This will set the baud to 57600.  However, you may not see the response OK. 
4) uncomment the line //BTserial.begin(57600); and download the sketch again - this sets BTserial to the new setting.
5) now that BTserial is set to 57600 open the serial monitor - this will restart the sketch.  You should see a secomd line after "Goodnight Moon"
that will show the BT module responding to the change.
6) be sure to re-comment the line //BTserial.print("AT+BAUD7"); when done

*/
#include <SoftwareSerial.h>

// Connect HC05/HC-06-GND to Arduino GND
// Connect HC05/HC-06-+5V to Arduino +5V
#define BT_EN    2  // Connect the HC-05/HC-06 EN to Arduino pin D2, ENABLE, LOW=disable, NOTE: Connect through a voltage divider!
#define BT_RX    3  // Connect the HC-05/HC-06 TX to Arduino pin D3, (Achtung: Arduino-RX == HC-05-TX)
#define BT_TX    4  // Connect the HC-05/HC-06 RX to Arduino pin D4, Arduino-Tx=HC-05-Rx, NOTE: Connect through a voltage divider!.
#define BT_STATE 5  // Connect the HC-05/HC-06 State to Arduino pin D5, LOW means: HC-05 is connected
#define BT_PIN34 6  // Connect only the HC-05 Pin34 (=PIO11) to Arduino pin D6, HIGH means same as pressing the button, NOTE: Connect through a voltage divider!

void AutoCommands();

SoftwareSerial BTserial(BT_RX, BT_TX); // RX, TX
long cnt=0, cmd=0, dto=0;
String str;
int ac=1+2+4;   // use auto-commands: 0=off, 1=baud-detection, 2(bit1=1)=send-endless-commands, 4(bit2=1)=ask-master, 8(bit3=1)=ask-slave, 
int nlcr=1; // need "\n\r" at end of AT-commands: 0=no, 1=yes, must be set to 0 for HC-06!

void setup()
{
  pinMode(BT_STATE, INPUT);
  // comment out one of the two following lines to start either in AT-mode or in communication mode:
  pinMode(BT_PIN34, OUTPUT); digitalWrite(BT_PIN34, LOW);  // start in communication mode, only needed for HC-05
  pinMode(BT_EN,    OUTPUT); digitalWrite(BT_EN, HIGH); // enable device from start on
  
  //Serial.begin(9600);
  //Serial.begin(57600);
  Serial.begin(115200);
  //BTserial.begin(9600);
  BTserial.begin(38400);
  //BTserial.begin(57600);
  //BTserial.begin(115200);    //if you change the baud and want to re-run this sketch, make sure this baud rate matches the new rate.

  str="AT";         delay(1000); BTserial.print(str); Serial.print("INIT: "); Serial.println(str);
  //str="AT+VERSION"; delay(1000); BTserial.print(str); Serial.print("INIT: "); Serial.println(str);
  
  //delay(1000); BTserial.print("AT+BAUD4"); Serial.println("Set baudrate to 9600"); // comment out this line when done!
  //delay(1000); BTserial.print("AT+BAUD6"); Serial.println("Set baudrate to 38400"); // comment out this line when done!
  //delay(1000); BTserial.print("AT+BAUD7"); Serial.println("Set baudrate to 57600"); // comment out this line when done!
  //delay(1000); BTserial.print("AT+BAUD8"); Serial.println("Set baudrate to 115200"); // comment out this line when done!
}

void loop() // run over and over
{
  ++cnt; // increment loop-counter, allow overrun.
  if (ac) AutoCommands();
  
  if (BTserial.available())
  {
    str=BTserial.readString();
    str.trim();
    Serial.print(str);
    for (int i=0; i<str.length(); ++i)
    {
      if (str[i] == 'x')
      {
        Serial.println("-X-");
        BTserial.println("-X-");
      }
    }
    Serial.print(", @cnt="); Serial.print(cnt); Serial.print(", delta="); Serial.print(cnt-cmd-dto); Serial.print(" ");
    MyCommands(str, "via BT: ");
  }
  if (Serial.available())
  {
    String tmp_str;
    str=Serial.readString();
    // Copy the serial data back to to the serial monitor.
    // This makes it easy to follow the commands and replies
    Serial.println(""); // make a new-line
    Serial.print("You: \"");
    tmp_str=str; tmp_str.trim(); Serial.print(tmp_str);
    cmd=cnt; dto=0;
    Serial.print("\", @cmd="); Serial.print(cmd); Serial.print(", HC-06: ");     
    if (0 == MyCommands(tmp_str, "Serial: "))
    {
      BTserial.print(str);
    }
  }
  delay(1);
}

void SendCmd(String str)
{
  if (nlcr)
    BTserial.println(str);
  else
    BTserial.print(str);
  Serial.print("\nSent: ");
  Serial.println(str);
  cmd=cnt; dto=0;
}
void AutoCommands()
{
  long dt=0;
  if (ac&4 &&cnt==(dt+=1200)) { BTserial.println("");} // start AT-MODE, only needed for HC-05 }
  if (ac&4 &&cnt==(dt+=1200)) { digitalWrite(BT_PIN34, HIGH);} // start AT-MODE, only needed for HC-05 }
  if (ac&1 &&cnt==(dt+=1200)) { SendCmd("AT"); } // my HC-06 responds with "OK"
  if (ac&12&&cnt==(dt+=1200)) { SendCmd("AT+VERSION"); } // my HC-06 responds with "OKlinvorV1.8"
  if (ac&4 &&cnt==(dt+=1200)) { SendCmd("AT+PIN=1234"); }
  if (ac&4 &&cnt==(dt+=1200)) { SendCmd("AT+NAME=ASK-HC-05"); } // Set Name to ASK-HC-05
  if (ac&8 &&cnt==(dt+=1200)) { SendCmd("AT+PIN1234"); }
  if (ac&8 &&cnt==(dt+=1200)) { SendCmd("AT+NAMEVIN-HC-06"); } // Set Name to VIN-HC-06
  
  // NOTE: the next line will send a command every 1.2 seconds, actually the HC-06 should allow a command every second, but it doesn't!
  if (ac&2 &&cnt>=(cmd+dt+1200)) { str="AT+VERSION"; if (nlcr) BTserial.println(str); else BTserial.print(str); 
    Serial.print("type ac0 to stop!\nSent: "); str.trim(); Serial.print(str); Serial.print(", Rx: ");
    cmd=cnt-dt; dto=dt; // communicate as fast as possible (as fast as allowed) with HC-06
    if (nlcr==1) BTserial.print("\n\r");
  }
}

int MyCommands(String msg, String who)
{
  int ret_val = 1; // 0 == unused
  msg.trim();
  who = who+msg+" ==> ";
  if (msg == "off")     { Serial.println(who+"BT_EN->LOW");
    BTserial.println("bye."); delay(50);
    digitalWrite(BT_EN, LOW);  // turn BT-Module off
  }
  else if (msg == "on") { Serial.println(who+"BT_EN->HIGH");
    digitalWrite(BT_EN, HIGH); // turn BT-Module on
  }
  else if (msg == "34hi") { Serial.println(who+"Pin34->HIGH");
    digitalWrite(BT_PIN34, HIGH); // simulate press of the button, turn AT-mode on
  }
  else if (msg == "34lo") { Serial.println(who+"Pin34->LOW");
    digitalWrite(BT_PIN34, LOW); // simulate release of the button
  }
  else if (msg == "ac0") { Serial.println(who+"Auto-Commands Off"); ac=0; }
  else if (msg == "ac1") { Serial.println(who+"Baud-Rate-Detection On"); ac=1; cnt=0; }
  else if (msg == "ac2") { Serial.println(who+"Send commands as fast as possible on timeout"); ac=2; cnt=0; }
  else   ret_val = 0;
  return ret_val;
}

