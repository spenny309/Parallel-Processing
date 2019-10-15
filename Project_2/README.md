Part 1 of Project 2 for CS338, Fall 2019

Spence Carrillo

Lines 55 and 56 allow user to define or undefine ROW_OR_COL and SEQ_OR_INT

If ROW_OR_COL is defined, processing will execute in row-major order, otherwise processing will execute in column-major order

If SEQ_OR_INT is defined, processing will execute in chunk-style, otherwise processing will execute in interleaved-style

To run:

./prog2 {number of processors} {input image file} {output image file}