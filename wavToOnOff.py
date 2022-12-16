import requests
import json
import os
import os.path 
from os import path 
from time import sleep 
import sys 
from gpiozero import LED
from time import sleep

filePath=sys.argv[1]
# POST wav file
# url = "http://192.168.8.106:12101/api/speech-to-intent"
url = "http://192.168.43.40:12101/api/speech-to-intent"
# filePath = '/home/pi/Desktop/rec.wav'

headers = {
   # "acl": "public-read",
  #  "Authorization": "Bearer ...",
    "Accept": "application/json, text/plain, */*",
    "Accept-Encoding": "gzip, deflate",
   # "Accept-Language": "en-GB,en;q=0.9,de-DE;q=0.8,de;q=0.7,en-US;q=0.6",
  #  "Connection": "keep-alive",
  #  "Content-Length": "30764",
    "Content-Type": "audio/wav",
}

params = {"uploadType": "media", "name": "rec.wav"}

while (True):
	sleep (0.5)
	if not path.isfile(filePath):
		print ("waiting for file")
		continue
	try:
		with open(filePath, 'rb') as file:
			r = requests.post(url, params=params, headers=headers, data=file)
		os.remove(filePath)
	#	print("r.text")
	#	print(r.text)
		

		#getting response
		response=json.loads(r.text)
	#	print("response")
	#	print(response)

		on_off=response["entities"][0]["raw_value"]
		print("got file with: " + on_off)
		
		
		with open('/home/pi/Desktop/finalCommand.txt', 'w') as file:
	  		file.write(on_off)
	  	
	  	# Initialisierung von GPIO17 als LED (Ausgang) und on/off des LEDs
		led = LED(17)
	  	
		if str(on_off)=="on":
			led.on()
			print("The LED is turned on")
		if str(on_off)=="off":
			led.off()
			print("The LED is turned off")
	  		
	except:
		with open('/home/pi/Desktop/finalCommand.txt', 'w') as file:
	  		file.write("unknown error")
	  		print("unknown error")
	  		
  


