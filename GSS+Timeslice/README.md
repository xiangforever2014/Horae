# This folder contains the source code files of the baseline, GSS+Timeslice

## How to use?
### Environment
We implement Horae in a Red Hat Enterprise Linux Server release 6.2 with an Intel 2.60GHz CPU and 64GB RAM, the size of one cache line in the server is 64 bytes. 

The g++ version we use is 7.3.0.

Build & run

```txt
make
```
After executing the make command, we get the binary executable program "gss-timeslice".

### Configurations
Some important parameters setting and theirs descriptions are as follows.
| Command-line parameters | Descriptions                                       |
|:----------------------- | :------------------------------------------------- |
| **-w**                  | the width of the hash matrix                       |
| **-d**                  | the depth of the hash matrix                       |
| **-gl**                 | granularity length                                 |
| **-slot**               | slot numbers of one bucket                         |
| **-write**              | save test results to specified file                |
| **-fplength**           | fingerprint length                                 | 
| **-edgeweight**         | run edge weight query                              |
| **-edgeexistence**      | run edge existence query                           |
| **-nodeinweight**       | run node-in aggregated weight query                |
| **-nodeoutweight**      | run node-out aggregated weight query               |
| **-bool**               | run bool query                                     |
| **-dataset**            | choose dataset for testing                         |
| **-filename**           | the file path of dataset                           |
| **-input_dir**          | the folder path of input files                     |
| **-output_dir**         | the folder path of output files                    |
| **-para_query**         | execute query tasks in parallel                    |
| **-seq_query**          | execute query tasks serially                       |
| **-row_addrs**          | number of alternative addresses for matrix rows    |
| **-col_addrs**          | number of alternative addresses for matrix columns |
| **-write**              | output test results to file                        |


We give a simple example of how to use these parameters:
``` code
e.g. ./gss-timeslice -dataset <int> -filename <path> -w <int> -d <int> -gl <int> -fplength <int> -slot <int> -edgeweight -write -input_dir <path> -output_dir <path>

e.g. ./gss-timeslice -dataset 3 -filename ../Dataset/stackoverflow -w 20396 -d 20396 -gl 86400 -fplength 6 -para_query -write -input_dir ../TestFiles/stackoverflow/input/ -output_dir TestFiles/stackoverflow/output/ -edgeweight -write
```