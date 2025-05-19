import os
import re
from PIL import Image

# Regex pattern to match filenames and extract values (supports negative numbers)
pattern = re.compile(r'cv(-?\d+)_v(-?\d+)_h(-?\d+)_osc(-?\d+)\.png')

# Store images with their coordinates
images = []

# Read and parse files
for filename in os.listdir('.'):
    match = pattern.match(filename)
    if match:
        cv, v, h, osc = map(int, match.groups())
        img = Image.open(filename)
        images.append({'cv': cv, 'v': v, 'h': h, 'osc': osc, 'img': img})

# Exit if no images found
if not images:
    print("No matching images found.")
    exit()

# Image dimensions (assuming all images are same size)
img_width, img_height = images[0]['img'].size

# Output image size
rows = 1
cols = len(images)
output_width = cols * img_width
output_height = rows * img_height

# Create blank output image
output_image = Image.new('RGBA', (output_width, output_height), (0, 0, 0, 0))

# Paste images into the grid
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

# Save the output
output_image.save('combined_output.png')
print("Combined image saved as combined_output.png")

