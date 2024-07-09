#!/usr/bin/env python3

from Levenshtein import ratio

send=""
with open("send.txt", mode='rb') as f:
    send = f.read()

received=""
with open("received.txt", mode='rb') as f:
    received = f.read()

sendBin=""
receivedBin=""

for idx in range(len(send)):
    for i in range(8):
        sendBin += str(send[idx] // 2**(7-i) % 2)

for idx in range(len(received)):
    for i in range(8):
        receivedBin += str(received[idx] // 2**(7-i) % 2)

print("Levenshtein similarity between 'send.txt' and 'received.txt': " + str(ratio(sendBin, receivedBin) * 100) + "%")
#print("Levenshtein similarity between 'send.txt' and 'received.txt': " + str(ratio(send, received) * 100) + "%")
