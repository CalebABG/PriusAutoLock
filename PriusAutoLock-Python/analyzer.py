def get_all_lines(file_name):
    lines = []
    if file_name is not None and file_name != "":
        with open(file_name) as file:
            while True:
                line_read = file.readline()
                if not line_read:
                    break
                lines.append(line_read.strip("\n\r"))
    return lines


if __name__ == '__main__':
    try:
        all_lines = get_all_lines("./data/t2.csv")
        can_dict = {}

        for line in all_lines:
            message_id = line[:line.find(',')]

            if message_id not in can_dict:
                can_dict[message_id] = [line]
            else:
                can_dict[message_id].append(line)

        for val in can_dict['3CA']:
            data = val[val.rfind(',') + 1:].split(':')
            kph = int(data[-3], base=16)
            mph = kph * 0.6214
            print(mph)
    except Exception as ex:
        print(f"Error occurred: {ex}")
