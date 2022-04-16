#include <Arduino.h>
#include <CAN.h>

/*
--- Algorithm ---

Only listen for ID's: 0x3CA (Vehicle Speed), 0x5B6 (Door Lock Status)

Track State with State Machine:
  0x10: Determining

  0x20: Sending
    If we NEED to retry to send command, if the speed has dropped below threshold
    cancel sending the command

  0xXX
    Invalid state, go to state 0x10

Read Vehicle Speed First --- 0x3CA (Vehicle Speed)
  Read and save speed (convert to MPH)

Read Door Lock Status --- 0x5B6 (Door Lock Status)
  Read and save status to temp var

  If speed is greater than threshold
      If any doors are physically OPEN
        DONT try to lock

      If all doors are already locked
        If they are DONT try to lock all doors

      Send lock command
        If unable to send, retry up to 10 times
          If the speed has dropped below threshold on retry, cancel sending command
*/

#define CAN_BAUD_RATE 115200
#define CAN_BUS_SPEED 500E3
#define CAN_CLOCK_SPEED 8E6

#define VEHICLE_SPEED_CAN_ID 0x3CA
#define DOOR_LOCKS_CAN_ID 0x5B6

#define AUTO_LOCK_SPEED_MPH 1
#define MAX_RETRY_SEND_COUNT 10

#define SEPARATOR ','
#define NEWLINE '\n'

enum State
{
  Determining = 0x10,
  Sending = 0x20,
};

typedef struct CANPacket_t
{
  long id;
  byte rtr;
  byte ide;
  byte dlc;
  byte data[8];

} CANPacket;

State state = Determining;

int lastVehicleSpeedMPHRead = 0;
int lastDoorsLockedStatus = 0;
int lastDoorsClosedStatus = 0;

byte retrySendCount = 0;

void setup()
{
  Serial.begin(CAN_BAUD_RATE);

  while (!Serial);

  // Set to correct CAN board clock frequency (lower than lib default)
  CAN.setClockFrequency(CAN_CLOCK_SPEED);

  if (!CAN.begin(CAN_BUS_SPEED))
  {
    Serial.println("CAN begin failed");
    while (1);
  }
}

void printHex(long num)
{
  if (num < 0x10) Serial.print("0");
  Serial.print(num, HEX);
}

// Can cause error code by sending 0x00 as first byte
bool sendLockAllDoorsCANPacket()
{
  CAN.beginPacket(DOOR_LOCKS_CAN_ID, 0x3);
  CAN.write(0xE4);
  CAN.write(0xC4);
  CAN.write(0x00);
  return CAN.endPacket();
}

void handleAutoLock()
{
  int packetSize = CAN.parsePacket();
  if (packetSize < 1) return;

  CANPacket packet = 
  {
      .id = CAN.packetId(),
      .rtr = CAN.packetRtr(),
      .ide = CAN.packetExtended(),
      .dlc = CAN.packetDlc()
  };

  for (byte i = 0; i < packet.dlc && CAN.available(); ++i)
    packet.data[i] = CAN.read();

  if (packet.id == DOOR_LOCKS_CAN_ID)
  {
    lastDoorsLockedStatus = packet.data[1];
    lastDoorsClosedStatus = packet.data[2];
  }

  if (packet.id == VEHICLE_SPEED_CAN_ID)
  {
    // Higher speed bias due to ceil
    lastVehicleSpeedMPHRead = (byte)ceil(packet.data[2] * 0.6214F);
    Serial.println(lastVehicleSpeedMPHRead);
  }

  switch (state)
  {
  case Determining:
    if (lastVehicleSpeedMPHRead > AUTO_LOCK_SPEED_MPH &&
        lastDoorsLockedStatus != 0x0 &&
        lastDoorsClosedStatus == 0x0)
    {
      state = Sending;
    }
    break;

  case Sending:
    if (lastVehicleSpeedMPHRead < AUTO_LOCK_SPEED_MPH)
    {
      retrySendCount = 0;
      state = Determining;
      Serial.println("Speed lower than threshold, not locking");
    }
    else
    {
      // for (byte i = 0, retry = 0; i < 26 && i > -1 && retry < 5; ++i)
      // {
      //   if(!sendLockAllDoorsCANPacket()){ --i; ++retry; }
      // }
      // Serial.println("Sent lock message");
      // state = Determining;

      if (sendLockAllDoorsCANPacket())
      {
        retrySendCount = 0;
        state = Determining;
        Serial.println("Sent lock message");
      }
      else
      {
        if (++retrySendCount > MAX_RETRY_SEND_COUNT)
        {
          retrySendCount = 0;
          state = Determining;
          Serial.println("Retry count exceeded, will re-evaluate");
        }
      }
    }
    break;

  default:
    state = Determining;
    break;
  }
}

void canPacketsForPython()
{
  int packetSize = CAN.parsePacket();

  if (packetSize)
  {
    // ID
    printHex(CAN.packetId());
    Serial.print(SEPARATOR);

    // RTR
    printHex(CAN.packetRtr());
    Serial.print(SEPARATOR);

    // IDE
    printHex(CAN.packetExtended());
    Serial.print(SEPARATOR);

    // DLC
    int dlc = CAN.packetDlc();
    printHex(dlc);
    Serial.print(SEPARATOR);

    // Pad DLC to 8 Bytes
    if (dlc < 8)
    {
      byte padLen = abs(8 - dlc);
      for (byte i = 0; i < padLen; ++i)
      {
        printHex(0);
        Serial.print(":");
      }
    }

    // Data
    for (byte i = 0; i < dlc && CAN.available(); ++i)
    {
      printHex(CAN.read());
      if (i < dlc - 1) Serial.print(":");
    }

    Serial.print(NEWLINE);
  }
}

void loop()
{
  handleAutoLock();
  // canPacketsForPython();
}
