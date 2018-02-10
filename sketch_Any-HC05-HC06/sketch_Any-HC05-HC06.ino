
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
long cnt=0, cmd=0, dto=0, hc=2;
int cmd_via_BT=0;
String str;
String res_f, res_t; // response, full and trimmed (without white-space)
int nlcr=3; // need "\r\n" at end of AT-commands: 0,(2)=no, 1,(3)=yes, in brackets: guessed. Must be 0 for HC-06!

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
  String tmp_str;
  ++cnt; // increment loop-counter, allow overrun.
  if (ac) AutoCommands();
  
  if (BTserial.available())
  {
    delay(1); // read everything which is available within 1 ms, so that e.g. "OK\r\n" is not split up into "OK" and "\r\n"
    str=BTserial.readString();
    res_f=str;
    res_t=str; res_t.trim();
    str.trim();
    if ((cnt-cmd-dto) > 2000) Serial.println("."); // most likely we got a new line from remote
    Serial.print(", from BT: \"");
    tmp_str=str; tmp_str.replace("\n","\\n"); tmp_str.replace("\r","\\r"); Serial.print(tmp_str+"\", ");
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
    Serial.print("@cnt="); Serial.print(cnt); Serial.print(", delta="); Serial.print(cnt-cmd-dto); Serial.print(" ");
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
   Serial.print("\nDetails @cnt="); Serial.print(cnt); Serial.print("; from BT"); 
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
  Serial.print(" to "); Serial.print(rates[i]); Serial.print(" baud.");
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
  Serial.print("\nSent@cnt="); Serial.print(cnt); Serial.print(": \""); Serial.print(str); Serial.print("\"");
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
  if (cnt==hc+(dt+=1)) { Serial.println("Other txt is sent to module (e.g. AT-Commands)"); };
  if (cnt>=hc+dt && cnt<99) { cnt=99; }; // end of Help Info
  if (ac&5 &&cnt==(dt+=1200)) { digitalWrite(BT_PIN34, HIGH);} // start AT-MODE, only needed for HC-05 }
  if (ac&1 &&cnt==(dt+=1200)) { nlcr=2; SendCmd("AT"); res_t=""; }        // my HC-06 responds with "OK"
  if (ac&1 &&cnt==(dt+= 800)) { if(nlcr>1){if (res_t=="OK") nlcr=0; else {nlcr=3; SendCmd(""); }}} // my HC-05 responds with "OK\r\n"
  if (ac&1 &&cnt==(dt+=1200)) { if(nlcr>1){if (res_t=="OK") nlcr=1; else {nlcr=2; SendCmd("AT"); res_t=""; }}}
  if (ac&1 &&cnt==(dt+= 800)) { if(nlcr>1){if (res_t=="OK") nlcr=0; else {nlcr=3; SendCmd(""); }}}
  if (ac&1 &&cnt==(dt+=1200)) { Details(res_f); if (res_t=="OK") { nlcr &= ~2; } else { NextBaudRate(); cnt=9; } }
  if (ac&1 &&cnt==(dt+=1200)) { Serial.print("\nThe Baud-Rate is: "); Serial.print(baud_rate);
                                Serial.print(" baud, and nlcr="); Serial.println(nlcr); }
  
  if (ac&12&&cnt==(dt+=1200)) { SendCmd("AT+VERSION"); } // my HC-05 responds with "+VERSION:2.0-20100601\r\nOK"
  if (ac&12&&cnt==(dt+=1200)) { SendCmd("AT+PSWD=1234"); }
  if (ac&12&&cnt==(dt+=1200)) { SendCmd("AT+UART:38400,0,0"); } // Set Baud-Rate to 57600 baud
  if (ac&4 &&cnt==(dt+=1200)) { SendCmd("AT+NAME=Mas-HC-05"); } // Set Name to Mas-HC-05
  if (ac&4 &&cnt==(dt+=1200)) { SendCmd("AT+ROLE=1"); } // Set Role to Master
  if (ac&4 &&cnt==(dt+=1200)) { SendCmd("AT+CMODE:1"); } // Connect the module to any address
  if (ac&4 &&cnt==(dt+=1200)) { SendCmd("AT+BIND?"); } // Query BT-address
  if (ac&8 &&cnt==(dt+=1200)) { SendCmd("AT+NAME=Sla-HC-05"); } // Set Name to Sla-HC-05
  if (ac&8 &&cnt==(dt+=1200)) { SendCmd("AT+ROLE=0"); } // Set Role to Slave
  if (ac&5 &&cnt==(dt+=1200)) { digitalWrite(BT_PIN34, LOW);} // end AT-MODE, only needed for HC-05
  if (ac&12&&cnt==(dt+=1200)) { SendCmd("AT+RESET"); } // Set Baud-Rate to 57600 baud

  if (ac&16 &&cnt==(dt+=1200)) { SendCmd("AT+VERSION"); } // my HC-06 responds with "OKlinvorV1.8"
  if (ac&16 &&cnt==(dt+=1200)) { SendCmd("AT+PIN1234"); }
  if (ac&16 &&cnt==(dt+=1200)) { SendCmd("AT+NAMESla-HC-06"); } // Set Name to VIN-HC-06
  if (ac&16 &&cnt==(dt+=1200)) { SendCmd("AT+BAUD6"); }         // Set Baud-Rate to 57600 baud
  
  // final line used in all auto-commands:
  if (       cnt==(dt+=1200)) { SendCmd("AT+VERSION"); } // used in all auto-command modes!
  if (ac&1 &&cnt==(dt+=1200)) { MyCommands("help", "Auto-CMD: "); }
  
  // NOTE: the next line will send a command every 1.2 seconds, actually the HC-06 should allow a command every second, but it doesn't!
  if (ac&2 &&cnt>=(cmd+dt+1200)) { str="AT+VERSION";
    if (nlcr || cmd_via_BT) BTserial.println(str); else BTserial.print(str); 
    Serial.print(", type ac0 to stop!\nSent: \""); str.trim(); Serial.print(str);
    cmd=cnt-dt; dto=dt; // communicate as fast as possible (as fast as allowed) with BT-module
    if (nlcr || cmd_via_BT) { Serial.print("\\r\\n\""); } else Serial.print("\"");
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
  else if (msg == "ac1") { Serial.println(who+"Baud-Rate-Detection On"); ac=1; cnt=99; }
  else if (msg == "ac2") { Serial.println(who+"Send commands as fast as possible on timeout"); ac=2; cnt=99; }
  else if (msg == "ac4") { Serial.println(who+"Send all cmds to turn a HC-05 into a master (unfinished)"); ac=4; cnt=99; }
  else if (msg == "ac8") { Serial.println(who+"Send all cmds to turn a HC-05 into a slave (unfinished)"); ac=8; cnt=99; }
  else if (msg == "ac16") { Serial.println(who+"Send all cmds to turn a HC-06 into a slave (unfinished)"); ac=16; cnt=99; }
  else if (msg == "ac32") { Serial.println(who+"Not defined"); ac=32; cnt=99; }
  else if (msg == "ac64") { Serial.println(who+"Not defined"); ac=64; cnt=99; }
  else if (msg == "help") { Serial.println(who+"Help"); hc=cnt+2; }
  else   ret_val = 0;
  return ret_val;
}

