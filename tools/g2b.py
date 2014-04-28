#!/usr/bin/python
from array import array
print "Welcome to the g2b (g-code to binary) file converter by Antmicro Ltd." 
import re, struct, numpy

bfile = open("image.bin","wb")

tab = []
tab_index = 0
o = x = y = x = i = j = par = 0

import sys
if (len(sys.argv) == 1):
    print "ERROR: You need to specify an input g-code file."
    sys.exit(1)
try:
    gf = open(sys.argv[1])
except IOError:
    print "ERROR: The specified file could not be opened."
    sys.exit(1)

for line in gf:
    if(line[0] == "G" and line[1] == "0"):
        line_split = line.split(" ")
        #Save valuses to tab
        o = x = y = z = i = j = par = "\xFF\xFF\xFF\xFF"
        for index in line_split:
            if  (index[0] == "G"):
                index = re.sub('[G]', '', index)
                o = struct.pack('i',int(index))
            elif(index[0] == "X"):
                index = re.sub('[X]', '', index)
                x = struct.pack('f',float(index))
            elif(index[0] == "Y"):
                index = re.sub('[Y]', '', index)
                y = struct.pack('f',-float(index))
            elif(index[0] == "Z"):
                index = re.sub('[Z]', '', index)
                z = struct.pack('f',float(index))
            elif(index[0] == "I"):
                index = re.sub('[I]', '', index)
                i = struct.pack('f',float(index))
            elif(index[0] == "J"):
                index = re.sub('[J]', '', index)
                j = struct.pack('f',-float(index))
            elif(index[0] == "P"):
                index = re.sub('[P]', '', index)
                par = struct.pack('i',int(index))
         
        tab.append(o)
        tab.append(x)
        tab.append(y)
        tab.append(z)
        tab.append(i)
        tab.append(j)
        tab.append(par)
            
bfile.write(struct.pack('I',len(tab)))
for row in tab:
    for column in row:
       bfile.write(column)
                  
bfile.close()
gf.close()
