# Algorithm

`bitcount` counts the number of known bits, including the start bit. Receive
data bits are sted in `currentByte`. When an edge is detected, `bitcount` is
checked. 

If `bitcount == 0` and `level == 0`, this is a start bit edge; set `bitcount = 1` and
record the time. If `bitcount == 0` and `level == 1`