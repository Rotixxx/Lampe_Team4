import serial
from time import sleep
from Crypto.Cipher import AES
from Crypto.Random import get_random_bytes

ser = serial.Serial ("/dev/ttyS0", 115200)    #Open port with baud rate
filename = '/home/pi/Desktop/rec_1.wav'

counter = 0
audio_data = bytearray(b'')

while counter < 1:
    print('new while loop')
    if ser.in_waiting>0:
    
        hdr = ser.read(8)
        print(hdr.hex())
        nonce = ser.read(13)
        print(nonce.hex())
        received_data = ser.read(4096)
        ciphertext = received_data[:-8]
 #       print(ciphertext.hex())
        mac = received_data[-8:]
 #       print(mac.hex())
         
        
#        key = b'thats my kungfu!'
        key = bytes.fromhex('c0c1c2c3c4c5c6c7c8c9cacbcccdcecf')
        cipher = AES.new(key, AES.MODE_CCM, nonce=nonce, mac_len=8)
        cipher.update(hdr)
        
        try:
            plaindata = cipher.decrypt_and_verify(ciphertext,mac)
            print(plaindata.hex())
            audio_data.extend(plaindata)
            print('Block %d is good' % counter)
        except ValueError:
            print ("Key incorrect or message corrupted")
        counter+=1
    sleep(0.1)

with open(filename, 'wb') as f:
    f.write(audio_data)
print('done')
    
      