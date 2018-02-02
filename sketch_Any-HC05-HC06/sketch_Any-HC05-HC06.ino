
/*
The posible baudrates are (left AT-commands for HC-06, right AT-commands for HC-05)
AT+BAUD1------1200-----AT+UART:1200,0,0
AT+BAUD2------2400-----AT+UART:2400,0,0
AT+BAUD3------4800-----AT+UART:4800,0,0
AT+BAUD4------9600-----AT+UART:9600,0,0
AT+BAUD5-----19200-----AT+UART:19200,0,0
AT+BAUD6-----38400-----AT+UART:38400,0,0
AT+BAUD7-----57600-----AT+UART:57600,0,0
AT+BAUD8----115200-----AT+UART:115200,0,0
AT+BAUD9----230400--x--Careful: your serial port may not support this speed!
AT+BAUDA----460800--x--Careful: your serial port may not support this speed!
AT+BAUDB----921600--x--Careful: your serial port may not support this speed!
AT+BAUDC---1382400--x--Careful: your serial port may not support this speed!

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

// Set this variable to define the default behaviour:
int ac=1+2;   // use auto-commands: 0=off, 1=baud-detection, 2(bit1=1)=send-endless-commands, 4(bit2=1)=ask-master, 8(bit3=1)=ask-slave, 

SoftwareSerial BTserial(BT_RX, BT_TX); // RX, TX
long cnt=0, cmd=0, dto=0;
String str;
String res; // response
int nlcr=1; // need "\n\r" at end of AT-commands: 0=no, 1=yes, must be set to 0 for HC-06!
long baud_rate=38400; // the default baud rate to talk to the Bluetooth Module

void AutoCommands();

void setup()
{
  pinMode(BT_STATE, INPUT);
  pinMode(BT_PIN34, OUTPUT); digitalWrite(BT_PIN34, LOW);  // start in communication mode, only needed for HC-05
  pinMode(BT_EN,    OUTPUT); digitalWrite(BT_EN, HIGH); // enable device from start on

  // Select this same baud-rate on your Serial Monitor:
  //Serial.begin(9600);
  //Serial.begin(38400);
  //Serial.begin(57600);
  Serial.begin(115200);
  
  BTserial.begin(baud_rate);
}

void loop() // run over and over
{
  ++cnt; // increment loop-counter, allow overrun.
  if (ac) AutoCommands();
  
  if (BTserial.available())
  {
    str=BTserial.readString();
    res=str;
    str.trim();
    Serial.print(" from BT: "); Serial.print(str);
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
    Serial.print("\", @cmd="); Serial.print(cmd); Serial.print(", ");
    if (0 == MyCommands(tmp_str, "via Serial: "))
    {
      BTserial.print(str);
    }
  }
  delay(1);
}

void Details(String str)
{
   Serial.print("\nDetails @cnt="); Serial.println(cnt);
   for (int i=0; i<str.length(); ++i)
   {
      Serial.print("char[");Serial.print(i);Serial.print("]="); Serial.print((int)(str[i]));Serial.print(", ");
   }
   Serial.println("");
}
void NextBaudRate()
{
  // list only the rates which you want to have tested, terminate with a 0:
  // long rates[] = {1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600, 1382400, 0};
  long rates[] = {4800, 9600, 19200, 38400, 57600, 115200, 0};
  int i;
  for (i=0; rates[i]!=0; ++i)
  {
    if (rates[i] == baud_rate)
    {
      ++i; // use next baud-rate
      break;
    }  
  }
  if (rates[i] == 0)
    i = 0; // use first baud-rate
  Serial.print("Changing the Baud-Rate from "); Serial.print(baud_rate);
  Serial.print(" to "); Serial.print(rates[i]); Serial.println(" baud.");
  baud_rate = rates[i];
  BTserial.begin(baud_rate);
}
void SendCmd(String str)
{
  BTserial.print(str);
  if (nlcr)
    BTserial.println(""); // needed for HC-05
  Serial.print("\nSent@cnt="); Serial.print(cnt); Serial.print(": "); Serial.println(str);
  cmd=cnt; dto=0;
}
void AutoCommands()
{
  long dt=0;
  if (cnt==2) { Serial.println("OK, if you can read this your Baud-Rate is set correctly"); };
  if (cnt==2) { Serial.println("You can use the following commands:"); };
  if (cnt==2) { Serial.println("   help - print this overview"); };
  if (cnt==2) { Serial.println("   off  - set the EN (enable) pin to low"); };
  if (cnt==2) { Serial.println("   on   - set the EN (enable) pin to high"); };
  if (cnt==2) { Serial.println("   34hi - set the HC-05 Pin34 to high"); };
  if (cnt==2) { Serial.println("   34lo - set the HC-05 Pin34 to low"); };
  if (cnt==2) { Serial.println("   ac0  - turn auto commands off"); };
  if (cnt==2) { Serial.println("   ac1  - automatically detect the baud-rate of your Bluetooth Module"); };
  if (cnt==2) { Serial.println("   ac2  - send AT+VERSION to the module after a timeout-period"); };
  if (cnt==2) { Serial.println("   ac4  - send auto commands to turn your HC-05 into a master (unfinished)"); };
  if (cnt==2) { Serial.println("   ac8  - send auto commands to turn your HC-06 into a slave (unfinished)"); };
  if (cnt==2) { Serial.println("All other commands are directly sent to your module (e.g. AT-Commands)"); };
  if (ac&5 &&cnt==(dt+=1200)) { digitalWrite(BT_PIN34, HIGH);} // start AT-MODE, only needed for HC-05 }
  if (ac&1 &&cnt==(dt+=1200)) { nlcr=0; SendCmd("AT"); res="";}          // my HC-06 responds with "OK"
  if (ac&1 &&cnt==(dt+= 800)) { if (res!="OK") {nlcr=1; SendCmd(""); } } // my HC-05 responds with "OK\n\r"
  if (ac&1 &&cnt==(dt+=1200)) { if (res!="OK") {nlcr=0; SendCmd("AT"); } }
  if (ac&1 &&cnt==(dt+= 800)) { if (res!="OK") {nlcr=1; SendCmd(""); } }
  if (ac&1 &&cnt==(dt+=1200)) { Details(res); res.trim(); if (res!="OK") {NextBaudRate(); cnt=9;} }
  if (ac&1 &&cnt==(dt+=1200)) { Serial.print("The Baud-Rate is: "); Serial.print(baud_rate); Serial.println(" baud."); }
  if (ac&4 &&cnt==(dt+=1200)) { SendCmd("AT+VERSION"); } // my HC-05 responds with "+VERSION:2.0-20100601\n\rOK"
  if (ac&4 &&cnt==(dt+=1200)) { SendCmd("AT+PIN=12345"); }
  if (ac&4 &&cnt==(dt+=1200)) { SendCmd("AT+NAME=ASK-HC-05"); } // Set Name to ASK-HC-05
  if (ac&8 &&cnt==(dt+=1200)) { SendCmd("AT+VERSION"); } // my HC-06 responds with "OKlinvorV1.8"
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
  else if (msg == "ac0") { Serial.println(who+"Auto-Commands Off"); ac=1; }
  else if (msg == "ac1") { Serial.println(who+"Baud-Rate-Detection On"); ac=1; cnt=9; }
  else if (msg == "ac2") { Serial.println(who+"Send commands as fast as possible on timeout"); ac=2; cnt=9; }
  else if (msg == "ac4") { Serial.println(who+"Send all commands to turn a HC-05 into a master (unfinished)"); ac=4; cnt=9; }
  else if (msg == "ac8") { Serial.println(who+"Send all commands to turn a module into a slave (unfinished)"); ac=8; cnt=9; }
  else if (msg == "help") { Serial.println(who+"Help"); ac=0; cnt=0; }
  else   ret_val = 0;
  return ret_val;
}

