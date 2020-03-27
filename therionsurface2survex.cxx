#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <iostream>
#include <fstream>

// Strings benutzbar machen
#include <string>
#include <regex>

using namespace std;



void usage () {
  cout << "Convert therion surface meshes to survex"<<endl;
  cout << "Usage: [-h] [-o outfile] [-i infile] -- [infile]"<<endl;
  cout << "  -o outfile    File to write to. Will be derived from infile if not specified."<<endl;
  cout << "  -i infile     File to read from, if not given by last parameter."<<endl;
  cout << "  -h            Print this help and exit."<<endl;
}



int main (int argc, char **argv)
{
  string inFile;
  string outFile;


  // Parse cmdline options
  int index;
  int c;

  opterr = 0;

  while ((c = getopt (argc, argv, "hi:o:")) != -1)
    switch (c)
      {
      case 'h':
        usage();
        return 0;
        break;
      case 'i':
        inFile = optarg;
        break;
      case 'o':
        outFile = optarg;
        break;
      case '?':
        if (optopt == 'i' || optopt == 'o')
          fprintf (stderr, "Option -%c requires an argument.\n", optopt);
        else if (isprint (optopt))
          fprintf (stderr, "Unknown option `-%c'.\n", optopt);
        else
          fprintf (stderr,
                   "Unknown option character `\\x%x'.\n",
                   optopt);
        return 1;
      default:
        abort ();
      }


  // Test for file argument at the end
  if (argc - optind > 1) {
    printf ("More than one option argument given (%d), expected just filename of infile\n", argc - optind);
    return 1;
  }
  if (argc - optind == 1 ) {
    inFile =  argv[optind];
  }


  // sanity check
  if (inFile == "") {
    cout << "Error: no input file specified!" <<endl;
    return 1;
  }

  // generate default value for outFile in case nothing was given from user
  if (outFile == "") {
    outFile = inFile + ".swx";
  }




  // Read infile, parse and write result to outfile
  ifstream h_infile;
  string in_line;
  h_infile.open(inFile.c_str());
  long line_nr = 0;
  if (h_infile.is_open()) {
    // prepare some regexes for parsing
    std::regex detect_surface_start_re ("surface", std::regex_constants::extended);
    std::regex detect_surface_end_re ("endsurface");


    bool in_surfacedata = false;
    while ( getline (h_infile, in_line) ) {
      line_nr++;
      cout << line_nr << ": " << in_line << '\n';

      if (std::regex_search(in_line, detect_surface_start_re)) in_surfacedata = true;
      if (std::regex_search(in_line, detect_surface_end_re))   in_surfacedata = false;
      cout << "    in_surfacedata="<<in_surfacedata<<"\n";

// if (std::regex_match(fname, base_match, base_regex)) {
            // The first sub_match is the whole string; the next
            // sub_match is the first parenthesized expression.
//            if (base_match.size() == 2) {
//                std::ssub_match base_sub_match = base_match[1];
//                std::string base = base_sub_match.str();
//                std::cout << fname << " has a base of " << base << '\n';
//            }
//        }	



    }
    h_infile.close();

  } else {
    printf("Unable to open file '%s'\n", inFile.c_str()); 
    return 2;
  }



  return 0;
}

