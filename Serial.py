import serial
from time import sleep
from Crypto.Cipher import AES
from Crypto.Random import get_random_bytes
import requests
import json
import os
import os.path 
from os import path 
from time import sleep 
import sys 
from gpiozero import LED
import gpiozero.pins.rpigpio

ser = serial.Serial ("/dev/ttyS0", 115200)    #Open port with baud rate
filename = '/home/pi/Desktop/rec_1.wav'

counter = 0
audio_data = bytearray(b'')

# from https://github.com/gpiozero/gpiozero/issues/707#issuecomment-761540593
# LED Status bleibt nach Terminierung des Programms erhalten
def close(self): pass
gpiozero.pins.rpigpio.RPiGPIOPin.close = close
led = LED(17, initial_value=None, pin_factory=gpiozero.pins.rpigpio.RPiGPIOFactory())
errorLED = LED(18, initial_value=None, pin_factory=gpiozero.pins.rpigpio.RPiGPIOFactory())

url = "http://localhost:12101/api/speech-to-intent"

headers = {
"Accept": "application/json, text/plain, */*",
"Accept-Encoding": "gzip, deflate",
"Content-Type": "audio/wav",
}

params = {"uploadType": "media", "name": "rec.wav"}

def send_to_rhasspy(recording):
    try:
#        with open('/home/pi/Desktop/rec_1.wav', 'rb') as file:
        r = requests.post(url, params=params, headers=headers, data=recording)
        
        #getting response
        response=json.loads(r.text) 
            
        on_off=response["raw_text"]
           
        if str(on_off)=="on":
            led.on()
            print("The LED is turned on")
        elif str(on_off)=="stop":
            led.off()
            print("The LED is turned off")
        else:
            errorLED.on()
            print("unknown command")
            sleep(2)
            errorLED.off
    except:
        print("unknown error")
        errorLED.on()
        sleep(2)
        errorLED.off


while True:
    print('new while loop')
    if ser.in_waiting>0:
    
        hdr = ser.read(8)
        print(hdr.hex())
        nonce = ser.read(13)
        print(nonce.hex())
        received_data = ser.read(4096)
        ciphertext = received_data[:-8]
 #       print(received_data.hex())
        mac = received_data[-8:]
 #       print(mac.hex())
         
        
#        key = b'thats my kungfu!'
        key = bytes.fromhex('c0c1c2c3c4c5c6c7c8c9cacbcccdcecf')
        cipher = AES.new(key, AES.MODE_CCM, nonce=nonce, mac_len=8)
        cipher.update(hdr)
        
        try:
            plaindata = cipher.decrypt_and_verify(ciphertext,mac)
 #           print(plaindata.hex())
            audio_data.extend(plaindata)
            print('Block %d is good' % counter)
        except ValueError:
            print ("Key incorrect or message corrupted")
        counter+=1
        if counter == 16:
            with open(filename, 'wb') as f:
                f.write(audio_data)
                print('done')
                counter = 0
            sleep(0.1)
            send_to_rhasspy(audio_data)
            audio_data=bytearray(b'')
    sleep(0.1)
    errorLED.off()