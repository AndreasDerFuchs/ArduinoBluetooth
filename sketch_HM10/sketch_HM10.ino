
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

// Set these variables to define the default behaviour:
int ac=1; // Use auto-commands: 0=off, 1=baud-detection, 2(bit1=1)=send-endless-commands, 4(bit2=1)=ask-master, 8(bit3=1)=ask-slave, 
long baud_rate=38400; // The (initial) default baud rate to talk to the Bluetooth Module

SoftwareSerial BTserial(BT_RX, BT_TX); // RX, TX
long cnt=0, cmd=0, dto=0, hc=2, answer=0;
int cmd_via_BT=0;
String str;
String res_f, res_t; // response, full and trimmed (without white-space)
int nlcr=3; // need "\r\n" at end of AT-commands: 0,(2)=no, 1,(3)=yes, in brackets: guessed. Must be 0 for HC-06!
bool is_enabled=false;
bool is_connected=false;

void AutoCommands();

void setup()
{
  pinMode(BT_STATE, INPUT);
  pinMode(BT_PIN34, OUTPUT); digitalWrite(BT_PIN34, LOW);  // start in communication mode, only needed for HC-05
  pinMode(BT_EN,    OUTPUT); digitalWrite(BT_EN, HIGH); is_enabled=true;// enable device from start on

  // Select this same baud-rate on your Serial Monitor:
  //Serial.begin(9600);
  //Serial.begin(38400);
  //Serial.begin(57600);
  Serial.begin(115200);
  
  BTserial.begin(baud_rate);
}

void PrintCnt()
{
  Serial.print(", @cnt="); Serial.print(cnt);
}
void PrintlnCnt()
{
  PrintCnt(); Serial.println("");
}
void SetCnt(long new_cnt)
{
  PrintCnt();
  cnt=new_cnt;
  Serial.print("->"); Serial.println(cnt);
}
void CheckIfConnected()
{
  if (is_enabled && digitalRead(BT_STATE) != is_connected)
  {
    is_connected = !is_connected; // toggle connection state
    Serial.print("\nNew STATE="); Serial.print(is_connected); PrintlnCnt();
  }
}

void loop() // run over and over
{
  String tmp_str;
  ++cnt; // increment loop-counter, allow overrun.
  CheckIfConnected();
  if (ac) AutoCommands();
  CheckIfConnected();
  
  if (BTserial.available())
  {
    str=BTserial.readStringUntil('\n');
    res_f=str;
    res_t=str; res_t.trim();
    str.trim();
    if (answer > cmd || (cnt-cmd-dto) > 2000) Serial.println("."); // e.g. multiline answer or we got a new line from remote
    answer = cnt;
    Serial.print(", from BT: \"");
    tmp_str=str; tmp_str.replace("\n","\\n"); tmp_str.replace("\r","\\r"); Serial.print(tmp_str+"\"");
#if 1 // enable this to test communication with e.g. a smartphone. for every x the response will be -X-
    for (int i=0; i<str.length(); ++i)
    {
      if (str[i] == 'x')
      {
        Serial.println("-X-");
        BTserial.println("-X-");
      }
    }
#endif
    PrintCnt(); Serial.print(", delta="); Serial.print(cnt-cmd-dto); Serial.print(" ");
    if (MyCommands(str, "via BT: "))
    {
      cmd_via_BT=1; // send extra /n in Auto-Command ac2
    }
  }
  if (Serial.available())
  {
    str=Serial.readString();
    // Copy the serial data back to to the serial monitor.
    // This makes it easy to follow the commands and replies
    Serial.println(""); // make a new-line
    Serial.print("You: \"");
    tmp_str=str; tmp_str.replace("\n","\\n"); tmp_str.replace("\r","\\r"); Serial.print(tmp_str+"\", ");
    if (tmp_str==str && nlcr==1)
      Serial.print("\nWARNING: Your Device expects NL CR, change your terminal setting! ");
    if (tmp_str!=str && nlcr==0)
      Serial.print("\nWARNING: Your Device expects no NL and no CR, change your terminal setting! ");
    cmd=cnt; dto=0;
    Serial.print("@cmd="); Serial.print(cmd); // Serial.print(", ");
    tmp_str=str; tmp_str.trim();
    if (MyCommands(tmp_str, ", via Serial: "))
    {
      cmd_via_BT=0;
    }
    else
    {
      BTserial.print(str);
    }
  }
  delay(1);
}

void Details(String str)
{
   Serial.print("\nDetails"); PrintCnt(); Serial.print("; from BT"); 
   for (int i=0; i<str.length(); ++i)
   {
      Serial.print(", str[");Serial.print(i);Serial.print("]="); Serial.print((int)(str[i]));
   }
   if (str.length() == 0) 
     Serial.println("");
}
void NextBaudRate()
{
  // list only the rates which you want to have tested, terminate with a 0:
  // long rates[] = {1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600, 1382400, 0};
  long rates[] = {4800, 19200, 38400, 9600, 57600, 115200, 0};
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
  Serial.print(" to "); Serial.print(rates[i]); Serial.print(" baud");
  baud_rate = rates[i];
  BTserial.begin(baud_rate);
}
void SendCmd(String str)
{
  BTserial.print(str);
  if (nlcr & 1) // if bit 0 is set
  {
    // BTserial.println(""); // needed for HC-05
    BTserial.print("\r\n"); // this is equivalent to BTserial.println("")
    str += "\\r\\n";
  }
  Serial.print("\nSent: \""); Serial.print(str); Serial.print("\""); PrintCnt();
  if (nlcr>1) // as long as we are not sure whether we need \r\n
  {
    Serial.print(", nlcr="); Serial.print(nlcr);
  }
  cmd=cnt; dto=0;
}
void AutoCommands()
{
  long dt=0;
  // char * ok_str = "OK";
  if (cnt<99 && cnt>=hc) { Serial.print("  Help: "); };
  //if (cnt==2) { Serial.println("OK, if you can read this your PC-Baud-Rate is set correctly"); };
  if (cnt==2) { Serial.println("OK-PC"); }; // if you can read this your PC-Baud-Rate is set correctly
  if (cnt==hc+(dt+=1)) { Serial.println("Use these commands:"); };
# if 0 // 1: long help, 0: short help
  if (cnt==hc+(dt+=1)) { Serial.println("  help - print this overview"); };
  if (cnt==hc+(dt+=1)) { Serial.println("  off  - set the EN (enable) pin to low"); };
  if (cnt==hc+(dt+=1)) { Serial.println("  on   - set the EN (enable) pin to high"); };
  if (cnt==hc+(dt+=1)) { Serial.println("  34hi - set the HC-05 Pin34 to high"); };
  if (cnt==hc+(dt+=1)) { Serial.println("  34lo - set the HC-05 Pin34 to low"); };
  if (cnt==hc+(dt+=1)) { Serial.println("  ac0  - turn auto commands off"); };
  if (cnt==hc+(dt+=1)) { Serial.println("  ac1  - automatically detect the baud-rate of your Bluetooth Module"); };
  if (cnt==hc+(dt+=1)) { Serial.println("  ac2  - send AT+VERSION to the module after a timeout-period"); };
  if (cnt==hc+(dt+=1)) { Serial.println("  ac4  - send auto commands to turn your HC-05 into a master (unfinished)"); };
  if (cnt==hc+(dt+=1)) { Serial.println("  ac8  - send auto commands to turn your HC-05 into a slave (unfinished)"); };
  if (cnt==hc+(dt+=1)) { Serial.println("  ac16 - send auto commands to turn your HC-06 into a slave (unfinished)"); };
# else // short help (needed e.g. on Arduino nano because dynamic mermory is very limited)
  if (cnt==hc+(dt+=1)) { Serial.println("  help, off, on, 34hi, 34lo, ac0, ac1, ac2, ac4, ac8, ac16, ac128"); };
# endif
  // if (cnt==hc+(dt+=1)) { Serial.println("All other commands are directly sent to your module (e.g. AT-Commands)"); };
  if (cnt==hc+(dt+=1)) { Serial.print("Other txt is sent to module (e.g. AT-Commands)"); };
  if (cnt>=hc+dt && cnt<99) { SetCnt(99); }; // end of Help Info
  if (ac&5 &&cnt==(dt+=1200)) { MyCommands("34hi", "Auto-CMD: "); } // start AT-MODE, only needed for HC-05 }
  if (ac&1 &&cnt==(dt+=1200)) { nlcr=2; SendCmd("AT"); res_t=""; }        // my HC-06 responds with "OK"
  if (ac&1 &&cnt==(dt+= 800)) { if(nlcr>1){if (res_t=="OK") nlcr=0; else {nlcr=3; SendCmd(""); }}} // my HC-05 responds with "OK\r\n"
  if (ac&1 &&cnt==(dt+=1200)) { if(nlcr>1){if (res_t=="OK") nlcr=1; else {nlcr=2; SendCmd("AT"); res_t=""; }}}
  if (ac&1 &&cnt==(dt+= 800)) { if(nlcr>1){if (res_t=="OK") nlcr=0; else {nlcr=3; SendCmd(""); }}}
  if (ac&1 &&cnt==(dt+=1200)) { Details(res_f); if (res_t=="OK") { nlcr &= ~2; } else { NextBaudRate(); SetCnt(99); } }
  if (ac&1 &&cnt==(dt+=1200)) { Serial.print("\nThe Baud-Rate is: "); Serial.print(baud_rate);
                                Serial.print(" baud, and nlcr="); Serial.println(nlcr); }
  
  if (ac&12&&cnt==(dt+=1200)) { SendCmd("AT+VERSION"); } // my HM-10 responds with "+VERSION=Firmware V3.0.6,Bluetooth V4.0 LE\r\nOK"
  if (ac&12&&cnt==(dt+=1200)) { SendCmd("AT+PIN001234"); }
  if (ac&12&&cnt==(dt+=1200)) { SendCmd("AT+BAUD6"); } // Set Baud-Rate to 57600 baud
  if (ac&4 &&cnt==(dt+=1200)) { SendCmd("AT+NAMEMas-HM-10"); } // Set Name to Mas-HM-10
  if (ac&4 &&cnt==(dt+=1200)) { SendCmd("AT+ROLE=1"); } // Set Role to Master
  if (ac&4 &&cnt==(dt+=1200)) { SendCmd("AT+CMODE:1"); } // Connect the module to any address
  if (ac&4 &&cnt==(dt+=1200)) { SendCmd("AT+BIND?"); } // Query BT-address
  if (ac&8 &&cnt==(dt+=1200)) { SendCmd("AT+NAMESla-HM-10"); } // Set Name to Sla-HM-10
  if (ac&8 &&cnt==(dt+=1200)) { SendCmd("AT+ROLE=0"); } // Set Role to Slave
  if (ac&12&&cnt==(dt+=1200)) { SendCmd("AT+ROLE"); }
  if (ac&12&&cnt==(dt+=1200)) { SendCmd("AT+ADDR"); }
  if (ac&12&&cnt==(dt+=1200)) { SendCmd("AT+UUID"); }
  if (ac&12&&cnt==(dt+=1200)) { MyCommands("34lo", "Auto-CMD: "); } // end AT-MODE, only needed for HC-05
  if (ac&12&&cnt==(dt+=1200)) { SendCmd("AT+RESET"); } // Set Baud-Rate to 57600 baud
 
  // final line used in all auto-commands:
  if (       cnt==(dt+=1200)) { SendCmd("AT+VERSION"); } // used in all auto-command modes!
  if (ac&1 &&cnt==(dt+=1200)) { Serial.println(""); MyCommands("help", "Auto-CMD: "); }
}

int MyCommands(String msg, String who)
{
  int ret_val = 1; // 0 == unused
  msg.trim();
  who = who+msg+" ==> ";
  if (msg == "off")     { Serial.print(who+"BT_EN->LOW"); PrintlnCnt();
    BTserial.println("bye."); delay(50);
    digitalWrite(BT_EN, LOW); is_enabled=false; is_connected=false; // turn BT-Module off
  }
  else if (msg == "on") { Serial.print(who+"BT_EN->HIGH"); PrintlnCnt();
    digitalWrite(BT_EN, HIGH); is_enabled=true; // turn BT-Module on
  }
  else if (msg == "34hi") { Serial.print(who+"Pin34->HIGH"); PrintlnCnt();
    digitalWrite(BT_PIN34, HIGH); // simulate press of the button, turn AT-mode on
  }
  else if (msg == "34lo") { Serial.print(who+"Pin34->LOW"); PrintlnCnt();
    digitalWrite(BT_PIN34, LOW); // simulate release of the button
  }
  else if (msg == "ac0") { Serial.print(who+"Auto-Commands Off"); ac=0; SetCnt(99); }
  else if (msg == "ac1") { Serial.print(who+"Baud-Rate-Detection On"); ac=1; SetCnt(99); }
  else if (msg == "ac2") { Serial.print(who+"Send commands as fast as possible on timeout"); ac=2; SetCnt(99); }
  else if (msg == "ac4") { Serial.print(who+"Turn HM10 into master"); ac=4; SetCnt(99); }
  else if (msg == "ac8") { Serial.print(who+"Turn HM10 into slave"); ac=8; SetCnt(99); }
  else if (msg == "help") { Serial.print(who+"Help"); hc=cnt+2; PrintlnCnt(); }
  else if (msg.startsWith("Tx")) { msg=msg.substring(2);
                                   Serial.print(who+"Transmit: \""); Serial.print(msg); Serial.print("\""); PrintlnCnt(); }
  else   ret_val = 0;
  return ret_val;
}

