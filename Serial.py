#import of necessary libraries and functions
import serial
from time import sleep
#https://pycryptodome.readthedocs.io/en/latest/src/installation.html documentation to Crypto
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

#init LEDs
# from https://github.com/gpiozero/gpiozero/issues/707#issuecomment-761540593
# LED Status bleibt nach Terminierung des Programms erhalten
def close(self): pass
gpiozero.pins.rpigpio.RPiGPIOPin.close = close
lampLED = LED(20, initial_value=None, pin_factory=gpiozero.pins.rpigpio.RPiGPIOFactory())
errorLED = LED(18, initial_value=None, pin_factory=gpiozero.pins.rpigpio.RPiGPIOFactory())
yellowLED = LED(21, initial_value=None, pin_factory=gpiozero.pins.rpigpio.RPiGPIOFactory())
greenLED = LED(17, initial_value=None, pin_factory=gpiozero.pins.rpigpio.RPiGPIOFactory())

# init values for rhasspy post request
url = "http://localhost:12101/api/speech-to-intent"

headers = {
"Accept": "application/json, text/plain, */*",
"Accept-Encoding": "gzip, deflate",
"Content-Type": "audio/wav",
}

params = {"uploadType": "media", "name": "rec.wav"}

#counter for init test of LEDs blinking
init_counter = 0

#test the red LED on start of programm
errorLED.on()
sleep(1)
errorLED.off()

#test the green LED on start of programm
greenLED.on()
sleep(1)
greenLED.off()

#test the yellow LED on start of programm
yellowLED.on()
sleep(1)
yellowLED.off()

#test the blue LED (LAMP) on start of programm
lampLED.on()
sleep(1)
lampLED.off()

#test the red LED blinking on start of programm
while init_counter != 5:
    errorLED.on()
    sleep(0.25)
    errorLED.off()
    sleep(0.25)
    init_counter += 1

def send_to_rhasspy(recording):
    try:
        #        with open('/home/pi/Desktop/rec_1.wav', 'rb') as file:
        r = requests.post(url, params=params, headers=headers, data=recording)
        
        #getting response form rhasspy
        response=json.loads(r.text) 
        
        # reading state of on_off from rhasspy response        
        on_off=response["raw_text"]
        
        # 
        if str(on_off)=="on":
            lampLED.on()
            print("The LED is turned on")
        elif str(on_off)=="stop":
            lampLED.off()
            print("The LED is turned off")
        else:
            errorLED.on()
            greenLED.off()
            print("unknown command")
            sleep(2)
            errorLED.off()
            greenLED.on()
    except:
        print("unknown error")
        errorLED.on()
        greenLED.off()
        sleep(2)
        greenLED.on()
        errorLED.off()

#swith on green LED to signal that the system is ready to use
greenLED.on()

while True:
    
#    print('in while loop')
    if ser.in_waiting>0:
    
        # Toggle orange/yellow LED to signal reception of data        
        yellowLED.on()
        sleep(0.25)
        yellowLED.off()
        sleep(0.25)
        
        # read header for AES-CCM Mode
        hdr = ser.read(8)
        
        # read nonce for AES-CCM Mode
        nonce = ser.read(13)
        
        # read incrypted data 
        received_data = ser.read(4096)
        
        # parce data (all but last 8 byte) and mac (last 8 byte)
        ciphertext = received_data[:-8]
        mac = received_data[-8:]
         
        #set key
        key = bytes.fromhex('53616e64726120756e6420526f204734')
        
        # initialise/update AES parameters for decryption
        cipher = AES.new(key, AES.MODE_CCM, nonce=nonce, mac_len=8)
        cipher.update(hdr)
        
        # decryption of cipher, if not successful -> block is not appended on decrypted data 
        try:
            plaindata = cipher.decrypt_and_verify(ciphertext,mac)
            audio_data.extend(plaindata)
        except ValueError:
           print ("Key incorrect or message corrupted")
        counter +=1
        
        # if 16 loops of decryption are through (even if not all blocks were successfully decrypted), send audio_buffer to rhasspy
        # for every block, that was not decrypted successfully, the data of that block is not appended to the audio_buffer
        if counter == 16: 
            sleep(0.1)
            send_to_rhasspy(audio_data)
            print('done')
            counter = 0
            audio_data=bytearray(b'')
    sleep(0.1)