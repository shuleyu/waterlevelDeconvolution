# Download, Compile, Run

### Download:

```
$ git clone --recursive https://github.com/shuleyu/waterlevelDeconvolution.git
```

If `--recursive` is not added, the dependencies will not be downloaded. In such case, do: 

```
$ cd ./CPP-Library-Examples
$ git submodule update --init --recursive
```


### Compile: modify Makefile (instructions are in the file) then compile.

```
$ vim Makefile
$ make
```

### Run: modify "inputFileDeconParameters.txt" and run.

```
./DeconParameters.txt inputFileDeconParameters.txt
```

#### Result (for the sample inputFileDeconParameters.txt):
ls ./ExamplePcPevents/Event\_2012.12.21.22.28.08.80/results
