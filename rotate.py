import os
import sys

if os.path.isfile(sys.argv[2]):
	os.rename(sys.argv[2], sys.argv[2] + ".backup")

with open(sys.argv[1]) as f:
	for line in f:
		newfile = open(sys.argv[2], 'a')
		newfile.write(line)
