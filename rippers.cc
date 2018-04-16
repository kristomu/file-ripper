// Let's do this properly
//	Functions of the ABC:
//		Public:
//		is_header_here (position)
//		dump_from_header (position, maximum length)
//		suggest_maximum_length // for zzt etc
//			// The clues are ORs of ANDs: each vector index is
//			// a bundle where all must be registered in order
//			// to match.
//		vector<clue_bundle> get_clues(int length_wanted);
//		get_extension // ID of ripper

// I'm going to cut down radically on the clue bundle. It now acts more
// conservatively, only giving a single clue. This clue is selected so that
// all signatures match on that point for bytes_needed onwards. If none match,
// it returns an empty string.
// (YAGNI)

// We should probably rename is_header_here to get_header_here or somesuch (and
// then alias is_header_here to it).

// clue bundle:
// 		string clue;
// 		int offset;
// 		bool case_sensitive;

// For some odd reason, GIF matches too late. See out1.gif from Zipdisks.
//	That's because the footer pattern is imperfect. It should be just 
//	"00 3b", but that matches too early because I don't take size into
//	account.

// TODO: Add a class for the rough rippers - things like S3M where we just
// dump max_length no matter what. Just use size_ripper with get_integer
// overloaded to return MAXINT.

#include <vector>
#include <ext/rope>
#include <fstream>
#include <iostream>
namespace Sgi = ::__gnu_cxx;       // GCC 3.1 and later
using namespace std;

#define char_rope Sgi::rope<char>

struct clue_bundle {
	vector<string> clues;
	int offset;
	bool case_sensitive;
};

// except for some aliasing, this has no code; it's just an abstraction

class ripper {
	protected:
		unsigned int count; // how many bytes did we rip?
	public:
		virtual int get_header_here(char_rope::const_iterator position,
				char_rope::const_iterator end) = 0;
		bool is_header_here(char_rope::const_iterator position,
				char_rope::const_iterator end);
		virtual int dump_from_header(char_rope::const_iterator 
				position, char_rope::const_iterator end,
				int max_length, ostream & output) = 0;
		int dump_from_header(char_rope::const_iterator position,
				char_rope::const_iterator end, int max_length,
				string file_name);
		// used by zip for avoiding false positives, and may be used
		// by plain-text programming rippers that match on any
		// standard function (e.g PRINT for BASIC).
		// We now extend it to an int so that partial ZIP files can
		// have at least the headers we find skipped. (e.g 500 PK 03 04
		// followed by lot of noise followed by one PK 05 06; skipping
		// 499 of those will help a lot.)
		virtual int should_skip() { return(-1); }
		// -1 means no max length suggestion available
		virtual int suggest_max_length() { return(-1); }
		virtual vector<clue_bundle> get_clues(int length_wanted) = 0;
		virtual string get_extension() = 0;
};

int ripper::dump_from_header(char_rope::const_iterator position, char_rope::
		const_iterator end, int max_length, string file_name) {

	ofstream f;
	f.open(file_name.c_str());

	if (!f) return(false);

	return(dump_from_header(position, end, max_length, f));
}

bool ripper::is_header_here(char_rope::const_iterator position,
		char_rope::const_iterator end) {
	return (get_header_here(position, end) != -1);
}

// ------------------------------------------------------------------------- //

// header/footer ripper

class footer_ripper : public ripper {
	protected:
		// returns -1 if no footer, otherwise the one that triggered
		// it.
		virtual int get_footer_here(char_rope::const_iterator position,
				char_rope::const_iterator end) = 0;
		bool is_footer_here(char_rope::const_iterator position,
				char_rope::const_iterator end);
		virtual int footer_offset(char_rope::const_iterator 
				footer_begin, char_rope::const_iterator end) {
			return(0); }
		virtual int get_footer_length(int footer_num) = 0;
	public:
		virtual int get_header_here(char_rope::const_iterator position,
				char_rope::const_iterator end) = 0;
		virtual int dump_from_header(char_rope::const_iterator 
				position, char_rope::const_iterator end, 
				int max_length,	ostream & output);
};

bool footer_ripper::is_footer_here(char_rope::const_iterator position,
		char_rope::const_iterator end) {
	return (get_footer_here(position, end) != -1);
}

int footer_ripper::dump_from_header(char_rope::const_iterator position, 
		char_rope::const_iterator end, int max_length, ostream & 
		output) {

	count = 0;

	if (!is_header_here(position, end)) return(0);

	while (!is_footer_here(position, end) && position != end && 
			count < max_length) {
		output << *position++;
		count++;
	}

	/* For zips etc which have "dangling sections" after the real footer. */
	
	int footer_identified = get_footer_here(position, end);
	int remaining = 0;
	
	if (footer_identified != -1)
		remaining = footer_offset(position, end) + 
			get_footer_length(footer_identified);
	
	for (int counter = 0; counter < remaining && position != end; counter++)
		output << *position++;

	return(count + remaining);
}

// header/footer ripper, type: wildcard ripper. Here the difficulty
// starts. size ripper is derived from this one as well, overriding
// dump_from_header so we don't have to cut and paste get_clues etc

class wildcard_ripper : public footer_ripper {
	private:
		char no_care;
		bool case_sensitive;
	protected:
		vector<bool> common_to_header, common_to_footer;
		vector<string> header_targets;
		vector<string> footer_targets;
		vector<bool> calculate_skip(const vector<string> source);
		int check_matches(const vector<string> dest, const vector<bool>
				skip_field, char_rope::const_iterator position, 
				char_rope::const_iterator end);
		int get_footer_here(char_rope::const_iterator position,
				char_rope::const_iterator end);
		int get_footer_length(int footer_num);
	public:
		int get_header_here(char_rope::const_iterator position,
				char_rope::const_iterator end);
		vector<clue_bundle> get_clues(int length_wanted);
		void set_patterns(char dont_care, vector<string> headers, 
				vector<string> footers, bool case_matters);
		void set_patterns(char dont_care, string header, string footer,
				bool case_matters);
};

int wildcard_ripper::get_footer_length(int footer_num) {
	if (footer_num < 0 || footer_num >= footer_targets.size()) return(0);

	return(footer_targets[footer_num].size());
}

vector<bool> wildcard_ripper::calculate_skip(const vector<string> source) {

	// TODO: Make this a skip field. [DONE]

	// This calculates which positions are common to all the signatures
	
	vector<bool> toRet(0);
	
	if (source.size() == 1) {
		toRet.resize(source[0].size(), true);
		return(toRet);
	}
	
	// First find the signature of least length
	int counter, smallest = source[0].size(), sec;

	for (counter = 1; counter < source.size(); counter++)
		if (source[counter].size() < smallest) smallest = 
			source[counter].size();

	// Then fill the field
	toRet.resize(smallest, true);
	for (counter = 0; counter < smallest; counter++)
		for (sec = 0; sec < source.size()-1 && toRet[counter]; sec++)
			toRet[counter] = (source[sec][counter] == 
					source[sec+1][counter]);

	return(toRet);
}

int wildcard_ripper::check_matches(const vector<string> dest, const 
		vector<bool> skip_field, char_rope::const_iterator position, 
		char_rope::const_iterator end) {

	// Go through the common bytes first. If any fails, then the pattern
	// fails all, and thus we can return false off the bat.
	// Returns which header we matched (that is, index to header_targets).
	
	int counter;

	char_rope::const_iterator beginning = position;

	for (counter = 0; counter < skip_field.size() && position < end; 
			counter++){
		// ignore it if the position in question is not in the skip
		// field, or the current character is no_care.
	
		if (!skip_field[counter] || dest[0][counter] == no_care) {
			position++;
			continue;
		}

		if (!case_sensitive) {
			if (tolower(*position) != tolower(dest[0][counter]))
				return(-1);
		} else
			if (*position != dest[0][counter]) return(-1);

		position++;
	}

	// now check each in order. If we match one (OR), we're done
	
	for (counter = 0; counter < dest.size() && position < end; counter++) {
		bool match = true;
		position = beginning - 1;
		if (position + dest[counter].size() >= end) continue;

		for (int sec = 0; sec < dest[counter].size() && match; sec++) {
			position++;
			
			// have we already checked this point? If so, cont.
			if (sec < skip_field.size())
				if (skip_field[sec]) continue;
			if (dest[counter][sec] == no_care) continue;

			if (!case_sensitive) {
				if (tolower(*position) != tolower(
							dest[counter][sec]))
					match = false;
			} else
				if (*position != dest[counter][sec])
					match = false;
		}

		if (match)	return(counter);
	}

	return(-1);
}
			
int wildcard_ripper::get_footer_here(char_rope::const_iterator position,
		char_rope::const_iterator end) {
	return(check_matches(footer_targets, common_to_footer, position,
				end));
}

int wildcard_ripper::get_header_here(char_rope::const_iterator position,
		char_rope::const_iterator end) {
	return(check_matches(header_targets, common_to_header, position,
				end));
}

// Get clues
// for x = 0 to size(smallest) - length wanted inclusive
// Create new clue bundle
// 	for y = 0 to length_wanted
//	 	If thre is a ? at any of the inputs at this point, abort
// 	If we get through that, add all header substrings at that point with
//	length "length_wanted".
//	Sort and run uniq to remove duplicates
// Add clue bundle to vector.
// Next x

// This doesn't handle the worst case (differing offsets), but it's as close
// as we can get without ugly recursion. (Notably, this doesn't handle the
// MZX ripper because searchers three and four for nonprotected MZXes don't
// line up.)

vector<clue_bundle> wildcard_ripper::get_clues(int length_wanted) {
	
	vector<clue_bundle> toRet;
	clue_bundle prototype;
	prototype.case_sensitive = case_sensitive;
	int counter, sec, tri, smallest = common_to_header.size();

	for (counter = 0; counter <= smallest - length_wanted; counter++) {
		
		// check that there isn't a wildcard in the space we want -
		// on any of the header strings
		bool potential = true;
		for (sec = 0; sec < length_wanted && potential; sec++) 
			for (tri = 0; tri < header_targets.size() && potential;
					tri++) 
				potential = (header_targets[tri][counter+sec]
						!= no_care);

		if (!potential) continue;

		prototype.clues.resize(0);

		// now start adding, boys! (Miggs, lackey of Lynch)
		// substring operators really help
			
		prototype.offset = counter;
			
		for (tri = 0; tri < header_targets.size(); tri++)
			prototype.clues.push_back(header_targets[tri].
					substr(counter, length_wanted));

		// remove duplicates - we can do this since the
		// function is called only once, thus the n log n
		// doesn't impact on us that much.

		sort(prototype.clues.begin(), prototype.clues.end());
		vector<string>::iterator i = unique(prototype.clues.begin(), 
				prototype.clues.end());
		prototype.clues.erase(i, prototype.clues.end());
			
		toRet.push_back(prototype);
	}

	return(toRet);
}

/*vector<clue_bundle> wildcard_ripper::get_clues(int length_wanted) {
	// for x = 0 to size(smallest) - length_wanted inclusive
	// if all are equal from this point and there are no wildcards,
	// 	create clue_bundle and add.
	
	// then return
	
	vector<clue_bundle> toRet;
	
	// First, find smallest. (That's a nice trick)
	int counter, smallest = common_to_header.size();
	
	// then for each position
	for (counter = 0; counter <= smallest - length_wanted; counter++) {
		bool potential = true;
		// check for equality (by way of the common header)
		for (int sec = 0; sec < length_wanted && potential; 
				sec++)
			
			if (common_to_header[counter+sec]) {
				// and if it passes, check for lack of
				// wildcards on all
				for (int tri = 0; tri < header_targets.size()
						&& potential; tri++) 
					potential = (header_targets[tri]
							[counter+sec] != 
							no_care);
			} else	potential = false;

		// if we found something, add it to our cache
		if (potential) {
			clue_bundle x;
			x.clue = header_targets[0].substr(counter,
					length_wanted);
			x.offset = counter;
			x.case_sensitive = case_sensitive;
			toRet.push_back(x);
		}
	}

	return(toRet);
}*/

void wildcard_ripper::set_patterns(char dont_care, vector<string> headers,
		vector<string> footers, bool case_matters) {
	case_sensitive = case_matters;
	header_targets = headers;
	footer_targets = footers;
	no_care = dont_care;

	common_to_header = calculate_skip(header_targets);
	common_to_footer = calculate_skip(footer_targets);
};

void wildcard_ripper::set_patterns(char dont_care, string header, 
		string footer, bool case_matters) {
	vector<string> hdr_indirect(1, header);
	vector<string> ftr_indirect(1, footer);

	set_patterns(dont_care, hdr_indirect, ftr_indirect, case_matters);
}

// Text ripper: Wildcard ripper with a check for 16 nonprintables as footer.
// This is used for plain-text formats like HTML, BAT, and uncompressed BAS.

class text_ripper : public wildcard_ripper {
	private:
		int get_footer_here(char_rope::const_iterator pos, 
				char_rope::const_iterator end);
};

int text_ripper::get_footer_here(char_rope::const_iterator pos, 
		char_rope::const_iterator end) {
	if (pos == end) return(0);
	int q = check_matches(footer_targets, common_to_footer, pos,
				end);

	if (q != -1) return(q);

	// now check for a string of nonprintables. Since the length is greater
	// than 2, we should clear CRLF, but for extra points, we disregard
	// those.
	
	bool only_crlf = true;
	for (int counter = 0; counter < 16 && pos != end; counter++) {
		if (isprint(*pos)) return(-1);
		if (*pos != 10 && *pos != 13) only_crlf = false;
		pos++;
	}

	if (only_crlf) return(-1);
	return(0);
}


// ------------------------------------------------------------------------- //

// Size ripper - determines length of file based on a size field in the file
// itself. For code reuse purposes, we derive this off wildcard_ripper and
// override some virtual functions. Yes, it's ugly, but cut and paste is
// worse.

// TODO vectors of size locations for MSP and PCX? 

class size_ripper : public wildcard_ripper {
	private:
		int size_location;
		bool msb;
		int size_len;
		int multiplier; // to multiply size field value by once done
		int offset; // to add afterwards
	protected:
		virtual int get_integer(char_rope::const_iterator pos);
	public:
		int dump_from_header(char_rope::const_iterator position, 
				char_rope::const_iterator end, int max_length, 
				ostream & output);
		void set_size_pattern(char dont_care, vector<string> headers,
				bool case_matters, int size_loc, int s_offset,
				int size_length, int s_factor, bool is_msb);
		void set_size_pattern(char dont_care, string header, bool 
				case_matters, int size_loc, int s_offset, 
				int size_length, int s_factor, bool is_msb);
};

int size_ripper::get_integer(char_rope::const_iterator pos) {
	
	unsigned int toRet = 0, shift = 0;

	if (msb) {
		shift = size_len * 8;
		for (int counter = size_len - 1; counter >= 0; counter--) {
			unsigned int r = (unsigned char)*(pos++);
			shift -= 8;
			toRet += r << shift;
		}
	} else {
		for (int counter = 0; counter < size_len; counter++) {
			unsigned int r = (unsigned char)*(pos++);
			toRet += r << shift;
			shift += 8;
		}
	}

	return(toRet * multiplier);
}

int size_ripper::dump_from_header(char_rope::const_iterator position, 
		char_rope::const_iterator end, int max_length, 
		ostream & output) {
	count = 0;
	
	if (!is_header_here(position, end)) return(0);

	int span = get_integer(position + size_location);
	if (span < 0) span = max_length - offset;
	if (span + offset > max_length) span = max_length - offset;

	char_rope::const_iterator proj_end = position + span + offset;

	if (proj_end > end) proj_end = end;

	while (position != proj_end)
		output << *position++;

	count = span + offset;
		 
	return(span + offset);
}

void size_ripper::set_size_pattern(char dont_care, vector<string> headers, 
		bool case_matters, int size_loc, int s_offset, int size_length,
		int s_factor, bool is_msb){

	size_location = size_loc;
	msb = is_msb;
	size_len = size_length;
	offset = s_offset;
	multiplier = s_factor;

	vector<string> footers(1, ""); // not used

	set_patterns(dont_care, headers, footers, case_matters);
}

void size_ripper::set_size_pattern(char dont_care, string header, bool 
		case_matters, int size_loc, int s_offset, int size_length,
		int s_factor, bool is_msb) {
	vector<string> headers;
	headers.push_back(header);

	set_size_pattern(dont_care, headers, case_matters, size_loc, s_offset,
			size_length, s_factor, is_msb);
}

// --

// These always dump max_length. They are designed for hideously complex
// formats. Note that my abuse of inheritance is starting to show - there
// are at least two "specify data" commands by now, and neither completely
// encapsulates the class.

class dumb_ripper : public size_ripper {

	private:
		int get_integer(char_rope::const_iterator pos);
	public:
		void set_patterns(char dont_care, vector<string> headers,
				bool case_matters);
		void set_patterns(char dont_care, string header, 
				bool case_matters);
};

int dumb_ripper::get_integer(char_rope::const_iterator pos) {
	return(2147483647);
}

void dumb_ripper::set_patterns(char dont_care, vector<string> headers,
		bool case_matters) {

	set_size_pattern(dont_care, headers, case_matters, 0, 0, 0, 0, true);
}

void dumb_ripper::set_patterns(char dont_care, string header, bool 
		case_matters) {

	set_size_pattern(dont_care, header, case_matters, 0, 0, 0, 0, true);
}

/*

// Simple size ripper: LBM. Should go somewhere else.

class lbm_ripper : public size_ripper {
	public:
		lbm_ripper() { set_size_pattern('?', "FORM????ILBMBMHD",
				true, 4, 8, 4, 1, true); }
		string get_extension() { return(".lbm"); }
};

// Simple wildcard ripper: GIF. Should go somewhere else.

class gif_ripper : public wildcard_ripper {
	public:
		gif_ripper();
		string get_extension() { return(".gif"); }
};

gif_ripper::gif_ripper() {
	// create vector of possible headers
	vector<string> headers, footers;
	headers.push_back("GIF89a");
	headers.push_back("GIF87a");
	string foot(3, 0);
	foot[2] = 0x3B;
	footers.push_back(foot);
	set_patterns('?', headers, footers, true);
}
*/
