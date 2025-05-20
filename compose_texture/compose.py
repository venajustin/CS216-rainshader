#
# Compose.py  
# Justin Greatorex 
#
# Inteded for use with (cave.cs.columbia.edu/repository/Rain)
# Place images of one camera angle in this folder and this script will combine
# them into a single image for use in they rain shader program
#

import os
import re
from PIL import Image

# parse matching image names and capture values
pattern = re.compile(r'cv(-?\d+)_v(-?\d+)_h(-?\d+)_osc(-?\d+)\.png')

images = []

for filename in os.listdir('.'):
    match = pattern.match(filename)
    if match:
        cv, v, h, osc = map(int, match.groups())
        img = Image.open(filename)
        images.append({'cv': cv, 'v': v, 'h': h, 'osc': osc, 'img': img})

if not images:
    print("No matching images found.")
    exit()

# we assume all images are the same size
img_width, img_height = images[0]['img'].size

# we have one row and all images make up the width
rows = 1
cols = len(images)
output_width = cols * img_width
output_height = rows * img_height

output_image = Image.new('RGBA', (output_width, output_height), (0, 0, 0, 0))


# organizes images in the correct ordering for later indexing
for item in images:

    index = 0
    index += -80 + (((item['v'] + 90) / 20) * 90)
    if item['v'] == 90:
        index -= 80
    index += 10 * ((item['h'] - 10) / 20)
    index += item['osc']

    x = int(img_width * index)
    y = 0
    output_image.paste(item['img'], (x, y))

output_image.save('combined_output.png')
print("Combined image saved as combined_output.png")

