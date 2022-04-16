import serial

"""
Save data commands:
- stty -f /dev/cu.usbmodem1101 115200 & cat /dev/cu.usbmodem1101 |& tee ~/Desktop/t.csv

View Arduino data
- stty -f /dev/cu.usbmodem1101 115200 & cat /dev/cu.usbmodem1101


---


ID  = 0x5B6
DLC = 0x3

Second pos indicates which door is locked or unlocked
Third  pos indicates which which door is open or closed

`Lock`   All Doors Data        = E4:C4:00
`Unlock` All Doors Data        = E4:C5:00

All Doors `Locked`   Status    = 64:00:00
All Doors `Unlocked` Status    = 64:C5:00
"""

IDLE_IDS = {
    "20",
    "22",
    "23",
    "25",  # Steering Angle?
    "30",  # Brake Pedal Position
    "38",
    "39",
    "3A",  # Acceleration
    "3B",
    "3E",
    "B1",
    "B3",
    "B4",
    "120",  # Gas Pedal Position
    "230",  # Brake Down Indicator?
    "244",
    "262",
    "348",
    "3C8",  # Engine Speed
    "3C9",
    "3CA",  # Velocity (km/h)
    "3CB",
    "3CD",
    "3CF",
    "423",
    "484",
    "4C1",
    "4C3",
    "4C6",
    "4C7",
    "4C8",
    "4CE",
    "4D0",
    "4D1",
    "520",  # ICE Status?
    "521",
    "526",
    "527",
    "528",
    "529",
    "52C",
    "53F",
    "540",
    "553",
    "554",
    "56D",
    "57F",
    "591",
    "5A4",
    "5B2",
    "5B6",
    "5C8",
    "5CC",
    "5D4",
    "5EC",
    "5ED",
    "5F8",
    "602",
}

# Change name on diff OS
arduino_device_name = "/dev/cu.usbmodem1101"
can_baud_rate = 115200
can_dict = {}


def get_message_id(message: str):
    return message[:message.find(',')]


def sort_can_dict(d: dict):
    return sorted(d.items(), key=lambda x: int(x[0], base=16))


def is_idle_message_id(message_id: str):
    return message_id in IDLE_IDS


def print_selected_id(id: str, message_id: str, message: str):
    if id in can_dict and message_id == id:
        print(message)


def print_sorted_can_dict():
    # t is Tuple
    for t in sort_can_dict(can_dict):
        print(t[1])


def main():
    with serial.Serial(port=arduino_device_name,
                       baudrate=can_baud_rate,
                       timeout=.1) as arduino:
        while True:
            try:
                line_read = arduino.readline()
                if line_read is None or line_read == "":
                    continue

                can_message = line_read.decode('utf8').strip('\n\r\t')
                message_id = get_message_id(can_message)

                can_dict[message_id] = can_message

                print_selected_id('5B6', message_id, can_message)
                # print_sorted_can_dict()

            except Exception as e:
                print(f"Err: Could not parse incoming data - {e}")


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt as keyEx:
        print("CTRL+C hit, closing port and exiting")
    except Exception as ex:
        print(f"Error occurred: {ex}")
