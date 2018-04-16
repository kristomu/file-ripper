// 1.0.2: Added estimated time to arrival and a few rippers -KM 2007-05-31

// --------------------------------------------------------------------------

// Bluesky: Add all ANSI terminal codes to the ans ripper to get rid of false
// positives once and for all.

// TODO: Fix the crash in the MBBS userinfo ripper. This will require that we
// pass s.begin() to is_header and dump_from_header. [DONE, end works too]

// TODO: Include gdm.txt, so that others will be spared the insanity of trying
// to find it on the web. [DONE]

// TODO: "Timeout" values for formats where large files don't make sense.
// 	In a utopia, we should then spool back the counter to search from
// 	where the header left off, but we're not there.
// 	It might be easier to divide this into two parts - one that finds
// 	headers and footers, and another one that estimates size and dumps.
// 	[DONE]

// TODO: Add ZZT. FF FF then [0..7F] then 12 bytes of 00 then 64 00. (100 health)
// 	RK-match against 00 00 64 00.
// Also add JPEG and MZX. JPEG can be easily RK-searched, MZX is more complex.
// (Though we must recode the advance function for jpeg since the footer must
//  be preceded by a certain ff sequence without any other ff sequence after 
//  it)
// Once we've got JPEG, MZX, and ZZT, (and MZB?,) then this thing is feature-
// compatible with the find* programs I've got elsewhere. Then the fun starts.
// [DONE]

// Max theoretical zzt size is 2.54 MB (20k * 127 boards), but no game file
// I've ever seen has been > 350K. (FRED2 gets close)

// For kicks, implement LHA/LHx. We only have three consistent bytes, so no
// RK here. [DONE, uses 2-byte RK]

// Other interesting formats (among those in magic) that haven't been 
// implemented yet:
// 0       string          LZX             LZX compressed archive (Amiga)
// 257     string          ustar\0         POSIX tar archive
// 257     string          ustar\040\040\0 GNU tar archive
// 0       lelong&0x8080ffff       0x0000081a      ARC archive data, dynamic LZW
// 0       string          Rar!            RAR archive data,
// 0       string  MThd                    Standard MIDI data
// 0       string          \037\213        gzip compressed data
// 	(usually 8 afterwards)
// 1       belong&0xfff7ffff       0x01010000      Targa image data - Map
// 1       belong&0xfff7ffff       0x00020000      Targa image data - RGB
// 1       belong&0xfff7ffff       0x00030000      Targa image data - Mono
// 0       beshort         0x0a00  PCX ver. 2.5 image data
// 0       beshort         0x0a02  PCX ver. 2.8 image data, with palette
// 0       beshort         0x0a03  PCX ver. 2.8 image data, without palette
// 0       beshort         0x0a04  PCX for Windows image data
// 0       beshort         0x0a05  PCX ver. 3.0 image data
// 0       string  @
// >1      string/cB       rem\            MS-DOS batch file text
// >1      string/cB       set\            MS-DOS batch file text

// If any of these uses an overlay, they won't be caught, just like PE
//0       string  MZ              MS-DOS executable
//>0xe7   string  LH/2\ Self-Extract \b, %s
//>0x1c   string  diet \b, diet compressed
//>0x1c   string  LZ09 \b, LZEXE v0.90 compressed
//>0x1c   string  LZ91 \b, LZEXE v0.91 compressed
//>0x1c   string  tz \b, TinyProg compressed
//>0x1e   string  PKLITE \b, %s compressed
//>0x64   string  W\ Collis\0\0 \b, Compack compressed
//>0x24   string  LHa's\ SFX \b, LHa self-extracting archive
//>0x24   string  LHA's\ SFX \b, LHa self-extracting archive
//>0x24   string  \ $ARX \b, ARX self-extracting archive
//>0x24   string  \ $LHarc \b, LHarc self-extracting archive
//>0x20   string  SFX\ by\ LARC \b, LARC self-extracting archive
//>1638   string  -lh5- \b, LHa self-extracting archive v2.13S

//0       belong  0x31be0000                      Microsoft Word Document
//0       string  PO^Q`                           Microsoft Word 6.0 Document
//0       string  \376\067\0\043                  Microsoft Office Document
//0       string  \320\317\021\340\241\261\032\341        Microsoft Office Document
//0       string  \333\245-\0\0\0                 Microsoft Office Document

// (HTML only matches <html>)
//0   string/cB   \<!DOCTYPE\ html        HTML document text
//0   string/cb   \<head                  HTML document text
//0   string/cb   \<title                 HTML document text
//0   string/cb   \<html                  HTML document text
//
//
// among those not in magic (as much as is feasible)
// 	.ASM, .PAS, .COM, .PCX, .CHR, .PAL, .SAM, .RTF, .3DS, .MID, 
// 	[MBBS data files], [PKLITE]

#include <stdio.h>
#include <errno.h>
#include <error.h>
#include <ext/rope>
#include <vector>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctype.h>
#include <math.h>

#include <time.h>
#include <sys/time.h>

#include "implements.cc"

#include "rkarp.cc"

namespace Sgi = ::__gnu_cxx;       // GCC 3.1 and later
using namespace std;


void fail(char* s) { 
  fprintf(stderr, "%s errno = %d, ", s, errno); 
  perror(NULL);
  fprintf(stderr, "\n");
  exit(1); 
}

string itos(long long number) {
	ostringstream xav;
	xav << number;
	return(xav.str());
}

string itos(long long number, int padding) {
	string retval = itos(number);
	while (retval.size() < padding) retval = "0" + retval;
	return(retval);
}

// Time

double get_time() {
	timeval tv;
	if (gettimeofday(&tv, NULL) == 0)
		return (tv.tv_sec + tv.tv_usec/1e6);
	else	return (0);
}

string hhmmss(unsigned int timeval) {
	unsigned int seconds = timeval % 60;
	timeval = (timeval - seconds) / 60;
	unsigned int minutes = timeval % 60;
	timeval = (timeval - minutes) / 60;
	unsigned int hours = timeval;

	return(itos(hours, 2) + ":" + itos(minutes, 2) + ":" +
			itos(seconds, 2));
}

string eta(double started_at, double now, double fraction_done) {
	unsigned int remaining = (unsigned int) round(( (now-started_at) /
				fraction_done ) * (1-fraction_done));

	return(hhmmss(remaining));
}

class file_char_prod : public Sgi::char_producer<char> {
  public:
    FILE* f;
    file_char_prod(const char *file_name) {
      if (NULL == (f = fopen(file_name, "rb"))) 
        fail("Open failed");
    }

    ~file_char_prod() { fclose(f); }

    virtual void operator()(size_t start_pos, size_t len, char* buffer) {
      if (fseek(f, start_pos, SEEK_SET)) fail("Seek failed");
      if (fread(buffer, sizeof(char), len, f) < len) fail("Read failed");
    }

    long len() {
        // Return the length of a file; this is the only
        //   mechanism that the standard C library makes possible.
      if (fseek(f, 0, SEEK_END)) fail("Seek failed");
      return ftello(f);
    }
};

vector<ripper *> get_all_rippers() {
	
	// Ew. But got any better ideas?
	vector<ripper *> rippers(44);
	rippers[0] = new png_ripper;
	rippers[1] = new html_ripper;
	rippers[2] = new gif_ripper;
	rippers[3] = new zip_ripper;
	rippers[4] = new lbm_ripper;
	rippers[5] = new pbm_ripper;
	rippers[6] = new anm_ripper;
	rippers[7] = new bat_ripper;
	rippers[8] = new compressed_bas_ripper;
	rippers[9] = new rtf_ripper;
	rippers[10] = new hlp_ripper;
	rippers[11] = new wav_ripper;
	rippers[12] = new wri_ripper;
	rippers[13] = new jpg_ripper;
	rippers[14] = new arj_ripper;
	rippers[15] = new exe_ripper;
	rippers[16] = new ans_ripper;
	rippers[17] = new zzt_ripper;
	rippers[18] = new frm_ripper;
	rippers[19] = new uncompressed_bas_ripper;
	rippers[20] = new bmp_ripper;
	rippers[21] = new mzb_ripper;
	rippers[22] = new fli_ripper;
	rippers[23] = new msp_ripper;
	rippers[24] = new doc_ripper;
	rippers[25] = new mzx_ripper;
	rippers[26] = new gdm_ripper;
	rippers[27] = new it_ripper;
	rippers[28] = new s3m_ripper;
	rippers[29] = new mzx_sav_ripper;
	rippers[30] = new zzt_brd_ripper;
	rippers[31] = new lha_ripper;
	rippers[32] = new mod_ripper;
	rippers[33] = new xm_ripper;
	rippers[34] = new voc_ripper;
	rippers[35] = new svx_ripper;
	rippers[36] = new abm_ripper;
	rippers[37] = new cdr_ripper;
	rippers[38] = new mbbs_conf_ripper;
	rippers[39] = new mbbs_callers_ripper;
	rippers[40] = new mbbs_users_ripper;
	rippers[41] = new quetzal_ripper;
	rippers[42] = new lendian_tif_ripper;
	rippers[43] = new bendian_tif_ripper;

	return(rippers);
}

// This function registers a Rabin-Karp clue from a specified ripper. It does
// this by first getting available clues from the ripper, then going through
// them until one is found that does not produce any collisions. Note that the
// registration function does not order by performance, as it doesn't know the
// distribution of n-grams in the file in question. The function returns the
// offset to the clue (i.e position in the header we're looking for), or -1 if
// it couldn't register a clue.
int register_clue(ripper & source, RKSearch & case_sensitive_searcher,
		RKSearch & case_insensitive_searcher, int wanted_clue_length,
		int ripper_code) {

	bool success = false;
	int possible_offset;
	vector<clue_bundle> retval = source.get_clues(wanted_clue_length);
	
	//cout << "CLUE: " << retval.size() << endl;
	
	for (int sec = 0; sec < retval.size() && !success; sec++) {
		possible_offset = retval[sec].offset;

		if (retval[sec].case_sensitive)
			success = case_sensitive_searcher.add_all(retval[sec].
					clues, ripper_code);
		else
			success = case_insensitive_searcher.add_all(
					retval[sec].clues, ripper_code);
	}

	if (!success) {
		cout << "Couldn't add a " << wanted_clue_length << "-char" <<
			" clue for " << source.get_extension() << endl;
		return(-1);
	}

	return(possible_offset + wanted_clue_length - 1);
}

void register_all_clues(vector<ripper *> & rippers, RKSearch & 
		case_sensitive_searcher, RKSearch & case_insensitive_searcher,
		vector<bool> & registered, int wanted_clue_length,
		vector<int> & clue_offsets) {

	for (int counter = 0; counter < rippers.size(); counter++) {
		// if already registered, ignore it
		if (registered[counter]) continue;
		
		int potential_clue_location = register_clue(*rippers[counter],
				case_sensitive_searcher, 
				case_insensitive_searcher, wanted_clue_length, 
				counter);

		if (potential_clue_location != -1)
			clue_offsets[counter] = potential_clue_location;

		registered[counter] = (potential_clue_location != -1);
	}
}

// This finds the correct position of the header given a match by RKSearch,
// as well as the maximum offset. Count is the position in the file,
// btmax is the maximum offset, and the function returns -1 if no header was
// found. Dont_rip_before is a limit that does what it says: the function
// should return -1 unless we're at or past that byte, no matter if there's a 
// header or not.
int find_header_offset(ripper & this_ripper, size_t count, int btmax,
		size_t dont_rip_before, char_rope::const_iterator & current,
		const char_rope::const_iterator end) {
	bool found_header = false;
	int confirmed;

	if (count < dont_rip_before) return(-1);
	
	for (int backtrace = btmax; backtrace > 0 && !found_header; 
			backtrace--) {
		// don't fall off the beginning
		if (backtrace > count) continue;

		// Don't rip the same thing twice
		// We need this convoluted form as count is unsigned. Thus,
		// doing a subtraction without checking would falsely trigger
		// the condition through an overflow.
		
		if (count >= backtrace)
			if (count - backtrace < dont_rip_before)
				continue;

		// Did we find anything?
		if (this_ripper.is_header_here(current - backtrace, end)) {
			found_header = true;
			confirmed = backtrace;
		}
	}

	if (found_header) return(confirmed);
	else return(-1);
}

// this gets the array index from a ripper extension. We use it to find the
// ZZT board ripper (in rip_single_file) and to disable rippers (in 
// get_parameters). It returns -1 if nothing was found.
int get_index_by_extension(string extension, vector<ripper *> rippers) {
	for (int counter = 0; counter < rippers.size(); counter++)
		if (rippers[counter]->get_extension() == extension)
			return(counter);

	return(-1);
}


// TODO: Implement zzt_rigor
void rip_single_file(const char * input_file, vector<ripper *> rippers,
		vector<bool> disabled_rippers, bool zzt_rigor, bool
		board_skip, bool override_suggestions, int maxlen,
		string prefix_out) {
  
  file_char_prod* fcp = new file_char_prod(input_file);
  Sgi::rope<char> s(fcp, fcp->len(), true);
  Sgi::rope<char>::const_iterator current;

  int counter;

  // Find out which ripper is the ZZT board ripper. We need this for 
  // communication between the ZZT ripper and the BRD ripper.
  int zzt_board_ripper = get_index_by_extension(".brd", rippers);

  // Set the ZZT .BRD ripper to err liberal or conservative based on user input
  if (!zzt_rigor)
	  ((zzt_brd_ripper *)(rippers[zzt_board_ripper]))->set_rigor(zzt_rigor);

  vector<int> files_ripped(rippers.size(), 0);
  vector<size_t> next_valid_position(rippers.size(), 0);
  size_t count = 0; // - UNSIGNED -

  // Eventually we'll have a get_clue and get_clue_distance built into
  // the ripper classes. Rippers that don't have clues return false or -1.

  // distance between end of RKSearch "clue" and beginning of the header
  // This is used to "spool back" when we think we've found something.
  // We do this because some headers may have initial wildcard bytes which
  // we can't search for -- in that case, searching for a later rare sequence
  // and then "going backwards" works.
  vector<int> clue_location(rippers.size(), 3); // default four bytes away
  
  int match_len = 4;
  RKSearch searcher(true, match_len); // case sensitive, match 4 bytes
  RKSearch ci_searcher(false, match_len); // case insensitive, match 4 bytes
  // add stuff to our RKSearches, one ripper at a time

  vector<string> possible_clues;
  int possible_offset;
  vector<bool> succeeded(rippers.size());

  // Make succeeded equal to disabled_ripper. That way, rippers that are set
  // to be disabled are considered "already registered" by register_all_clues,
  // leading to no clues being set for that ripper. This effectively disables
  // the ripper because it'll never match anything.
  succeeded = disabled_rippers;
  
  // PHEW!
  register_all_clues(rippers, searcher, ci_searcher, succeeded, match_len,
		  clue_location);
 
  // And if at first you don't succeed.. 2-byte matcher
  int nmlen = 2;
  RKSearch two_byte(true, nmlen);

  register_all_clues(rippers, two_byte, two_byte, succeeded, nmlen,
		  clue_location);

  int tentative;

  ripper * this_ripper;

  list<int> possible_matches;

  bool new_progress_check;
  int max_for_this;

  double start_time = get_time(), last_time = start_time;
  const int stepsize = (1<<14) - 1;

  // Set precision so we don't have to fake it with round anymore.
  // Also disable stdio synchronization as we aren't going to use stdio anyhow
  // and that makes it slightly faster.
  cout.precision(3);
  cout.sync_with_stdio(false);

  double unit = 1/(double)s.size();
  
  for (current = s.begin(); current != s.end(); current++) {

	  if ((count & stepsize) == stepsize) {
		  new_progress_check = true;
		  double fraction_done = count * unit;
		  double now = get_time();

		  // Only show the progress indicator if there's been enough
		  // time since last we did it. 

		  if (now - last_time > 0.15) {
			    cout << prefix_out << "\tProgress: " << count 
				    << " of " << s.size() << "\t" <<
				    fraction_done * 100.0 << " % done  \t";
			     cout << eta(start_time, now, fraction_done);
			     cout << " left    \r" << flush;

			     last_time = now;
		  }
	  }

	  // First check if there are any matches for the current position.
	  tentative = searcher.found(*current);
	  if (tentative != -1) possible_matches.push_back(tentative);
	  tentative = ci_searcher.found(*current);
	  if (tentative != -1) possible_matches.push_back(tentative);
	  tentative = two_byte.found(*current);
	  if (tentative != -1) possible_matches.push_back(tentative);

	  list<int>::iterator pos;
	  
	  // pos now contains a list of the RK matches we found. For each of
	  // those, investigate if they're real or false positives by calling
	  // the ripper's "is this a header" function itself.
	  // If we find something, tell the ripper to extract it to a file.

	  for (pos = possible_matches.begin(); pos != possible_matches.end();
			  pos++) {
		  
		tentative = *pos;
//		cout << "Something may have matched " << tentative << " at " 
//			<< current - s.begin() << endl;
		this_ripper = rippers[tentative];
		pos = possible_matches.erase(pos);
		bool found_header = false;
		int backtrace, confirmed;

		int rel_pos = find_header_offset(*this_ripper, count, 
				clue_location[tentative], 
				next_valid_position[tentative],	current, 
				s.end());

		// If we didn't find anything, then be on our merry way
		if (rel_pos == -1) continue;

		// Else we dump. First figure out what is the maximum length
		// we should accept. If the ripper knows anything about the
		// realistic maximum length for the format, well, take the
		// lesser of that and the user specified maximum. Otherwise,
		// trust the user.
		if (this_ripper->suggest_max_length() != -1 && 
				!override_suggestions)
			max_for_this = min(maxlen, this_ripper->
					suggest_max_length());
		else	max_for_this = maxlen;

		// Let the user know we're going to dump something
		// If we've printed a new progress line since the last time,
		// print an endl so as not to obscure it. Otherwise, get right
		// to the information.
		if (new_progress_check) {
			new_progress_check = false;
			cout << endl;
		}
		
		cout << this_ripper->get_extension() << " ripper: dumping a" <<
			" new file. " << flush;
		string file_name = prefix_out + "_out" + itos(files_ripped
				[tentative]++) + this_ripper->get_extension();
		
		int how_much = this_ripper->dump_from_header(current - rel_pos,
				s.end(), max_for_this, file_name);
		
		// Now tell the user and set the correct next_valid_position
		// if the ripper tells us that we shouldn't revisit the area
		// more than once.
		cout << "\t" << how_much << " bytes ripped. ";

		int bytes_ignore = this_ripper->should_skip();
		if (bytes_ignore > 0) {
			next_valid_position[tentative] = count + bytes_ignore;
			cout << "Ignoring " << bytes_ignore << " bytes for " <<
				"this ripper.";
		}
		
		// If this is the ZZT ripper, tell the BRD ripper to avoid this
		// area since it's going to be filled with boards that we don't
		// need to rip twice. Since the BRD format is just a board
		// dump (with no headers), the BRD ripper would otherwise
		// trigger falsely.
		// This is kind of a kludge since I really should have a "real"
		// inter-ripper communication system, but I found that to be
		// too much overkill.
	
		if (this_ripper->get_extension() == ".zzt" && board_skip)
			if (next_valid_position[zzt_board_ripper] < count + 
					how_much) {
				cout << "Telling board ripper to skip." << endl;
				next_valid_position[zzt_board_ripper] = count +
					how_much;
			}

		// Done!
		cout << endl;
	  }

	  count++;
  }

  double now = get_time();
  cout << endl;
  cout << "Checked file " << prefix_out << " in " << hhmmss((int)round(now-
			  start_time)) << " for " << (count/(1<<20)) / 
	  (now-start_time) << " Mbytes/s" << endl;
  cout << endl;

  // At the end of it all, list a summary of which files we got and how many
  // of each.
  
  cout << " SUMMARY: " << flush;

  for (counter = 0; counter < rippers.size(); counter++) {
	  if (counter % 4 == 0) cout << endl;
	  cout << rippers[counter]->get_extension() << ": " <<  
		  files_ripped[counter] << "\t\t";
  }
  cout << endl;

}

// This is called if no parameters or files are specified by the user.
void print_user_info(char * self, vector<ripper *> rippers) {

	cout << "Usage: " << self << " [OPTION]... [FILE]..." << endl;
	cout << "Extracts various formats from disk image FILEs." << endl;
	cout << endl;
	cout << "Options:" << endl;
	cout << "  -B         Enable board skip, making the zzt board (.brd) "
		<< "ripper skip boards" << endl;
	cout << "             within ZZT files that have been ripped by the"
		<< " .zzt ripper." << endl;
	cout << "             (Default)" << endl;
	cout << "  -b         Disable board skip. " << endl;
	cout << "  -n ARG     Disable the ripper that handles files with " << 
		" extension equal to" << endl;
	cout << "             \".ARG\". For example, -n png will disable" 
		<< " the ripper that outputs" << endl;
	cout << "             .png files." << endl;
	cout << "  -m len     Set maximum output file length to \"len\" bytes." << endl;
	cout << "             (Default is 16777216 bytes.)" << endl;
	cout << "  -o         Override ripper-specific limits on maximum file length." << endl;
	cout << "  -Z         Enable rigorous ZZT board (.brd) validity test,"
		<< " excluding more" << endl;
	cout << "             false positives but potentially also partly "
		<< "corrupted .brd" << endl;
	cout << "             files. (Default)" << endl;
	cout << "  -z         Disable rigorous ZZT board validity test." <<
		endl;
					
	cout << endl;

	cout << "The following rippers are supported in this version: ";

	for (int counter = 0; counter < rippers.size(); counter++) {
		if (counter % 7 == 0) cout << endl;
		cout << rippers[counter]->get_extension() << "\t";
	}
	cout << endl;
	
}

bool get_parameters(int argc, char ** argv, vector<ripper *> & rippers,
		vector<bool> & ripper_disabled, bool & zzt_rigor, bool &
		board_skip, bool & override_suggestions, int & maxlen, 
		vector<string> & filenames) {
	// Parameters are either "-n [ext]" (such as -n zzt), which state
	// that the ripper in question should be disabled, -B/-b which state
	// that the ZZT board ripper should be rigorous or not, -Z/-z which
	// state whether the ZZT world ripper should prevent the board ripper
	// from detecting the boards that are part of the world it ripped, or
	// --maxlen (-m), which states the maximum length of any file to be
	// ripped.
	
	// Defaults are: maxlen = 16777216, all rippers enabled, ZZT should
	// be rigorous and exclude the board ripper.
	
	// We don't support GNU style longopts yet because that function is a
	// dog to work with.

	maxlen = 16777216;
	ripper_disabled.resize(rippers.size(), false);
	
	zzt_rigor = true;
	board_skip = true;
	override_suggestions = false;

	int c, index;

	string ext; int ripper_affected;

	while ((c = getopt(argc, argv, "BbZzn:m:")) != -1) {
		cout << optarg << endl;
		switch(c) {
			case 'B': // enable board skip
				board_skip = true;
				break;
			case 'b': // disable board skip
				board_skip = false;
				break;
			case 'Z': // enable rigorous .brd interpretation
				zzt_rigor = true;
				break;
			case 'z': // disable rigorous .brd interpretation
				zzt_rigor = false;
				break;
			case 'n': // disable ripper
				ext = optarg;
				// Handle ambiguity by doing twice, once
				// with . and once without
				ripper_affected = get_index_by_extension(ext, 
						rippers);
				if (ripper_affected != -1)
					ripper_disabled[ripper_affected] = true;
				ext = "." + ext;
				ripper_affected = get_index_by_extension(ext,
						rippers);
				if (ripper_affected != -1)
					ripper_disabled[ripper_affected] = true;
				break;
		 	case 'm': // maxlen
				maxlen = strtoul(optarg, NULL, 10); 
				break;
			case 'o': // override suggestions
				override_suggestions = true;
				break;
			case '?': // unknown option
				if (isprint(optopt))
					cerr << "Unknown option '-" << 
						(char)optopt << "'" << endl;
				else
					cerr << "Unknown option character '" <<
						(int)optopt << "'" << endl;
				break;
			default:
				abort();
		}
	}

	for (int counter = optind; counter < argc; counter++) {
		filenames.push_back(argv[counter]);
	}
}

string get_core_name(string in) {
	// snip off everything prior to the last slash.
	// First find it, then copy.
	
	string::const_iterator pos;

	for (pos = in.end() - 1; pos >= in.begin(); pos--)
		if (*pos == '/' || *pos == '\\')
			break;

	string toRet;
	string::const_iterator posii;

	// Yeah, quadratic complexity. Fix later.

	for (posii = pos+1; posii != in.end(); posii++)
		toRet.push_back(*posii);

	return(toRet);
}
	
int main(int argc, char** argv) {
	vector<ripper *> rippers = get_all_rippers();
	
	/*if (argc != 2) 
		fail("wrong number of arguments");*/

	vector<string> filenames;
	int maxlen;
	bool zzt_rigor, board_skip, override_suggestions;
	vector<bool> disabled_rippers;

	get_parameters(argc, argv, rippers, disabled_rippers, zzt_rigor,
			board_skip, override_suggestions, maxlen, filenames);

	if (filenames.size() < 1) {
		cerr << "You must specify a file name to search!" << endl;
		print_user_info(argv[0], rippers);
	}

	int core_count = 0;

	for (int counter = 0; counter < filenames.size(); counter++) {
		string core = get_core_name(filenames[counter]);
		if (core.empty())
			core = "unk" + itos(core_count++);
		
		rip_single_file(filenames[counter].c_str(), rippers, 
				disabled_rippers, zzt_rigor, board_skip, 
				override_suggestions, maxlen, core);
	}
}
	
