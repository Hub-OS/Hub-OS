import io
import os
import sys
import numpy as np
from PIL import Image

if len(sys.argv) <= 2:
	print("Provide (1) a source image as argument (2) target palette size")
	quit()
	
def convert_to_indexed_image(image, palette_size):
	# Convert to an indexed image
	indexed_image = image.convert('RGBA').convert(mode='P', dither='NONE', colors=palette_size) # Be careful it can remove colors
	# Save and load the image to update the info (transparency field in particular)
	f = io.BytesIO()
	indexed_image.save(f, 'png')
	indexed_image = Image.open(f)
	# Reinterpret the indexed image as a grayscale image
	grayscale_image = Image.fromarray(np.asarray(indexed_image), 'L')
	# Create the palette
	palette = indexed_image.getpalette()
	transparency = list(indexed_image.info['transparency'])
	palette_colors = np.asarray([[palette[3*i:3*i+3] + [transparency[i]] \
		for i in range(palette_size)]]).astype('uint8')
	palette_image = Image.fromarray(palette_colors, mode='RGBA')
	return grayscale_image, palette_image

path = sys.argv[1]
size = int(sys.argv[2])
name = os.path.splitext(path)[0]

source = Image.open(path)
greyscale, palette = convert_to_indexed_image(source, size)

greyscalepath = name+".greyscaled.png"
palettepath = name+".palette.png"

greyscale.save(greyscalepath,'png')
palette.save(palettepath, 'png') 

print("[OK] Dumped " + greyscalepath + " and " + palettepath)