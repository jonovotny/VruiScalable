import math
import re

filenames = ["/gpfs/runtime/opt/cave/ccv/share/vrsetup/cave_" + str(n) + ".vrsetup" for n in range(38)]
dicts = []

def parseline(line):
	split = re.split(' |	|\n',line)
	return (split[0], " ".join(split[1:]).strip(" 	\n"))

for filename in filenames:
	dicts.append({})
	with open(filename) as f:
		for line in f:
			attribute = parseline(line)
			dicts[-1][attribute[0]] = attribute[1]

screens = ["" for n in range(38)]
windows = ["" for n in range(38)]
arrayvals = ["" for n in range(38)]

for n in range(38):
	#print n
	#for key in ["#conf", "TileTopLeft", "TileTopRight", "TileBotLeft", "TileBotRight", "WindowWidth", "WindowHeight"]:
		#print key, dicts[n][key]
	botRight = dicts[n]["TileBotRight"].replace(" ", "").strip("()").split(",")
	topRight = dicts[n]["TileTopRight"].replace(" ", "").strip("()").split(",")
	botLeft = dicts[n]["TileBotLeft"].replace(" ", "").strip("()").split(",")
	br = [float(el) for el in botRight]
	tr = [float(el) for el in topRight]
	bl = [float(el) for el in botLeft]
	xdiff = [tr[i] - br[i] for i in range(3)]
	ydiff = [bl[i] - br[i] for i in range(3)]

	screens[n] += "		section screen" + str(n) + "\n"
	screens[n] += "			name screen" + str(n) + "\n"
	screens[n] += "			deviceMounted false\n"
	screens[n] += "			origin " + dicts[n]["TileBotRight"] + "\n"
	screens[n] += "			horizontalAxis (" + str(xdiff[0]) + ", " + str(xdiff[1]) + ", " + str(xdiff[2]) + ")\n"
	screens[n] += "			width " + str(math.sqrt(sum([xdiff[i]*xdiff[i] for i in range(3)]))) + "\n"
	screens[n] += "			verticalAxis (" + str(ydiff[0]) + ", " + str(ydiff[1]) + ", " + str(ydiff[2]) + ")\n"
	screens[n] += "			height " + str(math.sqrt(sum([ydiff[i]*ydiff[i] for i in range(3)]))) + "\n"
	screens[n] += "		endsection\n\n"

	windows[n] += "		section window" + str(n) + "\n"
	windows[n] += "			display " + dicts[n]["#conf"].replace(" ", "") + "\n"
	windows[n] += "			windowPos (0, 0), (" + dicts[n]["WindowWidth"] + ", " + dicts[n]["WindowHeight"] + ")\n"
	windows[n] += "			windowFullscreen false\n"
	windows[n] += "			windowType QuadbufferStereo\n"
	windows[n] += "			multisamplingLevel 1\n"
	windows[n] += "			screenName screen" + str(n) + "\n"
	windows[n] += "			viewerName YURTViewer\n"
	windows[n] += "			joinSwapGroup false\n"
	windows[n] += "		endsection\n\n"

	arrayvals[n] += "\"" + dicts[n]["#conf"].replace(" ", "") + "\", "

print "".join(screens)

print "".join(windows)

#print "".join(arrayvals)
