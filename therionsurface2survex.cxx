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

const char VERSION[]  = "1.1.0";
const char PROGNAME[] = "therionsurface2survex";

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <iostream>
#include <fstream>

#include <chrono>
#include <ctime>

// Strings benutzbar machen
#include <string>
#include <cstring>
#include <regex>
#include <clocale> // setlocale()

using namespace std;

// define datapoint structs, so we can track if they were initialized ok
struct datapoint_i {
  bool   valid;
  long   d;
  datapoint_i() {
      valid = false;
  }
};
struct datapoint_f {
  bool   valid;
  double d;
  datapoint_f() {
      valid = false;
  }
};


/* Print version */
void version() {
    cout << PROGNAME << " Version " << VERSION << ", License GPLv3" <<endl;
}

/* Print usage info */
void usage () {
  version();
  cout << "Convert therion surface meshes to survex" << endl;
  cout << "Usage: [-hsdt] [-o outfile] [-i infile] -- [infile]"<<endl;
  cout << "  -o outfile    File to write to. Will be derived from infile if not specified."<<endl;
  cout << "  -i infile     File to read from, if not given by last parameter."<<endl;
  cout << "  -h            Print this help and exit."<<endl;
  cout << "  -s            Skip check of contents - process entire file (use in case" <<endl;
  cout << "                your therion grid data file has no \"surface\" declaration)" <<endl;
  cout << "  -t            Export in therion centerline format (implicit if -o ends with '.th')\n" <<endl;
  cout << "  -d            debug mode: spew any action on stdout" <<endl;
}


int main (int argc, char **argv)
{
  string inFile;
  string outFile;
  bool skipCheck = false;  // true=process entire file
  bool outFormatTH = false; // output in therion centerline format
  bool debug = false;


  // Parse cmdline options
  int index;
  int c;

  // support standard POSIX options
  for (int i=0; i<argc; i++) {
    if (NULL != strstr(argv[i],"--version")) { version(); return 0; }
    if (NULL != strstr(argv[i],"--help")) { usage(); return 0; }
  }

  opterr = 0;

  while ((c = getopt (argc, argv, "vhstdi:o:")) != -1)
    switch (c)
      {
      case 'h':
        usage();
        return 0;
        break;
      case 'v':
        version();
        return 0;
        break;
      case 'i':
        inFile = optarg;
        break;
      case 'o':
        outFile = optarg;
        break;
      case 's':
        skipCheck = true;
        break;
      case 't':
        outFormatTH = true;
        break;
      case 'd':
        debug = true;
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

  // switch to therion mode in case outfile was given and ends with *.th
  if (outFile != "" && outFile.length() >= 3 && outFile.substr(outFile.length()-3,3) == ".th" )
    outFormatTH = true;
  
  // generate default value for outFile in case nothing was given from user
  if (outFile == "") {
    if (outFormatTH) {
      outFile = inFile + ".th";
    } else {
      outFile = inFile + ".swx";
    }
  }

  
  // therion/survex prefixes
  std::string comment_prfx = (outFormatTH)? "#" : ";";
  std::string command_prfx = (outFormatTH)? "" : "*";
  

  // Read infile, parse and write result to outfile
  std::setlocale(LC_NUMERIC,"C"); // decial points always ".", not ","
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

    // write a nice header
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    h_outfile << comment_prfx << " converted by " << PROGNAME << " Version " << VERSION << "\n";
    h_outfile << comment_prfx << " on " << std::ctime(&now_time);
    h_outfile << comment_prfx << " from file " << inFile.c_str() << "\n\n";
    if (outFormatTH) {
      h_outfile << "survey surface\n";
      h_outfile << "centerline\n";
    }

    // prepare some regexes for parsing
    std::regex detect_surface_start_re ("^\\s*(surface|grid)");
    std::regex detect_surface_end_re ("^\\s*endsurface");
    std::regex parse_cmddirect_re ("^\\s*(-nothing So Far-)\\s+(.+)");
    std::regex parse_cs_re ("^\\s*(cs)\\s+(.+)");
    std::regex parse_grid_re ("^\\s*grid\\s+(-?\\d+[\\.\\d]*)\\s+(-?\\d+[\\.\\d]*)\\s+(\\d+[\\.\\d]*)\\s+(\\d+[\\.\\d]*)\\s+(\\d+)\\s+(\\d+)$");
    std::regex parse_data_re ("^(\\s*\\d+[\\d.]*)+$");
    std::regex parse_gridflip_re ("^\\s*grid-flip\\s+(none|vertical|horizontal)");
    std::regex parse_gdalhdr_re ("^\\s*(ncols|nrows|xllcorner|yllcorner|cellsize|dx|dy)\\s+(-?[.\\d]+)");

    bool in_surfacedata = false;
    double origin_x, origin_y, step_x, step_y;
    long cols_num, rows_num;
    long cur_row; // initialized either by grid-flip or grid command
    bool header_valid = false;
    datapoint_i gdal_ncols, gdal_nrows;
    datapoint_f gdal_xllcorner, gdal_yllcorner, gdal_xcellsize, gdal_ycellsize;
    bool gdal_detected = false;
    bool flip_vertical, flip_horizontal = false;
    std::vector<std::vector<double> > parsedData;  // holds parsed data rows
    while ( std::getline (h_infile, in_line) ) {
      line_nr++;
      if (debug) cout << "DBG: " << line_nr << ": " << in_line << '\n';
      
      
      // detect if we are inside a surface-block
      if (std::regex_search(in_line, detect_surface_start_re)) in_surfacedata = true;
      if (std::regex_search(in_line, detect_surface_end_re))   in_surfacedata = false;
      
      // Detect GDAL format header
      std::smatch sm_gdalhdr;
      if (std::regex_search(in_line, sm_gdalhdr, parse_gdalhdr_re)) {
        if (debug) cout << "  DBG: GDAL parse OK; cmd=" << sm_gdalhdr[1] << "; val=" << sm_gdalhdr[2] << "\n";
        gdal_detected = true;
        
        if (sm_gdalhdr[1] == "ncols")     {gdal_ncols.d     = stol(sm_gdalhdr[2]); gdal_ncols.valid = (gdal_ncols.d > 0)?true:false;}
        if (sm_gdalhdr[1] == "nrows")     {gdal_nrows.d     = stol(sm_gdalhdr[2]); gdal_nrows.valid = (gdal_nrows.d > 0)?true:false;}
        if (sm_gdalhdr[1] == "xllcorner") {gdal_xllcorner.d = stod(sm_gdalhdr[2]); gdal_xllcorner.valid = true;}
        if (sm_gdalhdr[1] == "yllcorner") {gdal_yllcorner.d = stod(sm_gdalhdr[2]); gdal_yllcorner.valid = true;}
        if (sm_gdalhdr[1] == "cellsize")  {gdal_xcellsize.d = stod(sm_gdalhdr[2]); gdal_xcellsize.valid = true;
                                           gdal_ycellsize.d = stod(sm_gdalhdr[2]); gdal_ycellsize.valid = true;}
        if (sm_gdalhdr[1] == "dx")        {gdal_xcellsize.d = stod(sm_gdalhdr[2]); gdal_xcellsize.valid = true;}
        if (sm_gdalhdr[1] == "dy")        {gdal_ycellsize.d = stod(sm_gdalhdr[2]); gdal_ycellsize.valid = true;}
        
        if (gdal_ncols.valid && gdal_nrows.valid && gdal_xllcorner.valid && gdal_yllcorner.valid && gdal_xcellsize.valid && gdal_ycellsize.valid) {
          if (debug) cout << "  DBG: GDAL header complete! \n";
          // once we have a full GDAL header, we can parse the data :)
          // simply fake grid command so the parser below can work it out
          
          // if step size is negative, grid is flipped
          if (gdal_xcellsize.d < 0) { gdal_xcellsize.d *= -1; flip_horizontal = true; }
          if (gdal_ycellsize.d < 0) { gdal_ycellsize.d *= -1; flip_vertical   = true; }
          
          in_line = "grid " 
          + to_string(gdal_xllcorner.d) + " "
          + to_string(gdal_yllcorner.d) + " "
          + to_string(gdal_xcellsize.d) + " "
          + to_string(gdal_ycellsize.d) + " "
          + to_string(gdal_ncols.d) + " "
          + to_string(gdal_nrows.d);
          
          skipCheck = true;
          
          if (debug) printf ("  DBG: line %i: valid GDAL header detected. Generated grid command: '%s'\n", line_nr, in_line.c_str());
        }
        
      }

      // try to parse any relevant commands
      if (skipCheck || in_surfacedata) {
        if (debug) cout << "DBG: precessing line " << line_nr << ": " << in_surfacedata << '\n';

        // direct commandos will go directly into the tgt file
        std::smatch sm_cmd;
        if (std::regex_search(in_line, sm_cmd, parse_cmddirect_re)) {
          h_outfile << command_prfx << sm_cmd[1] << " " << sm_cmd[2] << "\n";
          if (debug) printf ("  DBG: line %i: Parsed '%s' to direct cmd\n", line_nr, in_line.c_str());
        }

        // copy cs command in case it was given
        std::smatch sm_cs;
        if (std::regex_search(in_line, sm_cs, parse_cs_re)) {
          h_outfile << command_prfx << sm_cs[1] << " " << sm_cs[2] << "\n";
          if (!outFormatTH)
            h_outfile << command_prfx << "cs out" << " " << sm_cs[2] << "\n";
        }

        // digest grid-flip command
        std::smatch sm_gridflip;
        if (std::regex_search(in_line, sm_gridflip, parse_gridflip_re)) {
            if (sm_gridflip[1] == "none")       { flip_horizontal = false; flip_vertical=false; }
            if (sm_gridflip[1] == "vertical")   { flip_vertical   = true;  }
            if (sm_gridflip[1] == "horizontal") { flip_horizontal = true;  }
            cur_row = (flip_vertical)? 0 : rows_num-1; // may be overwritten by "grid" command
            if (debug) printf ("  DBG: line %i: Parsed '%s' to grid-flip: flip_vertical=%d; flip_horizontal=%d\n", line_nr, in_line.c_str(), flip_vertical, flip_horizontal);
        }
        

        // parse grid command, we need to get some values out:
        // grid 400080 5419750 10   10   4      5
        //      ^x0    ^y0     ^xs  ^ys  ^cnum  ^rnum
        std::smatch sm_grid;
        if (std::regex_search(in_line, sm_grid, parse_grid_re)) {
          origin_x = stod(sm_grid[1]);
          origin_y = stod(sm_grid[2]);
          step_x   = stod(sm_grid[3]);
          step_y   = stod(sm_grid[4]);
          cols_num = stol(sm_grid[5]);
          rows_num = stol(sm_grid[6]);
          if (debug) printf ("  DBG: line %i: Parsed '%s' to grid header: origin_x=%f, origin_y=%f, step_x=%f, step_y=%f, cols_num=%i, rows_num=%i\n", line_nr, in_line.c_str(), origin_x, origin_y, step_x, step_y, cols_num, rows_num);
          header_valid = true;

          // init result array
          parsedData.resize(rows_num);
          for(int i = 0 ; i < rows_num ; ++i) {
            parsedData[i].resize(cols_num);
          }
          cur_row = (flip_vertical)? 0 : rows_num-1; // may be overwritten by "grid-flip" command
        }
        
        // parse raw data. These are heights at the given point in the matrix.
        // We note this and after parsing all of this we will generate the data out of that.
        if (cols_num > 0 && rows_num > 0 && header_valid) {
            
            if (std::regex_match(in_line, parse_data_re)) {
                std::vector<std::string> tokens;
                std::stringstream mySstream(in_line);
                std::string temp;
                while( getline( mySstream, temp, ' ') ) {
                    if (temp != "") 
                      tokens.push_back( temp );
                }
                if (debug) printf ("  DBG: data regex matched: %i tokens/%i cols expected. Data:\n", tokens.size(), cols_num);
                if (debug) {
                    for (size_t i = 0; i < tokens.size(); ++i) 
                      cout << "     TOKEN[" << i << "]=" << ": '" << tokens[i] << "'\n";
                }
                
                if (tokens.size() == cols_num) {
                    long cur_col = (flip_horizontal)? cols_num-1 : 0;
                    long cur_col_step  = (flip_horizontal)? -1 : +1;
                    long cur_col_ref = 0;
                    for ( ; cur_col >= 0 && cur_col < tokens.size(); cur_col += cur_col_step) {
                       if (debug) printf ("  DBG: data stored: parsedData[%i][%i]='%f'\n", cur_row, cur_col, stod(tokens[cur_col]) );
                       parsedData[cur_row][cur_col_ref++] = stod(tokens[cur_col]);
                    }
                    (flip_vertical)? cur_row++ : cur_row--;
                }
            }
        }
        
      }



    } // done reading h_infile
    h_infile.close();
    
    
    // check if we had valid header data and thus where be able to parse the data
    if ( !header_valid ) {
        if (gdal_detected) {
            std::string missingHeaders;
            if (!gdal_ncols.valid) missingHeaders += " ncols";
            if (!gdal_nrows.valid) missingHeaders += " nrows";
            if (!gdal_xllcorner.valid) missingHeaders += " xllcorner";
            if (!gdal_yllcorner.valid) missingHeaders += " yllcorner";
            if (!gdal_xcellsize.valid) missingHeaders += " dx|cellsize";
            if (!gdal_ycellsize.valid) missingHeaders += " dy|cellsize";
            printf("No valid (or complete) GDAL header data found in '%s', missing:%s\n", inFile.c_str(), missingHeaders.c_str());
        } else {
            printf("No valid grid command found in '%s'\n", inFile.c_str());
        }
        
        h_outfile.close();
        return 2;
    }
    
    
    /* Ok, now we write the fix station data */
    h_outfile << command_prfx << "flags surface" << "\n";
    double tgt_x, tgt_y, tgt_z;
    double bbox_ur_x = origin_x + step_x * cols_num;
    double bbox_ur_y = origin_y + step_y * cols_num;
    if (debug) printf ("  DBG: BBox: lowerLeft=(%f / %f); upperRight=(%f / %f)\n", origin_x, origin_y, bbox_ur_x, bbox_ur_y);
    for (cur_row = 0; cur_row < parsedData.size(); ++cur_row) {
      for (long cur_col = 0; cur_col < parsedData[cur_row].size(); ++cur_col) {
          // *fix 20 200 000 1050
          //  name^  ^coords
          tgt_x     = origin_x + step_x * cur_col;
          tgt_y     = origin_y + step_y * cur_row;
          tgt_z     = parsedData[cur_row][cur_col];
          string tgt_name = "surface." + to_string(cur_row) + "." + to_string(cur_col);
          string outStr   = command_prfx + "fix " + tgt_name + " " + to_string(tgt_x) + " " + to_string(tgt_y) + " " + to_string(tgt_z);
          h_outfile << outStr.c_str() << "\n";
          
          if (debug) printf ("  DBG: data written: parsedData[%i][%i]='%f' => '%s'\n", cur_row, cur_col, parsedData[cur_row][cur_col], outStr.c_str() );
      }
    }
    
    /* Ok, now on to write the mesh */
    h_outfile << "\n" << command_prfx << "data nosurvey from to" << "\n";
    for (cur_row = 0; cur_row < parsedData.size(); ++cur_row) {
      for (long cur_col = 0; cur_col < parsedData[cur_row].size(); ++cur_col) {

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
    
    if (outFormatTH) {
      h_outfile << "endcenterline\n";
      h_outfile << "endsurvey\n";
    }
    
    h_outfile.close();

  } else {
    printf("Unable to open file '%s'\n", inFile.c_str()); 
    h_outfile.close();
    return 2;
  }



  return 0;
}

