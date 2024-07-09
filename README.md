# DramaNg
DramaNg is a new implementation of the Drama Covert Channel published by
[Pessl et al](https://www.usenix.org/system/files/conference/usenixsecurity16/sec16_paper_pessl.pdf).
with some additional features (e.g. dumping memory accesses of multiple banks
in parallel, some additional initialization sequences, etc.)

## Build
* Build the binary using `make`:
```
make
```

## Usage
### Determine the Threshold
* Measure the threshold between row hit and row conflict on your system and use
  is as base value
* Run both of the following commands at the same time on your system, e.g. by
  using two shells:
```
taskset 0x1 ./bin/drama-ng -g ddr5 -R 32 -z 12 -d -t <threshold>
taskset 0x2 ./bin/drama-ng -g ddr5 -s "Hello World!" -R 32 -t <threshold>
```
* Read the debug output of the receiver process (first command) to determine if
  the threshold is too high (too many row hits) or too low (too many row
  conflicts) and adjust the threshold until the string `Hello World!` is
  received successfully.

### Use the covert channel
* The transmission rate can be increased by reducing the value of the `-R`
  command-line parameter. This parameter specifies the window as 2^R. It is also
  possible to use the `-r` command-line parameter instead to directly set the
  window (not as power of two).
* Instead of writing the string that should be transmitted to the command-line,
  it is also possible to use the `-s` command-line flag to specify a file. In
  that case, the content of the file is read and transmitted.
## Evaluation
* The receiver prints the data to the standard output and every other output to
  standard error. Therefore, the output can be piped to a file:
```
taskset 0x1 ./bin/drama-ng -g ddr5 -R 32 -z 12 -d -t <threshold> > received.txt
```
* You should name your files `send.txt` and `received.txt`. Afterwards, you can
  use the python script `evaluation/levenshtein.py` to get the levenshtein
  distance between both files (based on bits, not on bytes):
```
python evaluation/levenshtein.py
```
