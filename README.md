# paq_history

This repository aims to serve as a history of development of the [PAQ compression programs](https://en.wikipedia.org/wiki/PAQ). I have scraped [Matt Mahoney's Data Compression page](http://mattmahoney.net/dc/) as well as the [encode.ru forum](https://encode.ru/) in order to find as many versions as possible.

My main development machine runs OSX, and, as such, I have modified the programs in order to compile and run successfully under OSX. I have also re-formatted all of the code with [clang-format](https://clang.llvm.org/docs/ClangFormat.html) in order to make the code more consistent and more readable.

There is a [test script](test.py) which will run all of the compiled programs, at all of their possible compression levels, against all of the files in the `testfiles` directory. It records some statistics and then writes the results to a database.

TODO:
- [ ] Add SQL snippets to set up database tables
- [ ] Fix remaining compile errors
- [ ] Fix remaining runtime errors
- [ ] Make test script also decompress the output file
- [ ] Make test script validate successful decompression
- [ ] Fix compilation for Linux
- [ ] Publish benchmark / test results
- [ ] Create some visualizations, maybe determine Pareto curve
- [ ] Get some more testfiles, including some edge cases
  - [ ] File with long repetitions
  - [ ] Incompressible file
  - [ ] Files of types that PAQ variants have special detection code for
- [ ] Add missing versions of PAQ8PX and PAQ8PXD
- [ ] Make a post on encode.ru showing off this repo :smiley:
