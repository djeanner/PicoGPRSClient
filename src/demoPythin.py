"""
Raspberry Pi Pico (MicroPython) exercise:
work with SIM868 GSM/GPRS/GNSS Module
"""
import machine
import os
import utime
import binascii


# using pin defined
led_pin = 25  # onboard led
pwr_en = 14  # pin to control the power of the module
uart_port = 0
uart_baute = 115200

APN = "CMNET"

reading = 0
temperature = 0

# uart setting
uart = machine.UART(uart_port, uart_baute)
print(os.uname())

# LED indicator on Raspberry Pi Pico
led_onboard = machine.Pin(led_pin, machine.Pin.OUT)

# MQTT Server info
mqtt_host = '47.89.22.46'
mqtt_port = '1883'

mqtt_topic1 = 'testtopic'
mqtt_topic2 = 'testtopic/led'
mqtt_topic3 = 'testtopic/temp'
mqtt_topic4 = 'testtopic/adc'
mqtt_topic5 = 'testtopic/tempwarning'
mqtt_topic6 = 'testtopic/warning'
mqtt_topic7 = 'testtopic/gpsinfo'

mqtt_msg = 'on'


def led_blink():
    led_onboard.value(1)
    utime.sleep(1)
    led_onboard.value(0)
    utime.sleep(1)
    led_onboard.value(1)
    utime.sleep(1)
    led_onboard.value(0)


# power on/off the module
def power_on_off():
    pwr_key = machine.Pin(pwr_en, machine.Pin.OUT)
    pwr_key.value(1)
    utime.sleep(2)
    pwr_key.value(0)


def hexstr_to_str(hex_str):
    hex_data = hex_str.encode('utf-8')
    str_bin = binascii.unhexlify(hex_data)
    return str_bin.decode('utf-8')


def str_to_hexstr(string):
    str_bin = string.encode('utf-8')
    return binascii.hexlify(str_bin).decode('utf-8')


def wait_resp_info(timeout=2000):
    prvmills = utime.ticks_ms()
    info = b""
    while (utime.ticks_ms()-prvmills) < timeout:
        if uart.any():
            info = b"".join([info, uart.read(1)])
    print(info.decode())
    return info


# Send AT command
def send_at(cmd, back, timeout=2000):
    rec_buff = b''
    uart.write((cmd+'\r\n').encode())
    prvmills = utime.ticks_ms()
    while (utime.ticks_ms()-prvmills) < timeout:
        if uart.any():
            rec_buff = b"".join([rec_buff, uart.read(1)])
    if rec_buff != '':
        if back not in rec_buff.decode():
            print(cmd + ' back:\t' + rec_buff.decode())
            return 0
        else:
            print(rec_buff.decode())
            return 1
    else:
        print(cmd + ' no responce')


# Module startup detection
def check_start():
    while True:
        # simcom module uart may be fool,so it is better to send much times when it starts.
        uart.write(bytearray(b'ATE1\r\n'))
        utime.sleep(2)
        uart.write(bytearray(b'AT\r\n'))
        rec_temp = wait_resp_info()
        if 'OK' in rec_temp.decode():
            print('SIM868 is ready\r\n' + rec_temp.decode())
            break
        else:
            power_on_off()
            print('SIM868 is starting up, please wait...\r\n')
            utime.sleep(8)


# Check the network status
def check_network():
    for i in range(1, 3):
        if send_at("AT+CGREG?", "0,1") == 1:
            print('SIM868 is online\r\n')
            break
        else:
            print('SIM868 is offline, please wait...\r\n')
            utime.sleep(5)
            continue
    send_at("AT+CPIN?", "OK")
    send_at("AT+CSQ", "OK")
    send_at("AT+COPS?", "OK")
    send_at("AT+CGATT?", "OK")
    send_at("AT+CGDCONT?", "OK")
    send_at("AT+CSTT?", "OK")
    send_at("AT+CSTT=\""+APN+"\"", "OK")
    send_at("AT+CIICR", "OK")
    send_at("AT+CIFSR", "OK")


# Get the gps info
def get_gps_info():
    count = 0
    print('Start GPS session...')
    send_at('AT+CGNSPWR=1', 'OK')
    utime.sleep(2)
    for i in range(1, 10):
        uart.write(bytearray(b'AT+CGNSINF\r\n'))
        rec_buff = wait_resp_info()
        if ',,,,' in rec_buff.decode():
            print('GPS is not ready')
#            print(rec_buff.decode())
            if i >= 9:
                print('GPS positioning failed, please check the GPS antenna!\r\n')
                send_at('AT+CGNSPWR=0', 'OK')
            else:
                utime.sleep(2)
                continue
        else:
            if count <= 3:
                count += 1
                print('GPS info:')
                print(rec_buff.decode())
            else:
                send_at('AT+CGNSPWR=0', 'OK')
                break


# Get the gps info
def phone_call(phone_num='10000', keep_time=10):
    send_at('AT+CHFA=1', 'OK')
    send_at('ATD'+phone_num+';', 'OK')
    utime.sleep(keep_time)
    send_at('AT+CHUP;', 'OK')


# Bluetooth test
def bluetooth_test():
    send_at('AT+BTPOWER=1', 'OK', 3000)
    send_at('AT+BTHOST?', 'OK', 3000)
    send_at('AT+BTSTATUS?', 'OK', 3000)
    send_at('AT+BTSCAN=1,10', 'OK', 8000)
    send_at('AT+BTPOWER=0', 'OK')


# AT test
def at_test():
    print("---------------------------SIM868 TEST---------------------------")
    while True:
        try:
            command_input = str(input('Please input the AT command,press Ctrl+C to exit: '))
            send_at(command_input, 'OK', 2000)
        except KeyboardInterrupt:
            print("\r\nExit AT command test!\n")
            power_on_off()
            print("Power off the module!\n")
            break


# main program
check_start()
get_gps_info()
