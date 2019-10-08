*************************************************************
*         CS315 JPEG Picture Collection README File         *
*************************************************************

CHANGE LOG:

4/12 -- Reduced the variable radii for all -vrad files, to
        avoid overdoing the variable radii).
4/09 -- Added large files and -vrad image maps for Problem 3.
4/08 -- Put up first graphics files.

*************************************************************

For your use on the programming assignment, we've collected
together a selection of pictures that you can use to test
your programs for PA1.  Feel free to substitute your own, but
please try to use pictures of similar size in pixels.

Here is a summary of the pictures . . .


These two should mostly be used for debugging your code:

testimg.jpg     227x149     Closeup of a rose
tree.jpg        600x400     Silhouette of a tree


For performance runs, generally use images of these sizes:

fall.jpg        1024x768    Trees in fall near a lake
green.jpg       1024x768    Ridge on north shore of Kauai
leaf.jpg        1024x768    Closeup of a leaf
redtree.jpg     1024x768    Larger silhouette of a tree


If your performance runs are so quick that clock resolution
is becoming a problem (i.e. the resolution of the clock on
each run is more than a couple % of the total runtime), then
feel free to switch to these much larger images:

gorge.jpg       1920x2560   A very narrow gorge with river
greatwall.jpg   1920x2560   The Great Wall of China
peacock.jpg     1920x2560   A peacock with a full tail fan

Using pictures this large should only be necessary, if at all,
for Problem 2 of the assignment, where the time-per-pixel in the
inner loop may be small.


In addition to each "base" image, there is also a "-vrad" version
of the image.  This is a variable-radius blur control map saved
as a grayscale image file.  When doing Problem 3, please use
the "-vrad" associated with the picture of your choice.  If you
use your own pictures, then you can just use one of the defaults
(redtree-vrad is a good, fairly "generic" choice) to control
blurring if your image matches the resolution of one of our
pictures, or use a bitmap graphics editor like Photoshop to make
your own.
