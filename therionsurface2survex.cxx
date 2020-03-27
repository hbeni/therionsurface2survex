/***********************************************************************
* This program is free software: you can redistribute it and/or modify *
* it under the terms of the GNU General Public License as published by *
* the Free Software Foundation, either version 3 of the License, or    *
* (at your option) any later version.                                  *
*                                                                      *
* This program is distributed in the hope that it will be useful,      *
* but WITHOUT ANY WARRANTY; without even the implied warranty of       *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
* GNU General Public License for more details.                         *
************************************************************************
*
* Program to convert therion surface data to survex format
*
* @author 2020 Benedikt Hallinger <beni@hallinger.org>
*/

const char VERSION[] = "0.9";

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


/* Print usage info */
void usage () {
  cout << "therionsurface2survex Version "<< VERSION << ", License GPLv3" <<endl;
  cout << "Convert therion surface meshes to survex" << endl;
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
  ofstream h_outfile;
  h_infile.open(inFile.c_str());
  h_outfile.open(outFile.c_str());
  long line_nr = 0;
  if (! h_outfile.is_open()) {
    printf("Unable to open file '%s'\n", outFile.c_str());
    return 2;
  }
  if (h_infile.is_open()) {
    // strings for reading/writing
    std::string in_line;
    std::string out_line;

    // prepare some regexes for parsing
    std::regex detect_surface_start_re ("^\\s*(surface|grid)");
    std::regex detect_surface_end_re ("^\\s*endsurface");
    std::regex parse_cmddirect_re ("^\\s*(cs)\\s+(.+)");
    std::regex parse_grid_re ("^\\s*grid\\s+(\\d+[.\\d]*)\\s+(\\d+[.\\d]*)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)$");
    std::regex parse_data_re ("^(\\s*\\d+[\\d.]*)+$");

    bool in_surfacedata = false;
    float origin_x, origin_y;
    long step_x, step_y, cols_num, rows_num;
    long cur_col, cur_row = 0;
    std::vector<std::vector<float> > parsedData;  // holds parsed data rows
    while ( std::getline (h_infile, in_line) ) {
      line_nr++;
      //cout << "DBG: " << line_nr << ": " << in_line << '\n';
      
      
      // detect if we are inside a surface-block
      if (std::regex_search(in_line, detect_surface_start_re)) in_surfacedata = true;
      if (std::regex_search(in_line, detect_surface_end_re))   in_surfacedata = false;

      // try to parse any relevant commands
      if (in_surfacedata) {
        //cout << "DBG: precessing line " << line_nr << ": " << in_surfacedata << '\n';

        // direct commandos will go directly into the tgt file
        std::smatch sm_cmd;
        if (std::regex_search(in_line, sm_cmd, parse_cmddirect_re)) {
          h_outfile << "*" << sm_cmd[1] << " " << sm_cmd[2] << in_surfacedata << "\n";
          //printf ("  DBG: line %i: Parsed '%s' to direct cmd\n", line_nr, in_line.c_str());
        }

        // parse grid command, we need to get some values out:
        // grid 400080 5419750 10   10   4      5
        //      ^x0    ^y0     ^xs  ^ys  ^cnum  ^rnum
        std::smatch sm_grid;
        if (std::regex_search(in_line, sm_grid, parse_grid_re)) {
          origin_x = stof(sm_grid[1]);
          origin_y = stof(sm_grid[2]);
          step_x   = stol(sm_grid[3]);
          step_y   = stol(sm_grid[4]);
          cols_num = stol(sm_grid[5]);
          rows_num = stol(sm_grid[6]);
          //printf ("  DBG: line %i: Parsed '%s' to origin_x=%f, origin_y=%f, step_x=%i, step_y=%i, cols_num=%i, rows_num=%i\n", line_nr, in_line.c_str(), origin_x, origin_y, step_x, step_y, cols_num, rows_num);
          
          // init result array
          parsedData.resize(rows_num);
          for(int i = 0 ; i < rows_num ; ++i) {
            parsedData[i].resize(cols_num);
          }
        }
        
        // parse raw data. These are heights at the given point in the matrix.
        // We note this and after parsing all of this we will generate the data out of that.
        if (cols_num > 0 && rows_num > 0 ) {
            
            if (std::regex_match(in_line, parse_data_re)) {
                std::vector<std::string> tokens;
                std::stringstream mySstream(in_line);
                std::string temp;
                while( getline( mySstream, temp, ' ') ) {
                    if (temp != "") 
                      tokens.push_back( temp );
                }
                //printf ("  DBG: data regex matched: %i tokens/%i cols expected. Data:\n", tokens.size(), cols_num);
                //for (size_t i = 0; i < tokens.size(); ++i) 
                //   cout << "     TOKEN[" << i << "]=" << ": '" << tokens[i] << "'\n";
                
                if (tokens.size() == cols_num) {
                    for (cur_col = 0; cur_col < tokens.size(); ++cur_col) {
                       //printf ("  DBG: data stored: parsedData[%i][%i]='%f'\n", cur_row, cur_col, stof(tokens[cur_col]) );
                       parsedData[cur_row][cur_col] = stof(tokens[cur_col]);
                    }        
                  cur_row++;
                }
            }
        }
        
      }



    }
    h_infile.close();
    
    
    /* Ok, now we write the fix station data */
    h_outfile << "*flags surface" << "\n";
    float tgt_x, tgt_y, tgt_z;
    for (cur_row = 0; cur_row < parsedData.size(); ++cur_row) {
      for (cur_col = 0; cur_col < parsedData[cur_row].size(); ++cur_col) {
          // *fix 20 200 000 1050
          //  name^  ^coords
          tgt_x     = origin_x + step_x * cur_col;
          tgt_y     = origin_y + step_y * cur_row;
          tgt_z     = parsedData[cur_row][cur_col];
          string tgt_name = "surface." + to_string(cur_row) + "." + to_string(cur_col);
          string outStr   = "*fix " + tgt_name + " " + to_string(tgt_x) + " " + to_string(tgt_y) + " " + to_string(tgt_z);
          h_outfile << outStr.c_str() << "\n";
          
          //printf ("  DBG: data written: parsedData[%i][%i]='%f' => '%s'\n", cur_row, cur_col, parsedData[cur_row][cur_col], outStr.c_str() );
      }
    }
    
    /* Ok, now on to write the mesh */
    h_outfile << "\n*data nosurvey from to" << "\n";
    for (cur_row = 0; cur_row < parsedData.size(); ++cur_row) {
      for (cur_col = 0; cur_col < parsedData[cur_row].size(); ++cur_col) {

          string src_name = "surface." + to_string(cur_row) + "." + to_string(cur_col);
          
          // we will link the current station to the direct neighbours
          int next_col = cur_col + 1;
          if (next_col < parsedData[cur_row].size()) {
            string tgt_name = "surface." + to_string(cur_row) + "." + to_string(next_col);
            string outStr   =  src_name + " " + tgt_name;
            h_outfile << outStr.c_str() << "\n";
          }
          
          int next_row = cur_row + 1;
          if (next_row < parsedData.size()) {
            string tgt_name = "surface." + to_string(next_row) + "." + to_string(cur_col);
            string outStr   =  src_name + " " + tgt_name;
            h_outfile << outStr.c_str() << "\n";
          }
      }
    }
    
    h_outfile.close();

  } else {
    printf("Unable to open file '%s'\n", inFile.c_str()); 
    h_outfile.close();
    return 2;
  }



  return 0;
}

