#include "rippers.cc"
#include <stdio.h>

// Implemented rippers.

// -- //

// Size rippers: LBM, ANM, PBM, HLP, WAV, WRI, EXE, FLI, MSP [50%]
// To add:

// IFF rippers are easy - FORM then size field (MSB) then the ID
class iff_ripper : public size_ripper {
	private:
		string ext;
		int max_len;
	public:
		iff_ripper(string ID, string extension, int maxlen);
		string get_extension() { return (ext); }
		int suggest_max_length() { return(max_len); }
};

iff_ripper::iff_ripper(string ID, string extension, int maxlen) {
	ext = extension;
	max_len = maxlen;
	set_size_pattern('?', "FORM????" + ID, true, 4, 8, 4, 1, true);
};

class lbm_ripper : public iff_ripper {
	public:
	lbm_ripper() : iff_ripper("ILBMBMHD", ".lbm", 76800) {} 
		// 320x240 < 76800
};

class anm_ripper : public iff_ripper {
	public:
		anm_ripper() : iff_ripper("ANIMFORM", ".anm", -1) {}
};

class pbm_ripper : public iff_ripper {
	public:
		pbm_ripper() : iff_ripper("PBM ", ".pbm", 76800) {}
};

/*class lbm_ripper : public size_ripper {	// DeluxePaint pictures
	public:
		lbm_ripper() { set_size_pattern('?', "FORM????ILBMBMHD",
				true, 4, 8, 4, 1, true); }
		string get_extension() { return(".lbm"); }
		int suggest_max_length() { return(76800); } // 320x240
};

class anm_ripper : public size_ripper {	// DA animations
	public:
		anm_ripper() { set_size_pattern('?' , "FORM????ANIMFORM", true,
				4, 8, 4, 1, true); }
		string get_extension() { return(".anm"); }
};

class pbm_ripper : public size_ripper { // DA brushes or compressed images
	public:
		pbm_ripper() { set_size_pattern('?', "FORM????PBM ", true,
				4, 8, 4, 1, true); }
		string get_extension() { return(".pbm"); }
		int suggest_max_length() { return(76800); }
};*/

// some other IFF formats (they don't cost much) - untested

class svx_ripper : public iff_ripper {
	public:
		svx_ripper() : iff_ripper("8SVX", ".svx", -1) {}
};

class abm_ripper : public iff_ripper {
	public:
		abm_ripper() : iff_ripper("ANBM", ".abm", -1) {}
};

// standarized Interactive Fiction (Z-code) saves
class quetzal_ripper : public iff_ripper {
	public:
		quetzal_ripper() : iff_ripper("IFZSIF", ".quetzal", -1) {}
};


class wav_ripper : public size_ripper { // MS Windows .WAV sound format
	public:
		wav_ripper() { set_size_pattern('?', "RIFF????WAVEfmt", true,
				4, 8, 4, 1, false); }
		string get_extension() { return(".wav"); }
};

class cdr_ripper : public size_ripper { // Corel draw .CDR format
	public:
		cdr_ripper() { set_size_pattern('?', "RIFF????CDR?vrsn",
				true, 4, 8, 4, 1, false); }
		string get_extension() { return(".cdr"); }
};

class hlp_ripper : public size_ripper { // MS Windows help file
	public:
		hlp_ripper();
		string get_extension() { return(".hlp"); }
		int suggest_max_length() { return(16777216); } //rounded up
};

hlp_ripper::hlp_ripper() {
	// ? _ 0x3 0x0, four unknowns, then FF FF FF FF, then the size field.
	
	string hdr(12, 0xFF);
	hdr[0] = '?';	hdr[1] = '_';	hdr[2] = 3;	hdr[4] = 'X';
	hdr[5] = 'X';	hdr[6] = 'X';	hdr[7] = 'X';

	set_size_pattern('X', hdr, true, 12, 0, 4, 1, false);
};

class wri_ripper : public size_ripper { // MS Windows 3.x Write documents
	// 0x31BE (or 32BE) followed by 3 zeroes then 0xAB. Size field
	// should be multiplied by 128.
	public:
		wri_ripper();
		string get_extension() { return(".wri"); }
		int suggest_max_length() { return(524288); }
};

wri_ripper::wri_ripper() {
	string first_hdr(6, 0);
	first_hdr[0] = 0x31;
	first_hdr[1] = 0xBE;
	first_hdr[5] = 0xAB;
	string sec_hdr = first_hdr;
	sec_hdr[0] = 0x32;

	vector<string> headers;
	headers.push_back(first_hdr);
	headers.push_back(sec_hdr);

	set_size_pattern('?', headers, true, 0x60, 0, 4, 128, false);
};

// can't be used yet (because of clue problems)  (Now works.)
// Magic is either 0x44616ED9 (v1) or 0x4C696E53 (v2).
// Then two LSB u_shorts follow, width and height (in that order). Size is
// then equal to (Width / 8) * Height + 32, though that may overestimate for
// v2 files (which are RLE encoded). Note the /8 - that is because MSP is 2-
// color and thus 1 pixel = 1 bit.
class msp_ripper : public size_ripper {
	private:
		int get_integer(char_rope::const_iterator pos);
		string get_extension() { return(".msp"); }
		// very generous: 2 * (640x480 / 8)
		int suggest_max_length() { return(65536); }
	public:
		msp_ripper();
};

int msp_ripper::get_integer(char_rope::const_iterator pos) {
	// assumes we're at width.
	
	int width = (u_char)*(pos) + ((u_char)*(pos+1) << 8);
	int height = (u_char)*(pos+2) + ((u_char)*(pos+3) << 8);

	return ((width >> 3) * height + 32);
};

// We can now have multiple headers!
msp_ripper::msp_ripper() {
	string first = "xxxx"; vector<string> headers;
	first[0] =  0x44;	first[1] = 0x61;	first[2] = 0x6E;
	first[3] =  0xD9;

	headers.push_back(first); // v1
	first[0] = 0x4C;	first[1] = 0x69;	first[2] = 0x6E;
	first[3] = 0x53;
	headers.push_back(first);
	
	set_size_pattern('?', headers, true, 4, 0, 4, 1, false);
}

// ?? ?? ?? ?? 11 AF xx xx : ?? is the size field, x is the number of frames
// all LSB.
class fli_ripper : public size_ripper {
	private:
		int get_header_here(char_rope::const_iterator pos,
				char_rope::const_iterator end);
	public:
		fli_ripper();
		string get_extension() { return(".fli"); }
};

int fli_ripper::get_header_here(char_rope::const_iterator pos, 
		char_rope::const_iterator end) {
	// first check against the magic
	int p = check_matches(header_targets, common_to_header, pos,
			end);

	if (p == -1) return(-1); // thumbs down

	// Now get the number of frames
	// This is from the POV of the size field, so we must add in its
	// 4 bytes and the 2 bytes of the title.
	int numframes = (u_char)*(pos+6) + ((u_char)*(pos+7) << 8);

	// 0 frames make no sense, and the upper limit as given by Autodesk
	// Anim is 4000.
	if (numframes == 0 || numframes > 4000) return(-1);

	// get width and height, shouldn't be > 320x200
	int width = (u_char)*(pos+8) + ((u_char)*(pos+9) << 8);
	if (width > 320 || width == 0) return(-1);
	int height = (u_char)*(pos+10) + ((u_char)*(pos+11) << 8);
	if (height > 200 || height == 0) return(-1);

	// bpp - this is not a HDRI format
	int bpp = (u_char)*(pos+12) + ((u_char)*(pos+13) << 8);
	if (bpp == 0 || bpp > 8) return(-1);

	return(0);
}

fli_ripper::fli_ripper() {
	string hdr = "????";
	hdr.push_back(0x11);
	hdr.push_back(0xAF);

	set_size_pattern('?', hdr, true, 0, 0, 4, 1, false);
};

// MS Windows bitmap. BM then u_long size. We add some header checks to
// keep the false positives from heaping up.

class bmp_ripper : public size_ripper {
	public:
		int get_header_here(char_rope::const_iterator position,
				char_rope::const_iterator end);
		string get_extension() { return(".bmp"); }
		bmp_ripper() { set_size_pattern('?', "BM", true, 2, 0, 4, 1,
				false); }
};

int bmp_ripper::get_header_here(char_rope::const_iterator position, char_rope::
		const_iterator end) {

	// Hack. TODO: Fix this. Check matches doesn't work for some odd
	// reason.
	// Now it does [I think]
	/*if (*position != 'B') return(-1);
	if (*(position+1) != 'M') return(-1);*/

	// usual check
	int p = check_matches(header_targets, common_to_header, position,
			end);

	if (p == -1) return(p);
	
	// now check that index 0E is 0x28 and then three times 0.
	// Also check that the file length > 0.
	int len = get_integer(position+2);
	if (len == 0) return(-1);

	char_rope::const_iterator chk = position + 0x0E;

	if (*chk++ != 0x28) return(-1);
	if (*chk++ != 0) return(-1);
	if (*chk++ != 0) return(-1);

	return(0);
}

// MS-DOS executable (EXE) - note, does not work on PE / NE files:
// MZ then two bytes PartPag, then two bytes PagCnt, then size is
// 	PagCnt * 512 - (512 - PartPag).
// To match, PartPag < 512, and if PartPag == 0 then PagCnt > 0.
// Then at index 10, there's a min memory LSB u_short (<= 40960) and max memory
// just after (> 0 and >= min_memory, also <= 40960). Or does 0 mean "take all
// you can get"?

class exe_ripper : public size_ripper {
	private:
		int get_u_short(char_rope::const_iterator position);
		int get_header_here(char_rope::const_iterator position, 
				char_rope::const_iterator end);
		int get_integer(char_rope::const_iterator pos);
		
	public:
		exe_ripper();
		string get_extension() { return(".exe"); }
		int suggest_max_length() { return(2097152); }
};

// gets a LSB u_short size field
int exe_ripper::get_u_short(char_rope::const_iterator position) {
	return ((unsigned char)*position + ( (unsigned char)*(position+1) << 
				8));
}

int exe_ripper::get_header_here(char_rope::const_iterator position, 
		char_rope::const_iterator end) {

	if (*position != 'M') return(-1);
	if (*(position+1) != 'Z') return(-1);
	int partpag = get_u_short(position+2);
	if (partpag > 512) return(-1);
	int pagcnt = get_u_short(position+4);

	//cout << "Partpag: " << partpag << endl;
	//cout << "Pagcnt: " << pagcnt << endl;

	// Since the equation is pagcnt * 512  - (512 - partpag), having
	// partpag != 512 would give negative results.
	// Also assume that partpag is never > 512, because then DOS would
	// have updated pagecount instead (sketchy, but I gotta do something
	// about all these false positives)
	if (pagcnt == 0 && partpag != 512) return(-1);
	if (partpag > 512) return(-1);
	int lin_size = pagcnt * 512 - (512 - partpag);
	if (lin_size < 0) return(-1);

	int min_mem = get_u_short(position + 10);
	if (min_mem >= 40960) return(-1); // 40960 = 640k / 16
	int max_mem = get_u_short(position + 12);
	// allowing max_mem == min_mem == 0 gives too many false positives.
	if (max_mem <= min_mem /* && max_mem != 0*/) return(-1);
	//cout << max_mem << "\t" << min_mem << endl;
	// Assume overlay is < 255, that is that there are a reasonable number
	// of overlays.
	int overlays = get_u_short(position + 0x1A);
	if (overlays > 255) return(-1);
	// Assume relocation offset < length of file and that it also isn't
	// in the header
	int reloc = get_u_short(position + 0x18);

	if (reloc > lin_size) return(-1);
	if (reloc < 0x1C) return(-1);

	return(0);
}

int exe_ripper::get_integer(char_rope::const_iterator pos) {
	// assumes pos is at partpag
	int partpag = get_u_short(pos);
	int pagcnt = get_u_short(pos+2);

	return(pagcnt * 512 - (512 - partpag));
}

exe_ripper::exe_ripper() {
	// most for the sake of the clue generator
	set_size_pattern('?', "MZ", true, 2, 0, 2, 1, false);
}
		   
	
//////////////////////////////////////////////////////////////////////////////

// TODO: For RTF, enforce that the byte after the {\rtf? must be printable.
// That gets rid of EXE files that contain an RTF renderer.

// Wildcard rippers: GIF, PNG, HTML, compressed BAS, RTF, ZIP, BAT (50%), JPG
// 		     ARJ, ANS, ZZT, FRM, uncompressed BAS
// To add:

// MBBS callers' log.
class mbbs_callers_ripper : public text_ripper {
	private:
		int get_footer_length(int footer_num) { return(0); }
	public:
		mbbs_callers_ripper();
		string get_extension() { return("_callers.bbs"); }
		int should_skip() { return(count); } // since there may be
		// multiple quits/restarts in a log
};

mbbs_callers_ripper::mbbs_callers_ripper() {
	// using ? would create unintended trigraphs.
	string header = "xx:xx xx-xx-xx System UP! ";
	string footer(16, 0); // never match
	set_patterns('x', header, footer, true);
};
	

class html_ripper : public text_ripper {
	public:
		html_ripper() { set_patterns('?', "<HTML>", "</HTML>", false);}
		string get_extension() { return(".html"); }
		int suggest_max_length() { return(4194304); }
};

// Visual Basic
class frm_ripper : public text_ripper {
	public:
		frm_ripper();
		string get_extension() { return(".frm"); }
		int suggest_max_length() { return(262144); }
};

frm_ripper::frm_ripper() {
	string init = "VERSION ?.00";
	string next = "xx" + init;
	init.push_back(13); init.push_back(10);
	next[0] = 13; next[1] = 10;
	set_patterns('?', init, next, true);
}

// it's here but you can't use it yet (because of lousy clue generation)
// [Commented out the part that generates additional headers.]
// Now clue generation is a bit better.
class uncompressed_bas_ripper : public text_ripper {
	private:
		// same reason as in BAT ripper
		int get_footer_length(int footer_num) { return(0); }
	public:
		int should_skip() { return(count); } // always skip
		string get_extension() { return(".txt.bas"); }
		uncompressed_bas_ripper();
};

uncompressed_bas_ripper::uncompressed_bas_ripper() {
	string ftr(16, 0); // never match because we don't actually know what
	// the end is like - no fixed footer.
	
	vector<string> headers; // here we go
	headers.push_back("DECLARE SUB ");
	headers.push_back("DECLARE FUNCTION ");
	headers.push_back("DEFDBL ");
	headers.push_back("DEFSNG ");
	headers.push_back("RANDOMIZE TIMER");
	headers.push_back("SCREEN 12");
	headers.push_back("SCREEN 13");
	headers.push_back("DIM ");
	headers.push_back("OPEN \"");
	headers.push_back("PRINT USING #");
	headers.push_back("COMMON SHARED");

	vector<string> footers(1, ftr);

	set_patterns('?', headers, footers, true);
}

class compressed_bas_ripper : public wildcard_ripper {
	public:
		compressed_bas_ripper();
		string get_extension() { return(".bas"); }
		int suggest_max_length() { return(262144); }
};

compressed_bas_ripper::compressed_bas_ripper() {
	char hdr[17] = {0xFC, 0, 1, 0, 0x0C, 0, 0x81, 1, 0x82, 1, 6, 0,
		1, 2, 3, 4};

	string hdr_string;
	for (int counter = 0; counter < 16; counter++)
		hdr_string.push_back(hdr[counter]);

        // 6 FFs then >0 then 3x 00 then ? then 00 then 02
        // We use wildcards for the >0 part.
        string footer(14, 0xFF);
        footer[6] = '?';
        footer[7] = 0;
        footer[8] = 0;
        footer[9] = 0;
        footer[10] = '?';
        footer[11] = 0;
        footer[12] = 2;
	footer[13] = '?';
        set_patterns('?', hdr_string, footer, true);
}

class png_ripper : public wildcard_ripper {
	public:
		png_ripper();
		string get_extension() { return(".png"); }
};

png_ripper::png_ripper() {
	string header = "xPNGxxx";
	header[0] = 0x89;
	header[4] = 0x0D;
	header[5] = 0x0A;
	header[6] = 0x1A;
	set_patterns('?', header, "IEND????", true);
}

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
	// really just 00 3B but how are we going to match that without
	// false positives?
	string foot(3, 0);
	foot[2] = 0x3B;
	footers.push_back(foot);
	set_patterns('?', headers, footers, true);
}

// Trying ANSi - 0x1B 0x5Bh, stop when < 0x20 and not 0x1B, 13, or 10, or
// smileys (1 or 2).
// We also check that ESC is suffixed with [, as it should be.
// Obviously you'll want to skip the area once done.
class ans_ripper : public wildcard_ripper {
	private:
		int get_header_here(char_rope::const_iterator pos,
				char_rope::const_iterator end);
		int get_footer_here(char_rope::const_iterator pos,
				char_rope::const_iterator end);
	public:
		int should_skip() { return(count); }
		ans_ripper();
		string get_extension() { return(".ans"); }
};

int ans_ripper::get_header_here(char_rope::const_iterator pos,
		char_rope::const_iterator end) {
	// If the first is not ESC (0x1B), forget it
	// Otherwise, if the three next are not printable, forget it
	// Otherwise, if none of index 2, 3 are numbers, forget it
	// Otherwise, if it ends in less than 20 bytes, forget it (removes
	// false positives)
	// Otherwise, OK!
	
	if (*pos != 0x1B) return(-1);
	if (isspace(*pos)) return(-1);
	if (*(pos+1) != '[') return(-1);
	if (!isprint(*(pos+1)) || !isprint(*(pos+2)) || !isprint(*(pos+3)))
		return(-1);
	if (!isdigit(*(pos+2)) && !isdigit(*(pos+3))) return(-1);
	for (int counter = 1; counter < 20; counter++) {
		if (pos + counter == end) return(-1);
		if (is_footer_here(pos+counter, end)) return(-1);
	}

	return(0);
};

int ans_ripper::get_footer_here(char_rope::const_iterator pos, 
		char_rope::const_iterator end) {

	// If it is 01 or 02 (smileys) or > 0x20, OK (not footer)
	// If it is 13 and +1 is 10, OK (CRLF); if it is 10, also OK (LF part)
	// If it is 0x1B (ESC) and +1 is 0x5B (opening bracket), OK
	// else not OK.
	
	unsigned char p = *pos;

	if (p == 1 || p == 2 || p >= 0x20 || p == 10) return(-1);
	if (p == 13 && *(pos+1) == 10) return(-1);
	if (p == 0x1B && *(pos+1) == '[') return(-1);
	return(0);
};

ans_ripper::ans_ripper() {
	string hdr(2, '[');
	hdr[0] = 0x1B;
	set_patterns('?', hdr, "", true);
}

// now checks that the first byte after the "header" is printable or 13 (CR).
class rtf_ripper : public text_ripper {
	private:
		int get_header_here(char_rope::const_iterator pos,
				char_rope::const_iterator end);
	public:
		rtf_ripper();
		string get_extension() { return(".rtf"); }
};

int rtf_ripper::get_header_here(char_rope::const_iterator position,
		char_rope::const_iterator end) {
	int first_stage = check_matches(header_targets, common_to_header,
			position, end);

	if (first_stage == -1) return(-1);

	char after = *(position+header_targets[first_stage].size()+1);

	if (isprint(after) || after == 13 || after == 10) return(first_stage);
	else	return(-1);
}

rtf_ripper::rtf_ripper() {
	// begins with {\rtf and ends in \par[CRLF]} or [LF]\par }
	vector<string> headers, footers;
	headers.push_back("{\\rtf");
	footers.push_back("\\par??}");
	footers.push_back("?\\par }");
	set_patterns('?', headers, footers, false);
}

class bat_ripper : public wildcard_ripper {
	private:
		int get_footer_here(char_rope::const_iterator position,
				char_rope::const_iterator end);
		// the nonalphanumerics aren't really part of the file
		int get_footer_length(int footer_num) { return(0); }
	public:
		bat_ripper();
		string get_extension() { return(".bat"); }
		int suggest_max_length() { return(16384); }
};

// A BAT file's "footer" is three nonprintable bytes. (Two would match CRLF)
// It's a bit of a hack, but it works.

int bat_ripper::get_footer_here(char_rope::const_iterator position,
		char_rope::const_iterator end) {
	if (!isprint(*position) && !isprint(*(position+1)) && !isprint(*
				(position+2)))
		return(0);
	else	return(-1);
}

bat_ripper::bat_ripper() {
	vector<string> headers;
	// we use the ? for alignment purposes. This will go away if I
	// implement a real clue-finding system.
	headers.push_back("?@echo off");
	headers.push_back("@ echo off");
	headers.push_back("@set ");
	headers.push_back("@rem "); 
	vector<string> footers(1, "");

	set_patterns('?', headers, footers, false);
};

// ARJ: 0x60 0xEA, then the sixth byte is >= 2 and <= 15, and eighth byte is
// < 9. Ends in 60 EA 00.
class arj_ripper : public wildcard_ripper {
	private:
		int get_header_here(char_rope::const_iterator 
				position, char_rope::const_iterator end);
	public:
		arj_ripper();
		string get_extension() { return(".arj"); }
};

int arj_ripper::get_header_here(char_rope::const_iterator pos,
		char_rope::const_iterator end) {

	char_rope::const_iterator begin = pos;

	if ((u_char)(*pos++) != 0x60) return(-1);
	if ((u_char)(*pos++) != 0xEA) return(-1);

	// the next two constitute the header size field. They must be
	// greater than 0 and less than 2600 (decimal). LSB.
	int hszfield = (u_char)(*pos++) + ( (u_char)(*pos++) << 8);
	if (hszfield == 0 || hszfield > 2600) return(-1);

	// next byte x, 2 <= x <= 15.
	pos++;
	if (*pos < 2 || *pos > 15) return(-1); // archiver used
	pos++;
	if (*pos++ > 15) return(-1); // min. archiver needed to extr.
	if (*pos++ > 9) return(-1); // host OS
	pos++;
	//if (*pos++ == 0) return(-1); // arj flags - too defensive.
	pos++; // security flag
	// uncomment if you want ZIP-type file fragments.
	// That may catch what would otherwise be lost, but as it is, it gives
	// too many false positives.
	// A ZIP-type state would be really nice here: if we've visited the
	// area before, only extract *pos == 2 files, otherwise extract both.
	if (*pos != 2 /* || *pos != 0 */) return(-1); // file type, must be 2

	/* Checking
	for (pos = begin; pos != begin + 0x10; pos++) {
		printf("%.2x ", (unsigned char)*pos);
	}
	printf ("\n");*/

	return(0);
}

arj_ripper::arj_ripper() {
	string header = "XX";
	string footer = "XXXX"; // 60 EA 00 00
	header[0] = 0x60;
	header[1] = 0xEA;
	footer[0] = 0x60;
	footer[1] = 0xEA;
	footer[2] = 0;
	footer[3] = 0;
	set_patterns('?', header, footer, true);
};

// JPEG: FF D8 FF E0, two unknowns, then JFIF or Exif.
// The file ends in FF D9, but the last FF xx sequence before it must
// have been FF DA (handles Photoshop pictures that have more than one
// FF D9). Thus it is placed here, as it is a bit more advanced than a
// simple wildcard searcher.

class jpg_ripper : public wildcard_ripper {
	private:
		bool ff_seen;
		bool last_da;
	public:
		jpg_ripper();
		string get_extension() { return(".jpg"); }
		int dump_from_header(char_rope::const_iterator position,
				char_rope::const_iterator end, int max_length,
				ostream & output);
};

jpg_ripper::jpg_ripper() {
	// set header and footer
	string first_hdr = "??????JFIF";
	first_hdr[0] = 0xFF;
	first_hdr[1] = 0xD8;
	first_hdr[2] = 0xFF;
	first_hdr[3] = 0xE0;
	string sec_hdr = first_hdr;
	sec_hdr[6] = 'E';
	sec_hdr[7] = 'x';
	sec_hdr[8] = 'i';
	sec_hdr[9] = 'f';
	
	vector<string> headers;
	headers.push_back(first_hdr);
	headers.push_back(sec_hdr);

	vector<string> footers(1);
	footers[0].resize(2);
	footers[0][0] = 0xFF;
	footers[0][1] = 0xD9;

	set_patterns('?', headers, footers, true);
};

int jpg_ripper::dump_from_header(char_rope::const_iterator position, 
		char_rope::const_iterator end, int max_length, ostream & 
		output) {

	if (!is_header_here(position, end)) return(0);

	ff_seen = true;
	last_da = false;

	bool done = false;

	count = 0;

	while (count < max_length && position != end && !done) {
		// Have we found a legitimate footer, and was the last FF
		// sequence FF DA?
		if (last_da)
			if (is_footer_here(position, end))
				done = true; // yes, we're done

		if (done) continue;

		// else, check for FF; if we had an FF at the previous
		// byte, check for DA.
		if (!ff_seen) {
			if ((u_char)*position == 0xFF) ff_seen = true;
		} else {
			if ((u_char)*position == 0xDA) last_da = true;
			ff_seen = false;
		}

		output << *position++;
		count++;
	}

        int remaining = 0;
	int footer_identified = get_footer_here(position, end);

        if (footer_identified != -1)
                remaining = footer_offset(position, end) +
                        get_footer_length(footer_identified);

        for (int counter = 0; counter < remaining ; counter++)
                output << *position++;

	count += remaining;

	return(count);
}

// MBBS "USERS.DAT" file. We match the postal code in order to find out
// that there may be an USERS.DAT here, and then we go backwards to find
// the first entry. Finally, we go forwards to the last, and dump all.

class mbbs_users_ripper : public wildcard_ripper {
	private:
		// is_userdat_esque - called by get_header and
		// dump_from_header in different ways
		// used to check if we have the ubiquitous "length strings"
		// of MBBS
		bool is_length_string(char_rope::const_iterator pos, 
				char_rope::const_iterator end, bool
				accept_zero, int limit_above, bool 
				only_numbers);
		bool is_userdat_struct(char_rope::const_iterator pos,
				char_rope::const_iterator end);
		int get_header_here(char_rope::const_iterator pos,
				char_rope::const_iterator end);
		int dump_from_header(char_rope::const_iterator pos,
				char_rope::const_iterator end, int max_length,
				ostream & output);
	public:
		int should_skip() { return(count); } //std fare for stream fmts
		string get_extension() { return("_userdat.bbs"); }
		mbbs_users_ripper();
};

// returns true if we have a length string with maximum length limit_above
// only_numbers also accepts punctuation, because people who don't want to
// divulge their phone numbers often used punctuation as a substitute.
bool mbbs_users_ripper::is_length_string(char_rope::const_iterator pos,
	char_rope::const_iterator end, bool accept_zero, int limit_above,
	bool only_numbers) {

	// first get the length byte
	unsigned char lb = *pos;

	if (lb == 0 && !accept_zero) return(false);
	if (lb > limit_above) return(false);

	// now check that all the characters after it are printable
	for (int counter = 0; counter < lb && pos < (end-1); counter++) 
		if (only_numbers) {
			++pos;
			if (!isdigit(*pos) && !ispunct(*pos)) return(false);
		} else
			if (!isprint(*(++pos))) return(false);

	return(true);
}

bool mbbs_users_ripper::is_userdat_struct(char_rope::const_iterator pos,
		char_rope::const_iterator end) {

	if (pos >= end) return(false);

	// MBBS USERS.DAT user field has the following length strings
	// 	at 00: name, max length 30 chars (should have space also)
	// 	at 0x1F: street address, max length 30 chars
	// 	at 0x3E: postal code, max length 30 chars (who'da guessed it)
	// 	at 0x5D: telephone number, max length 15 chars, all numbers or -
	// 	at 0x6E: work telephone number, ditto
	
	if (!is_length_string(pos, end, false, 30, false)) return(false);
	if (!is_length_string(pos+0x1F, end, false, 30, false)) return(false);
	if (!is_length_string(pos+0x3E, end, false, 30, false)) return(false);
	if (!is_length_string(pos+0x5D, end, false, 15, true)) return(false);
	// almost never used, so be a bit forgiving
	if (!is_length_string(pos+0x6E, end, false, 15, false)) return(false);

	return(true);
}

int mbbs_users_ripper::get_header_here(char_rope::const_iterator pos, 
		char_rope::const_iterator end) {

	// standard code
	int p = check_matches(header_targets, common_to_header, pos, end);

	if (p == -1) return(p);         // no? shoo!

	if (!is_userdat_struct(pos, end)) return(-1);

	return(p);
}

// need a begin() for this
int mbbs_users_ripper::dump_from_header(char_rope::const_iterator pos,
		char_rope::const_iterator end, int max_length,
		ostream & output) {

	if (!is_header_here(pos, end)) return(0);
	
	// First scan backwards -- there may be some people in the users.dat
	// file who don't have the right postal code, and thus we passed them
	// by.
	// Note that count only counts the number of bytes *after* the header
	// we found at first. This way should_skip synchronizes correctly.
	
	char_rope::const_iterator incursion = pos;
	pos -= 0x400;

	while (is_userdat_struct(pos, end))
		pos -= 0x400;

	// now we're at the first that's not a header. Add 0x400 to get back 
	// on track.
	char_rope::const_iterator very_begin = pos + 0x400;
	pos = incursion + 0x400;

	// now scan forwards
	while (is_userdat_struct(pos, end))
		pos += 0x400;

	// we now are 0x400 past the last header.
	char_rope::const_iterator very_end = pos; // inclusive
	
	//cout << very_end - very_begin << endl;
	
	// And now, the moment you've all been waiting for
	count = 0;

	for (pos = very_begin; pos != very_end && pos != end; pos++) {
		if (pos >= incursion) count++;
		output << *pos;
	}

	return(count);
}
	
	

mbbs_users_ripper::mbbs_users_ripper() {
	// The postal code starts at 0x3F, but we ignore the first four digits
	// in favor of the name of the location itself. The space is included
	// in the match, thus there are 0x3F + 4 = 67 unknowns, a space, and
	// then the name.
	// Obviously only applicable to one country.

	count = 0;

	string prefix(67, '?');
	prefix += ' ';
	vector<string> headers;
	
	headers.push_back(prefix + "Arendal");
	headers.push_back(prefix + "Bergen");
	headers.push_back(prefix + "Bodø");
	headers.push_back(prefix + "Drammen");
	headers.push_back(prefix + "Fredrikstad");
	headers.push_back(prefix + "Hamar");
	headers.push_back(prefix + "Kristiansand");
	headers.push_back(prefix + "Leikanger");
	headers.push_back(prefix + "Lillehammer");
	headers.push_back(prefix + "Molde");
	headers.push_back(prefix + "Oslo");
	headers.push_back(prefix + "Porsgrunn");
	headers.push_back(prefix + "Sandnes");
	headers.push_back(prefix + "Sarpsborg");
	headers.push_back(prefix + "Skien");
	headers.push_back(prefix + "Stavanger");
	headers.push_back(prefix + "Steinkjer");
	headers.push_back(prefix + "Tønsberg");
	headers.push_back(prefix + "Tromsø");
	headers.push_back(prefix + "Trondheim");
	headers.push_back(prefix + "Vadsø");

	vector<string> footer(1, "");

	set_patterns('?', headers, footer, false); // case-insensitive
};
		
	
// Hybrid size-ripper / wildcard-ripper. I use wildcard ripper for its
// automatic clue construction.
class zzt_ripper : public wildcard_ripper {
	private:
		int get_header_here(char_rope::const_iterator pos,
				char_rope::const_iterator end);
	public:
		string get_extension() { return(".zzt"); }
		int suggest_max_length() { return(524288); }
		int dump_from_header(char_rope::const_iterator pos,
				char_rope::const_iterator end, int max_length,
				ostream & output);
		zzt_ripper(); // lip service to wildcard_ripper
};

int zzt_ripper::get_header_here(char_rope::const_iterator pos, char_rope::
		const_iterator end) {

	// Extremely Permissive ZZT Ripper
	// Done for quality checks.
	
	if (pos+0x200 > end) return(-1); // no zzt file is this small
	
	// first two FF?
	if ((u_char)*(pos) != 0xFF) return(-1);
	if ((u_char)*(pos+1) != 0xFF) return(-1);

	// OK, is number three zero? MSB for number of boards, but number
	// of boards is limited above by 0xA0
	if (*(pos+3) != 0) return(-1);

	// okay, does the LSB (byte 2) agree with limits?
	int num_boards = (unsigned char)*(pos+2) + 1; 
			// counter excludes title screen, so we add one.
	if (num_boards > 0xA0) return(-1);

	// Check title length first to distinguish from zero runs
	// Also do a sanity check
	if (*(pos+0x1D) == 0 || *(pos+0x1D) >= 0x30) return(-1);

	// Okay, now check the keys
	char_rope::const_iterator beginning = pos;
	for (pos = beginning + 8; pos < beginning + 0xD; pos++)
		if ((u_char)*pos > 1) return(-1); // must be either 0 or 1
	pos = beginning;

	// Final checks: Health, starting board and save game field
	// Health should always be > 0, anything else would result in instant
	// game over.
	if (*(pos+0x0F) == 0 && *(pos+0x10) == 0) return(-1);
	
	// It stands to reason the board we want to start at must actually
	// exist. Thus its index must be lower than the number of boards
	// present (actually <= since the "number of boards" index ignores
	// the title screen).
	if ((u_char)*(pos+0x11) >= num_boards)
		return(-1);
	
	// The save game field must be either 0 or 1.
	if (*(pos+0x108) > 1) return(-1);

	// Torch cycles remaining must be < 0xC6. Consequentially, the MSB
	// must be 0.
	if ((u_char)*(pos+0x15) >= 0xC6 || (u_char)*(pos+0x16) != 0) 
		return(-1);

	// if title length > 0 then the first byte of title must be printable
	if ((u_char)*(pos+0x1D) > 0 && !isprint(*(pos+0x1E))) return(-1);

	// And obviously, the board length of the title screen must be > 0.
	// (Actually at least 0xC0.)
	// (( Doesn't seem to be needed ))
	//if (*(pos+0x200) == 0 && *(pos+0x201) == 0) return(-1);
	
	
	return(0); // Success!
}

int zzt_ripper::dump_from_header(char_rope::const_iterator pos,
		char_rope::const_iterator end, int max_length, 
		ostream & output) {

	count = 0;
	int expected_len = 0;
	bool done = false;
	
	if (!is_header_here(pos, end)) return(0);

	char_rope::const_iterator begin = pos;

	// now we have a valid ZZT file. Get the number of boards so that
	// we can figure out its size.
	
	int num_boards = (unsigned char)*(pos + 2) + 1;

	// start traversing, boys!
	// From each board start position, get the next board start position,
	// updating as we go.
	pos += 0x200; // start of first board
	expected_len = 0x200;
	for (int i = 0; i < num_boards && !done; i++) {
		// LSB u_short (or signed? In any case, the real limit is
		// vastly lower, about 20K or so before ZZT starts acting
		// weird).
		int board_size = (unsigned char)*(pos) + 
			((unsigned char)*(pos+1) << 8);
		// If > 32K, something's amiss. We may have jumped wrongly, so
		// add the size and get outta here.
		expected_len += board_size + 2; // two bytes for the specifier
		if (board_size > 32767) {
			/*cout << "Unreasonable length detected. Bailing out at"
				<< expected_len << endl;*/
			done = true;
		}else {
	//		cout << "Reasonable length " << board_size << endl;
		}
		if (expected_len > max_length) done = true;
		
		pos += (board_size + 2);
	}

	// That's the hard part done. Now just dump as many bytes as we've got.
	
	pos = begin;
	for (count = 0; count < expected_len; count++)
		output << *pos++;

	return(count);
}

zzt_ripper::zzt_ripper() {
	string hdr(2, 0xFF);
	string ftr = "";

	set_patterns('?', hdr, ftr, true);
}

// Here we search for the "unique player" signature (run of 1, code of 04,
// color 0x1F). Then we go up to 1500 bytes backwards, searching for a possible
// beginning of the board at a fixed distance away (where it would be if the
// position in question was the first byte). Finally, we simply read off the
// correct length and dump that much.

// Note that this ripper will also dump the boards of ZZT files (as a BRD is
// just a disk dump of a single ZZT-file board). You'll have to sort those out 
// on your own.

// beware of scribbling "before" the start. We need a way to detect s.begin
// just as we do s.end.

// "Move this thing!" later.
//
// If rigor is on, the ripper checks the entire RLE field for oddities.
// More than 16 and the file is rejected. That narrows down false positives
// considerably, but may miss some corrupted boards. The reason I do this is
// because KE most likely won't be able to read such corrupted boards, so
// if you want to do manual recovery, leave rigor off, but if you just want
// to import into KE, leave it on.

class zzt_brd_ripper : public wildcard_ripper {
	private:
		bool rigor;
		int real_start_off;	// set by get_header_here
		int get_header_here(char_rope::const_iterator pos,
				char_rope::const_iterator end);
		int dump_from_header(char_rope::const_iterator position, 
				char_rope::const_iterator end,
				int max_length, ostream & output);
	public:
		void set_rigor(bool rigor_in) { rigor = rigor_in; }
		string get_extension() { return(".brd"); }
		int get_max_length() { return(32768); }
		zzt_brd_ripper();
};

int zzt_brd_ripper::get_header_here(char_rope::const_iterator pos, char_rope::
		const_iterator end) {

	// first do the usual check (pos is at the offset of the encoded
	// player)
	
	int p = check_matches(header_targets, common_to_header, pos, end);

	if (p == -1) return(p);         // no? shoo!
	
	// now travel backwards in steps of three until we match the title
	// as if this was the first byte. Do some simple checks while we're
	// at it.
	
	char_rope::const_iterator first_found = pos;
	
	pos+=3;

	real_start_off = -1;
	
	for (int counter = 0; counter < 1500 && real_start_off == -1 && 
			pos != end; counter++) {
		pos-=3; // skip backwards
		
		char_rope::const_iterator hypot = pos - 0x35; // A BRD's header
		// is 34 bytes. (Or apparently 35)
		
		// now we have size. Get it and do some sanity checks - the
		// size must be > 34 (obviously) and < 32K (ZZT stops working
		// at about 20K).
		// Experiment shows that the minimum brd file is of length 195
		// bytes. (Empty except for player)

		int putative_size = (u_char)*(hypot) + 
			((u_char)*(hypot+1) << 8);
	//	cout << "PS: " << putative_size << endl;
		if (putative_size < 195 || putative_size > 32768) continue;
		// then check that a file of the size we found actually 
		// includes the player we matched at the very beginning.
		if (putative_size < (first_found - hypot)) continue;
		
		// Okay, the size is reasonable. Now check the title.
		// Title length byte must be < 32 (use 48 for good measure)
		// and all the following bytes must be printable.
		
		char_rope::const_iterator at_beginning = hypot;
		
		int title_len = (u_char)*(hypot+2);
	//	cout << end - (hypot+2) << endl;
	//	cout << "TL: " << title_len << endl;
		// We need at least some leverage - 1 byte title
		// Find some way of doing away with this later

		// I don't see how I can do away with this - I'm getting false
		// "early" matches from the padding after the title as much
		// as it is already.
		// I could remove this and add KE's validity routine somewhere
		// instead, but bah.
		if (title_len > 48 || title_len == 0) continue;

		hypot += 3;
		bool check = true;
		for (int sec = 0; sec < title_len && check; sec++) {
			check = isprint(*hypot++);
	//		if (check) cout << *(hypot-1) << flush;
		}
	//	if (check) cout << "Check" << endl;

		if (!check) continue;

		// If title len < 3, it is possible that we have RLE triplets
		// masquerading as title bytes. This'll happen very scarcely
		// for title >= 3 (only if there's some text objects involved).
	
		// Check that what we have at hypothesis aren't RLE bytes
		// We check three of them. If we have a masquerade, the title
		// byte would be either number or code (unless the games maker
		// enjoys blue or cyan on black). Let's test them all.

		hypot = at_beginning;
		// first find where code is
		int code = -1;
		if ((u_char)*hypot < 0x3D && (u_char)*(hypot+3) < 0x3D && 
				(u_char)*(hypot+6) < 0x3D)
			code = 0;
		/*if ((u_char)*(hypot+1) < 0x3D && (u_char)*(hypot+4) < 0x3D &&
				(u_char)*(hypot+7) < 0x3D)
			code = 1;
		if ((u_char)*(hypot+2) < 0x3D && (u_char)*(hypot+5) < 0x3D &&
				(u_char)*(hypot+8) < 0x3D)
			code = 2;*/
		// Then check number (code + 2) - neither can be 0
	
//		cout << "Code is " << code << endl;
		hypot += 2;
		if (code != -1) {
		if (*(hypot + code) != 0 && *(hypot + code + 3) != 0 
				&& *(hypot + code + 6) != 0) continue;
		}

		//Check that the triplet after this one (since the RLE triplet
		//at pos must be the first for our hypothesis to hold) is also
		//a valid RLE triplet.

	//	if ((u_char)*(pos+1) > 0x3D) continue;

		// We're now about to accept the position. But if we're
		// rigorous, don't give up just yet. Check all RLE bytes
		// forward until we have sum number = 1500. Abort if we find
		// any where number == 00 or code >= 0x3D.
		if (rigor) {
			int rle_count = 0;
			char_rope::const_iterator rle_pos = pos;

			for (pos == rle_pos; pos != (rle_pos+1500) && 
					rle_count < 1500; pos += 3) {
				if (*pos == 0) return(-1); // number = 0
				// illegal code?
				if ((u_char)*(pos+1) >= 0x3D) return(-1);
				rle_count += (u_char)*pos;
			}
		}

		// If we're here, we probably have the right size
		real_start_off = counter;
	}

	if (real_start_off != -1) return(0);
	return(-1);
}

int zzt_brd_ripper::dump_from_header(char_rope::const_iterator position, 
		char_rope::const_iterator end, int max_length, 
		ostream & output) {
	// This is rather easy - just get real_start_off, offset pos by it,
	// get size, and go at it.
	
	// I'm breaking my own rules (never make a query function set something
	// that isn't obvious), but doing it the right way would be very slow.
	if (!is_header_here(position, end)) return(0);

	// now real_start_off is set.
	
	//cout << "RSO " << real_start_off << endl;
	
	position -= 0x35 + (3 * real_start_off);

	// We must take the 2 size bytes into consideration as well, thus +2.
	int length = (u_char)*position + ( (u_char)*(position+1) << 8) + 2;
	//cout << "Second time: " << length << ", " << end - position << endl;

	if (length > max_length) length = max_length;

	int counter;

	for (counter = 0; counter < length && position != end; counter++) 
		output << *position++;

	return(counter);
}

zzt_brd_ripper::zzt_brd_ripper() {
	rigor = true;
	string header = "xxx";
	header[0] = 1;
	header[1] = 4;
	header[2] = 0x1F;

	set_patterns('?', header, "", true);
}

// LHarc/LZH/LHA: This is like ZIP, with headers before each file. The strategy
// is as follows: 1. Find magic (compression method). 2. Do a few sanity 
// checks. 3. Jump forward based on "compressed byte length" field until we
// don't find a new header where we land. 4. Dump.
// First byte < 101 dec (obviously also > 0, begin header size), then 1 
// unknown, then compression method. Then compressed size. Offset 21 is length
// of pathname (file included) and onwards is the path/file itself (yes, check
// [21] > 0).

class lha_ripper : public wildcard_ripper {
	private:
		int get_header_here(char_rope::const_iterator pos, char_rope::
				const_iterator end);
	public:
		int dump_from_header(char_rope::const_iterator pos, char_rope::
				const_iterator end, int max_length, ostream &
				output);
		string get_extension() { return(".lzh"); }
		int suggest_max_length() { return(2097152); }
		int should_skip() { return(count); } // because of zip-like format
		lha_ripper();
};

int lha_ripper::get_header_here(char_rope::const_iterator pos, char_rope::
		const_iterator end) {

	// First check for the magic compression methods.
	int p = check_matches(header_targets, common_to_header, pos,
			end);

	if (p == -1) return(p);         // no? shoo!

	// Now check that the first byte is < 101 and > 21, maximum and
	// minimum header lengths
	
	if ((u_char)*pos > 101 || (u_char)*pos < 21) return(-1);
	// check offset 21 != 0
	int pathname_len = (u_char)*(pos+21);
	if (pathname_len == 0) return(-1);
	// name should be printable
	for (int counter = 22; counter < 22 + pathname_len; counter++)
		if (!isprint(*(pos+counter))) return(-1);

	cout << "Found header at " << end - pos << endl;

	return(0);
}

int lha_ripper::dump_from_header(char_rope::const_iterator pos, char_rope::
		const_iterator end, int max_length, ostream & output) {
	// First, if there is no header, forget it.
	// 
	// Starting at that header:
	// 	get compressed length + header length - (offset of size field)
	// 	go forwards and set avoid count to 20.
	// 	If avoid count == 0 then abort, else avoid count --,
	// 	look for header
	// 		If header found, cycle the loop
	// 	Otherwise continue
	
	if (!is_header_here(pos, end)) return(0);

	char_rope::const_iterator arch_pos; // position after last jump
	char_rope::const_iterator beginning = pos;
	int avoid_count = 100, size = 0;

	count = 0;

	bool done = false;
	
	while (count < max_length && !done) {
		while (!is_header_here(pos, end) && !done) {
			if (avoid_count <= 0) {
				done = true;
			} else {
				avoid_count--;
				pos++;
			}
		}

		if (done) continue;

		avoid_count = 100;
		int header_len = *pos;
		pos += 7; // Now we're at the compressed length field
		unsigned int compressed_len = (u_char)*pos + ((u_char)*(pos+1)
				<< 8) + ((u_char)*(pos+2) << 16) +
			((u_char)*(pos+3) << 24);

		//cout << "Compressed len " << compressed_len << endl;

		arch_pos = pos;
		pos += compressed_len - (header_len - 7);
		if (arch_pos > pos) pos = arch_pos + 1;		
		// in case of corrupt headers
		
	}

	arch_pos = pos;

	count = 0;

	// now just dump
	for (pos = beginning; pos != arch_pos && pos != end; pos++) {
		output << *pos;
		count++;
	}

	return(count);
}

lha_ripper::lha_ripper() {
	vector<string> headers;
	// Archive magics can be  -lz[0145s]- and -lh[01\40d234567]-
	// They all start at relative position 2.
	string pre = "??"; // two first bytes

	headers.push_back(pre + "-lz0-");
	headers.push_back(pre + "-lz1-");
	headers.push_back(pre + "-lz4-");
	headers.push_back(pre + "-lz5-");
	headers.push_back(pre + "-lzs-");
	headers.push_back(pre + "-lh0-");
	headers.push_back(pre + "-lh1-");
	headers.push_back(pre + "-lh\040-");
	headers.push_back(pre + "-lhd-");
	headers.push_back(pre + "-lh2-");
	headers.push_back(pre + "-lh3-");
	headers.push_back(pre + "-lh4-");
	headers.push_back(pre + "-lh5-");
	headers.push_back(pre + "-lh6-");
	headers.push_back(pre + "-lh7-");

	vector<string> footers(1, "");

	set_patterns('?', headers, footers, true);
	count = 0;
}

// Now a bit more complex as it counts data and central directory entries.
// If both are the same, it assumes the file is proper and that thus there's
// no need to revisit the area.

class zip_ripper : public wildcard_ripper {
	private:
		int last_consistent_count; // of headers that have
			//correct size specifications; used for skipping chunks
			//that are correct once we dump hem.
		int header_count; // actual files in zip
		int centdir_count;
		int footer_offset(char_rope::const_iterator footer_st,
				char_rope::const_iterator rope_end);
		bool is_centdir_here(char_rope::const_iterator pos,
				char_rope::const_iterator end);
	public:
		zip_ripper();
		string get_extension() { return(".zip"); }
		int dump_from_header(char_rope::const_iterator position, 
				char_rope::const_iterator end, int max_length, 
				ostream & output);
		// If the central directory already specifies more files than
		// we see, we're not going to get those files by shortening the
		// zip, so skip it.
		int should_skip() { 
			if (centdir_count>=header_count) return(count);
			else return(last_consistent_count); }
};

// is there a central directory listing here?
bool zip_ripper::is_centdir_here(char_rope::const_iterator pos, 
		char_rope::const_iterator end) {
	if (*pos++ != 'P') return(false);
	if (*pos++ != 'K') return(false);
	if (*pos++ !=  01) return(false);
	return (*pos == 02);
}

int zip_ripper::footer_offset(char_rope::const_iterator footer_start,
		char_rope::const_iterator rope_end) {

	// PK 05 06 and then the 17 and 18th byte after that: byte count in
	// LSB for the comment section.
	
	int lsb = (unsigned char)*(footer_start + 20);
        int msb = (unsigned char)*(footer_start + 21);
	int footer_size = lsb + (msb << 8);

	if (footer_size > 16384) footer_size = 0;

	if (footer_start + footer_size + 18 - 1 > rope_end)
		return(0);

	return(footer_size + 18);
}

zip_ripper::zip_ripper() {
	// Header is PK 03 04, footer is PK 05 06.
	// Since .zip is really a chunked format, we'll get loads of
	// head-truncated files. Deal with that externally - the only way
	// to get around it would be to read off the numbers of files from the
	// central directory (in the footer) and compare the number of PK 03
	// 04 past to that; but the way the stream output works, we can't
	// just cancel the file if there's a mismatch. Maybe return false in
	// that case and let the main rip program delete the file? Hm. But
	// that would cause great numbers of false negatives (say a file with
	// one of its PK 03 04 blown away by noise [if matching on central
	// directory number too high] or one with its PK 05 06 blown away
	// with another zip file just after it [if matching on central
	// directory number too low]. What we really need is for the ZIP not
	// to visit areas we've dumped before unless there's a missing PK 05
	// 06 there somewhere - that is, if the central directory number is
	// too low.
	
	last_consistent_count = 0;
	header_count = 0;
	centdir_count = -1;
	string hdr = "PKxx?x???x"; // PK 03 04 then the MSB of the version
				   // byte is 0, then 2 generalized bit field
				   // bytes (don't know their values) then the
				   // MSB of the compression method is 0.
	string ftr = "PKxx";
	hdr[2] = 3;
	hdr[3] = 4;
	hdr[5] = 0;
	hdr[9] = 0;
	ftr[2] = 5;
	ftr[3] = 6;
	set_patterns('?', hdr, ftr, true);
}

// stolen from footer_ripper

int zip_ripper::dump_from_header(char_rope::const_iterator position,
                char_rope::const_iterator end, int max_length, ostream &
                output) {

	count = 0;
	last_consistent_count = 0;

        if (get_header_here(position, end) == -1) return(0);

	// first null out the counters
	header_count = 1;
	centdir_count = 0; // already have one header

	bool contradict = false; // becomes true once we find a jump from one
	// header to another that's longer than the header's "compressed data"
	// field implied.

	// size of the compressed file. We don't compare headers inside
	// the compressed data itself, so that ZIP files stored in other
	// ZIP files under compression method "Stored" (no compression)
	// don't yield false positives. On the other hand, this may cause
	// false negatives if the compressed data length file is too large;
	// that would result in many zip files where there should be only
	// one, so it's not the end of the world.
	int avoid_count = (u_char)*(position+18) +
		((u_char)*(position+19) << 8) + 
		((u_char)*(position+20) << 16) +
		((u_char)*(position+21) << 24);

        while ((get_footer_here(position, end) == -1) && position != end &&
                        count < max_length) {
                output << *position++;
		if (position == end) continue;
		avoid_count--;
		
		// SLOW ! Oh well. Kludge a bit to make quicker and to avoid
		// "zips within zips"
		if (*position == 'P' && avoid_count <= 0) {
			if (get_header_here(position, end) != -1) {
//				cout << "Avoid count: " << avoid_count << endl;
				if (avoid_count > -60 && !contradict) {
//					cout << "Setting LCC" << endl;
					// -1 so that it'll find the header
					// next time.
					last_consistent_count = count - 1;
				} else
					contradict = true;
				header_count++;
				// skip 18 bytes, then LSB
				avoid_count = (u_char)*(position+18) + 
					((u_char)*(position+19) << 8) + 
					((u_char)*(position+20) << 16) + 
					((u_char)*(position+21) << 24);
			}else {
				if (is_centdir_here(position, end))
					centdir_count++;
			}
		}
		
                count++;
        }

        /* For zips etc which have "dangling sections" after the real footer. */
        int footer_identified = get_footer_here(position, end);
        int remaining = 0;

        if (footer_identified != -1)
                remaining = footer_offset(position, end) +
                        get_footer_length(footer_identified);

        for (int counter = 0; counter < remaining ; counter++)
                output << *position++;

	cout << "ZIP RIPPER: " << header_count << "\t" << centdir_count << endl;
	count += remaining;

        return(count);
}


/////////////////////////////////////////////////////////////////////////////
// Dumb rippers: where it isn't worth it to code the traversal logic.
// (These dump a fixed length)

// For some reason this doesn't work. (Now it does.)

// Supported: MZB (2.x), DOC, IT, GDM, S3M, VOC, MOD, XM
// To add: MBBS config

// this is one instance where a dumb ripper actually works.
// The config file is always (or seems always to be) 20017 bytes, but to be
// certain, we round up to 32768.

class mbbs_conf_ripper : public dumb_ripper {
	public:
		string get_extension() { return("_mbbs.conf"); }
		int suggest_max_length() { return(32768); }
		mbbs_conf_ripper();
};

mbbs_conf_ripper::mbbs_conf_ripper() {
	// The magic is MBBS\4 then 77 unknowns then "AT" (beginning of 
	// init string)
	
	string magic = "MBBS";
	magic.push_back(4);
	string nobodyknows(77, '?');
	magic += nobodyknows + "AT";

	set_patterns('?', magic, true);
}

class mzb_ripper : public dumb_ripper {
	public:
		string get_extension() { return(".mzb"); }
		int suggest_max_length() { return(65536); }
		mzb_ripper();
};

mzb_ripper::mzb_ripper() {
	// ff 4d 42 32
	string hdr(4, 0xFF);
	hdr[0] = 0xFF;
	hdr[1] = 0x4D;
	hdr[2] = 0x42;
	hdr[3] = 0x32;

	set_patterns('?', hdr, true);
}

// TODO: Check this and IT ripper [DONE]

// This, too, is out of order, since it started as a dumb ripper. Pretty soon
// I found out that it creates too large files, though.

// For some odd reason, the GDM ripper always rips slightly more than it should,
// on the order of 83 bytes per MB. I don't know where the excess comes from, 
// but it seems to be harmless, though it may suggest a deeper bug somewhere 
// "under the carpet".
class gdm_ripper : public size_ripper {
	private:
		u_short get_short(char_rope::const_iterator pos);
		int get_long(char_rope::const_iterator pos);
		int get_integer(char_rope::const_iterator pos);
	public:
		int suggest_max_length() { return(4194304); }
		string get_extension() { return(".gdm"); }
		gdm_ripper();
};

// There used to be no return here. But the function still worked, albeit
// only without optimization. Odd!
u_short gdm_ripper::get_short(char_rope::const_iterator pos) {
	return( (u_char)*pos + ((u_char)*(pos+1) << 8) );
}

int gdm_ripper::get_long(char_rope::const_iterator pos) {
	// That's a neat trick. 
	return (get_short(pos) + (get_short(pos+2) << 16));
}

int gdm_ripper::get_integer(char_rope::const_iterator pos) {
	// Here we're going to find the real size of the GDM file. That is
	// done by finding the length of the order (what order the patterns
	// go in), of the patterns in total, and of the samples in total. Then
	// it is just a matter of adding up.
	
	// We assume pos is at 0, just like for MOD
	
	// First we need some information from the header. I assume LSB.
	int orderlen = (u_char)*(pos+0x7A);	// length of order pattern
	int pattern_off = get_long(pos+0x7B);	// where the patterns start
	int num_pats = (u_char)*(pos+0x7F);	// number of patterns
	int smp_hdr_off = get_long(pos+0x80);	//where the sample headers start
	int num_samples = (u_char)*(pos+0x88);	// how many samples

	// Apparently the num_pats and num_samples count from zero, so the
	// real number of patterns and samples is at least 1. That was something
	// that stumped me as the GDM file specification didn't mention it, but
	// it seems to work, so we adjust it here.
	num_pats += 1;
	num_samples += 1;

	cout << "Number of patterns: " << num_pats << endl;
	cout << "Number of samples: " << num_samples << endl;

	// The order length is simple, we already have it.
	
	// Now we get the pattern length by going to the first offset and then
	// just following through. The pattern is first 2 bytes of size then
	// the pattern itself, so that is rather easy. Since the 2 bytes of
	// size does not include itself, we add 2 to the total pattern length
	// each iteration.
	
	int total_pattern_length = 0;
	int cur_pattern;
	char_rope::const_iterator pat_pos = pos + pattern_off;
	for (cur_pattern = 0; cur_pattern < num_pats; cur_pattern++) {
		int this_pattern_len = get_short(pat_pos);
		total_pattern_length += this_pattern_len + 2;
		pat_pos += this_pattern_len;
	}

	// Now we get the sample length. This we do by traversing the sample
	// headers in order - they're all 0x3E bytes and the size (u_long) is
	// at 0x2D. We add both the sample data length (specified by the bytes
	// at 0x2D onwards) and the sample header length (constant, 0x3E) to
	// the count each time so that he size takes into account both header
	// and body. Finally, as a precaution, if the sample length gets
	// absurdly large (more than 16MB), we return it and go no further.
	
	int total_sample_length = 0;
	int cur_sample;
	char_rope::const_iterator smp_pos = pos + smp_hdr_off + 0x2D;
	for (cur_sample = 0; cur_sample < num_samples; cur_sample++) {
		//cout << "Sample verification " << *(smp_pos-0x2D) << *(smp_pos-0x2C) << endl;
		int this_sample_len = get_long(smp_pos);
		if (this_sample_len > 16777216 || this_sample_len < 0) {
			cout << "Absurd sample length of " << this_sample_len
				<< " detected!" << endl;
			break;
		}
		
		//cout << "This sample length: " << this_sample_len << endl;
		total_sample_length += this_sample_len + 0x3E;
		smp_pos += 0x3E; // next header!
	}

	//cout << "What we have: " << total_pattern_length << "\t" << total_sample_length << "\t" << orderlen << endl;

	// The 0xA0 is the size of the initial header, and thus must also
	// be added.
	return(total_pattern_length + total_sample_length + orderlen + 0xA0);
}
			
gdm_ripper::gdm_ripper() {
	// Magic is "GDM" then 0xFE then lots of wildcards then "GMFS".
	string header(71, '?');
	header[0] = 'G';
	header[1] = 'D';
	header[2] = 'M';
	header[3] = 0xFE;
	header = header + "GMFS";

	set_size_pattern('?' , header, true, 0, 0, 4, 1, false);
}

// I'm NOT going to learn the monstrosities that are S3M and IT. 
// Just "IMPM" then up to 26 printables plus 00 - there must be a 00, which
// distinguishes it from plain text.
class s3m_ripper : public dumb_ripper {
	public:
		string get_extension() { return(".s3m"); }
		int suggest_max_length() { return(2097152); }
		s3m_ripper();
};

s3m_ripper::s3m_ripper() {
	// 44 unknowns then SCRM
	string header(44, '?');
	header[28] = 0x1A; // another magic
	header = header + "SCRM";
	
	set_patterns('?', header, true);
}

class it_ripper : public dumb_ripper {
	private:
		int get_header_here(char_rope::const_iterator pos, char_rope::
				const_iterator end); 
	public:
		string get_extension() { return(".it"); }
		int suggest_max_length() { return(2097152); }
		it_ripper() { set_patterns('?', "IMPM", true); }
};

int it_ripper::get_header_here(char_rope::const_iterator pos, char_rope::
		const_iterator end) {
	// first check if we're at the header
	int p = check_matches(header_targets, common_to_header, pos,
			end);

	if (p == -1) return(p); 	// no? shoo!

	// now check the printables. There must be at least one zero and all
	// before it must be printable.

	pos += 4;
	for (int counter = 0; counter < 26 && pos != end; counter++) {
		char atpos = *pos++;
		if (!isprint(atpos)) { // not printable
			if (atpos != 0) return(-1); // and not 0 either? Begone!
			else return(0); // else speak friend and enter
		}
	}

	return(-1); // no terminating zero
}

class mzx_sav_ripper : public dumb_ripper {
	public:
		string get_extension() { return(".sav"); }
		int suggest_max_length() { return(2097152); }
		mzx_sav_ripper();
};

mzx_sav_ripper::mzx_sav_ripper() {
	vector<string> headers;

	string first = "MZS";
	first.push_back(2);	// 2.x
	headers.push_back(first);
	first = "MZXSA";	// >2.50?
	first.push_back(0);
	headers.push_back(first);
	headers.push_back("MZSAV");	// 1.x

	set_patterns('?', headers, true);
};

// can't be used yet, because of clue problems (Now works.)
class doc_ripper : public dumb_ripper {
	public:
		string get_extension() { return(".doc"); }
		int suggest_max_length() { return(2097152); }
		doc_ripper();
};

doc_ripper::doc_ripper() {
	// Magics: Red Alert version (unknown): 0xD0CF11E0
	// U2rest: 0xDBA52D00
	// Very old: 0x9BA52100
	
	//00161a00  db a5 2d 00 78 40 09 08  00 00 00 00 2d 00 00 00  |Û¥-.x@......-...|
	//00163600  db a5 2d 00 78 40 09 08  00 00 00 00 2d 00 00 00  |Û¥-.x@......-...|
		
	
	string hdr = "xxxx";
	vector<string> headers(0);
	hdr[0] = 0xD0; hdr[1] = 0xCF; hdr[2] = 0x11; hdr[3] = 0xE0;
	headers.push_back(hdr);
	hdr[0] = 0xDB; hdr[1] = 0xA5; hdr[2] = 0x2D; hdr[3] = 0; // OOPS was 4
	headers.push_back(hdr);
	hdr[0] = 0x9B; hdr[1] = 0xA5; hdr[2] = 0x21; hdr[3] = 0;
	headers.push_back(hdr);

	set_patterns('?', headers, true);
}

// Out of sequence, yes. But once it was in sequence and I'm not going to
// retype just to get it in the right order.

// It's not worth the hassle.
class mod_ripper : public size_ripper {
	private:
		int get_integer(char_rope::const_iterator pos);
	public:
		string get_extension() { return(".mod"); }
		mod_ripper();
		int suggest_max_length() { return(2091752); }
};

int mod_ripper :: get_integer(char_rope::const_iterator pos) {
	// Abusers of the get_integer function, unite!
	// Length of a mod file is equal to 1084 + (number of patterns * 1024) +
	// sum of sample lengths. 
	//
	// We assume this is called with pos at the very beginning.
	
	// To get the sample length: start at offset 20. Skip 22 bytes and
	// read the next two. Multiply the result by 2. That's the length
	// of that sample. Now skip those and 6 more bytes to get to the
	// next header. Repeat for a total of 31 times.
	// (No, the headers are in one chunk, it appears.)
	int samplelen = 0;
	int sample_count;
	char_rope::const_iterator samp_pos = pos + 42;
	for (sample_count = 0; sample_count < 31; sample_count++) {
		//cout << "SAM" << samplelen << endl;
		// LSB i presume. No, MSB
		// words are 16 bit values, bytes are 8, so multiply and add
		int words = ((u_char)*samp_pos << 8) + (u_char)*(samp_pos+1);
		samplelen += words * 2;
		samp_pos += 30; // apparently the headers are all in one chunk
	}
	
	// To get the pattern length: go to byte 952. Scan the next (inclusive)
	// 128 bytes - the largest is the number of patterns. (Minus one; it
	// appears MOD counts from 0, reckoning there's always a "first 
	// pattern".)
	pos += 952;
	int record_pattern = 0;
	for (int counter = 0; counter < 128; counter++) {
		if ((u_char)*pos > record_pattern)
			record_pattern = (u_char)*pos;
		pos++;
		//cout << "RP: " << record_pattern << endl;
	}

	return(1084 + (record_pattern+1) * 1024 + samplelen);
}

mod_ripper::mod_ripper() {
	//Module files have one of the following four bytes at location
	//1080: "M.K.", "M!K!", "FLT4", "FLT8", "4CHN", "6CHN", "8CHN",
	//"CD81", "OKTA", "16CN", or "32CN".
	
	// 1079 unknowns then 00 then one of a number of signatures then 00.
	string first(1079, '?');
	first.push_back(0);
	string last(1, 0);

	vector<string> headers;
	headers.push_back(first + "M.K." + last);
	headers.push_back(first + "M!K!" + last);
	headers.push_back(first + "FLT4" + last);
	headers.push_back(first + "FLT8" + last);
	headers.push_back(first + "4CHN" + last);
	headers.push_back(first + "6CHN" + last);
	headers.push_back(first + "8CHN" + last);
	headers.push_back(first + "CD81" + last);
	headers.push_back(first + "OKTA" + last);
	headers.push_back(first + "16CN" + last);
	headers.push_back(first + "32CN" + last);

	//set_patterns('?', headers, true);
	set_size_pattern('?' , headers, true, 0, 0, 4, 1, false);
}

// There is a complex way to do this too, but meh
class voc_ripper : public dumb_ripper {
	public:
		string get_extension() { return(".voc"); }
		int suggest_max_length() { return(1048576); } // Most are short
		voc_ripper();
};

voc_ripper::voc_ripper() {
	// The magic is "Creative Voice File" followed by 1A then 1A 00.
	string magic = "Creative Voice File";
	magic.push_back(0x1A);
	magic.push_back(0x1A);
	magic.push_back(0);

	set_patterns('?', magic, true);
}

class xm_ripper : public dumb_ripper {
	private:
		int get_header_here(char_rope::const_iterator pos,
				char_rope::const_iterator end);
	public:
		string get_extension() { return(".xm"); }
		int suggest_max_length() { return(4194304); }
		xm_ripper();
};

int xm_ripper::get_header_here(char_rope::const_iterator pos,
		char_rope::const_iterator end) {
	// first the usual
	int p = check_matches(header_targets, common_to_header, pos,
			end); // polly have a cracker?

	if (p == -1) return(-1); // Awwk! No.

	// now check the following:
	// Byte at offset 37 is 0x1A (chaffs out text files)
	// Byte at offset 58 is 0x01 and the next should be < 0x1F.
	
	//cout << "Past point one " << endl;
	
	if (*(pos+37) != 0x1A) return(-1);
	if (*(pos+59) != 1) return(-1);
	if ((u_char)*(pos+59) >= 0x1F) return(-1);

	return(p);
}

xm_ripper::xm_ripper() {
	set_patterns('?', "Extended Module: ", true);
}
		


// TODO: Test this on a protected world.
class mzx_ripper : public dumb_ripper {
	private:
		int get_header_here(char_rope::const_iterator pos, 
				char_rope::const_iterator end);
	public:
		vector<clue_bundle> get_clues(int length_wanted);
		string get_extension() { return(".mzx"); }
		int suggest_max_length() { return(2097152); }
		mzx_ripper();
};

int mzx_ripper::get_header_here(char_rope::const_iterator pos, char_rope::
		const_iterator end) {
	// We cheat a bit here. We first check byte number 25 against 0,1,2,3.
	// If it is 0, we check the next bytes for MZ[X2A], otherwise, we skip
	// 15 bytes (the password). The idea is that the correct match for an 
	// unprotected MZX file will get here in time (due to backtrace in
	// pngrip.cc), and thus we don't have to code complex what-if code for
	// the two cases.
	
	if (end - pos < 80) return(-1); // file is too small.
	
	// First of all, check that the byte we're on is either 0 or printable
	// (first byte of title)
	//cout << "Checking title" << endl;
	//cout << (u_int)((u_char)*pos) << endl;
	if ((u_char)*pos != 0 && !isprint(*pos)) return(-1);
	//cout << "Checking protection byte" << endl;
	
	char x = *(pos+25);
	if (x > 3) return(-1);
	//cout << "Checking p (x was " << (int)x << ")" << endl;
	//cout << *(pos+26) << endl;

	// unprotected; the next should be "M" then "Z". Some trickery does
	// the job
	char_rope::const_iterator at_version;

	// F. check_matches until I get clues installed.
	
	if (x == 0)
		at_version = pos + 41 - 15;
	else 
		at_version = pos + 41;

	// Check MZ
	if (*at_version != 'M') return(-1);
	if (*(at_version+1) != 'Z') return(-1);
	char distinguish = *(at_version+2);
	if (distinguish != '2' && distinguish != 'X' && distinguish != 'A')
		return(-1);

	//cout << "P check done" << endl;

	// Okay, now check that char 0 is either empty or full.
	// Since the encryption is just single byte xor, the equality test
	// works equally well if the file is encrypted. (But does it work for
	// MZX 1.x? I think the charsets are in the same place, so yes.)
	
	char compare = *(at_version + 3);
	for (pos = at_version + 4; pos != at_version + 4 + 14 && pos != end; 
			pos++) {
		if (*pos != compare) return(-1);
	}

	return(0);
}

vector<clue_bundle> mzx_ripper::get_clues(int length_wanted) {
	// I can see you cringe right here and now!
	// Well, this will have to hold until I get the real clue generator
	// changed.
	
	// (We can't match on MZ because that's taken by EXE)
	vector<clue_bundle> toRet;
	
	if (length_wanted != 2) return(toRet);

	clue_bundle first;
	first.offset = 41;
	first.case_sensitive = true;
	first.clues.resize(0);
	first.clues.push_back("ZX");
	first.clues.push_back("Z2");
	first.clues.push_back("ZA");
	first.clues.push_back("\002\010");
	toRet.push_back(first);

	return(toRet);
}


mzx_ripper::mzx_ripper() {
	// Worst case (protected file) there are 40 unknown bytes and then
	// MZX, MZ2, or MZA. Best case, there are 25 unknown bytes and then
	// the same. Thus we use 40 because for unprotected ones it will just
	// match a little earlier.
	string base_hdr(40, '?');
	string first = base_hdr + "MZX";
	string sec = base_hdr + "MZ2";
	string tri = base_hdr + "MZA";
	string tet = base_hdr + "M";	// apparently this is also a magic
	tet.push_back(2);
	tet.push_back(8);

	vector<string> headers;
	headers.push_back(first);
	headers.push_back(sec);
	headers.push_back(tri);
	headers.push_back(tet);

	set_patterns('?', headers, true);
}

// new tiff

class lendian_tif_ripper : public dumb_ripper {
  public:
   string get_extension() { return("_le.tiff"); }
   lendian_tif_ripper();
};

lendian_tif_ripper::lendian_tif_ripper() {
  string recognize(4, 0xFF);
  recognize[0] = 'I';
  recognize[1] = 'I';
  recognize[2] = 42;
  recognize[3] = 0;
  set_patterns('?', recognize, true);
}

class bendian_tif_ripper : public dumb_ripper {
  public:
   string get_extension() { return("_be.tiff"); }
   bendian_tif_ripper();
};

bendian_tif_ripper::bendian_tif_ripper() {
  string recognize(4, 0xFF);
  recognize[0] = 'M';
  recognize[1] = 'M';
  recognize[2] = 0;
  recognize[3] = 42;
  set_patterns('?', recognize, true);
}

