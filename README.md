# CSPB2270---Final
Final Project for CSPB 2270 Summer 2025

My initial idea was to use quadtrees to create the Conway Game of Life, but after some deeper diving into it, I realized that it was not really possible to do so without implementing the hashlife algorithm. My research on that algorithm turned up that it is incredibly complicated, and I did not feel like it would be fair for me to attempt to implement that, or I would have to copy most of it to make the project work, which would be the real bread and butter of the project. So I decided to find a different use of quadtrees, and I did so by creating height maps.

I did a lot of research on heightmaps and came acorss the Perlin Noise Algorithm which I use in my code. My program has three main parts.

1) The Perlin Noise Algorithm is run and generates seeds that reflect a random grayscale image that represents a height map. In this image lighter colors represent areas of higher elevation (mountains, hills, etc.) and darker colors reprents areas for lower elevation (valley, canyons, etc.).
2) The quadtree subdivides the image into four quadrants, from there it finds the quadrant that is the most mountainous, and continues to subdivide. This gives you a clear image of what is the tallest section of the heightmap, as well as indicating that the larger sections of squares are indicitive of flatter areas.
3) It visualizes the processes stated above and outputs an image that can be viewed. This image includes the grayscale map, as well as the quadrants laid out by the quadtrees.

You should be able to simply build the project and it will output the image for viewing.
