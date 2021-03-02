#include <Arduino.h>
//--------------------------------------------------------------------
// Author:        Aamir Hasan Khan
// Version:       01
// Description:   Controls the Lift Motor
// File Type:     Main Controller
// Date:          25-12-2017
//--------------------------------------------------------------------

//--------------------------------------------------------------------
// Revised
// 25 Februrary 2018
// Include Seven Segment Display
// 04 March 2018
// Save Sataus to EEPROM
// 21-01-2019
// Remove latches, brake, starter ==> Mega --> Pro Mini
// Changes made for bypass , 26-2-2020 by arslan
// includes bydefault 0, 1 relay added,Led indicator  etc.
// 27-02-2021
// Revised hardware with I/O extender.
//--------------------------------------------------------------------
#include <PCF8574.h>
#include <Wire.h>
#include <EEPROM.h>

#define basementFloor 0
#define groundFloor 1
#define firstFloor 2
#define secondFloor 3

//--------------------------------------------------------------------
// Hardware Configurations
//--------------------------------------------------------------------
// Floor Sensors
#define basementSensor 7
#define groundSensor 6
#define firstSensor 5
#define secondSensor 4

// Motor Control Relays
#define m_up 0
#define m_dn 1
#define m_bp 2 // Bypass

//Led indicator for cabin
#define cabinLight 3
#define cabinFan 4

#define CLEAR_DISP 0xFF

#define IRQPIN 3
//--------------------------------------------------------------------
bool flag = false;
bool command = true;
int KeyValue;

byte SegementNumber[] = {0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90, 0x86};

PCF8574 SetFloorPosition(0x20);
PCF8574 DisplaySegment(0x21);
PCF8574 Controls(0x22);

//--------------------------------------------------------------------
// Lift Position variables
//--------------------------------------------------------------------
struct pos
{
  int present;
  int desired;
  int present_addr = 0;
  int desired_addr = 1;
  boolean up;
  boolean down;
};
struct pos Lift;

//--------------------------------------------------------------------
// Lift Position Update
//--------------------------------------------------------------------
int current_pos()
{
  boolean basement, ground, first, second;
  int current_state;

  current_state = Lift.present;

  basement = digitalRead(basementSensor);
  ground = digitalRead(groundSensor);
  first = digitalRead(firstSensor);
  second = digitalRead(secondSensor);

  if (!basement)
    current_state = basementFloor;
  else if (!ground)
    current_state = groundFloor;
  else if (!first)
    current_state = firstFloor;
  else if (!second)
    current_state = secondFloor;
  else
  {
    Lift.desired = EEPROM.read(Lift.desired_addr);
    current_state = EEPROM.read(Lift.present_addr);
    DisplaySegment.write8(SegementNumber[current_state]);
    return current_state;
  }
  EEPROM.write(Lift.present_addr, current_state);
  return current_state;
}
//--------------------------------------------------------------------

//--------------------------------------------------------------------
// Motor Control based on required position
//--------------------------------------------------------------------
void control_pos()
{
  if (Lift.desired != 255)
  {
    DisplaySegment.write8(SegementNumber[Lift.desired]);
    Controls.write(cabinLight, HIGH);
    Controls.write(cabinFan, HIGH);
    Controls.write(m_bp, HIGH);
    if (Lift.desired > Lift.present)
    {
      Lift.up = true;
      delay(2000);
      Controls.write(m_dn, LOW);
      Controls.write(m_up, HIGH);
    }
    else if (Lift.desired < Lift.present)
    {
      Lift.down = true;
      delay(2000);
      Controls.write(m_dn, HIGH);
      Controls.write(m_up, LOW);
    }
  }

  if (Lift.present == Lift.desired)
  {
    Controls.write(m_dn, LOW);
    Controls.write(m_up, LOW);

    Controls.write(cabinLight, LOW);
    Controls.write(cabinFan, LOW);
    Controls.write(m_bp, LOW);
    Lift.up = false;
    Lift.down = false;

    Lift.desired = 255;
    command = true;
    delay(6000);
  }
}
//--------------------------------------------------------------------

void pcf_irq()
{
  flag = true;
}

void setup()
{
  Serial.begin(19200);
  SetFloorPosition.begin();
  DisplaySegment.begin();
  Controls.begin(0);

  Lift.desired = 255;

  pinMode(IRQPIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(IRQPIN), pcf_irq, FALLING);

  Lift.present = current_pos();
  DisplaySegment.write8(SegementNumber[Lift.present]);
}

void loop()
{
  if (flag)
  {
    flag = false;
    KeyValue = SetFloorPosition.read8();
    if (KeyValue != 0xFF)
    {
      if (KeyValue == 254)
      {
        Controls.write(m_dn, LOW);
        Controls.write(m_up, LOW);
        DisplaySegment.write8(0x86);
        while (1)
          ;
      }
      else if (KeyValue == 253 && command)
        Lift.desired = secondFloor;
      else if (KeyValue == 251 && command)
        Lift.desired = firstFloor;
      else if (KeyValue == 247 && command)
        Lift.desired = groundFloor;
      else if (KeyValue == 239 && command)
        Lift.desired = basementFloor;
      EEPROM.write(Lift.desired_addr, Lift.desired);
      command = false;
    }
  }

  control_pos();
  Lift.present = current_pos();

  // Serial.print("Destination Floor :");
  // Serial.println(Lift.desired);
  // Serial.print("Current Floor :");
  // Serial.println(Lift.present);
}
