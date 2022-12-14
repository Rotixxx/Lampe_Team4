import requests
import json
import os
import os.path 
from os import path 
from time import sleep 

# POST wav file
url = "http://192.168.8.106:12101/api/speech-to-intent"
filePath = '/home/pi/Desktop/rec.wav'

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
		print ("kein File")
		continue
	with open(filePath, 'rb') as file:
		r = requests.post(url, params=params, headers=headers, data=file)
	# print(r.text)

	#getting response
	response=json.loads(r.text)
	# print(response)

	on_off=response["entities"][0]["raw_value"]
	print(on_off)
	
	os.remove(filePath)
	
	with open('/home/pi/Desktop/finalCommand.txt', 'w') as file:
  		file.write(on_off)

	
	
