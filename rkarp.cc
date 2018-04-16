#include <stdio.h>
#include <string.h>

#include <iostream>
#include <vector>
#include <string>
#include <math.h>
#include <queue>
#include <deque>
#include <list>
#include <map>

using namespace std;

// If you have hash maps, use them. Almost no change in speed with 32 
// footprints, 12% faster than an ordinary map with 256.

#define USE_HASH_MAP

#ifdef USE_HASH_MAP
 #include <ext/hash_map>
 namespace SGI = ::__gnu_cxx;
 #define lookup_map SGI::hash_map
#endif
#ifndef USE_HASH_MAP
 #define lookup_map map
#endif

// Rabin-Karp substring search. Stolen from a university course text, as I
// don't know how the number theory stuff works (never was good at the CRT).

// We don't support collisions either, because instantiating a list on every
// return is too costly.

// I'm suspecting char vs unsigned char is what's responsible for the PNG
// signature not matching, but it could be a problem in acquisition instead
// (though I doubt it), and thus in the main program, not in this hash class.
// To figure whether it is, ressurrect the main() here with some PNG headers
// to test. If that works, try printf-debugging the main program to see if it
// somehow skips past the second PNG header.
//	I was right. Hunches are nice.

// Now uses deques, which sped up the process (for 4-char signatures) by
// 25%. Changed that to queue to make the workings more clear, for a slight
// improvement (~2%). Also changed char to unsigned char and int to unsigned
// int to clear up the signedness issues once and for all.

class RKSearch {
	private:
		bool case_sensitive;
		lookup_map<unsigned long, unsigned int> lookup_table;
		queue<unsigned char> c_queue;
		int count;
		unsigned long fast_b, fast_m, top_one, needle_len, needle_hash,
		     haystack_hash;
		template<typename T> unsigned long hash (const T str, long len,
				long b, long m);
	/*	unsigned long hash (const deque<unsigned char> & str, long b, 
				long m);*/
		unsigned long exp_mod(unsigned long x, unsigned long n,
				unsigned long m);
		// returns -1 if none, otherwise the response number
		// Used to avoid redundancy
		int return_value(int potential_hash);
	public:
		bool can_add_all(const vector<string> needles);// no collisions?
		bool add_all(const vector<string> needles, int response_number);
		bool add(const string needle, int response_number);
		RKSearch(bool case_sens, int length);
		int found(unsigned char next_char); // returns response number or -1
		int found (unsigned char * buffer, int position, int size); // optimization check
		int get_length() const { return(needle_len); }
};

int RKSearch::return_value(int potential_hash) {
	if (lookup_table.find(potential_hash) != lookup_table.end())
		return(lookup_table.find(potential_hash)->second);
	else	return(-1);
}

template<typename T> unsigned long RKSearch::hash (const T str, long len, 
		long b, long m) {
	
	long i;
	unsigned long value = 0;
	unsigned long power = 1;

	for (i = len - 1; i >= 0; i--) {
		unsigned char x = str[i];
		if (case_sensitive)
			value += (power * x);
		else	value += (power * tolower(x));
		value %= m;
		power *= b;
		power %= m;
	}
	
	return (value);
}

/*unsigned long RKSearch::hash (const deque<unsigned char> & str, long b, 
		long m) {
	deque<unsigned char>::const_iterator pos = --str.end();

	unsigned long value = 0, power = 1;

	for(;;) {
		if (case_sensitive)
			value += (power * *pos);
		else	value += (power * tolower(*pos));
		
		value %= m;
		power *= b;
		power %= m;

		if (pos == str.begin())	return(value);
		else			pos--;
	}
}*/

// Calculates x^n % m

unsigned long RKSearch::exp_mod (unsigned long x, unsigned long n, unsigned 
		long m) {
	if (n == 0)
		return (1);
	else if (n == 1)
		return (x % m);
	else {
		unsigned long square = (x * x) % m;
		unsigned long exp = exp_mod (square, n / 2, m);

		if (n % 2 == 0)
			return (exp % m);
		else
			return ((exp * x) % m);
    }
}

bool RKSearch::add(const string needle, int response_number) {
	// RK search has a limitation in that all "needle" strings must be
	// of the same size. This lets the user know if he tried to add a
	// string of the wrong length.
	
	if (needle.size() != needle_len) return(false);

	needle_hash = hash(needle, needle_len, fast_b, fast_m);

	if (lookup_table.find(needle_hash) != lookup_table.end())
		return(false); // collision

	lookup_table[needle_hash] = response_number;
	return(true);
}

bool RKSearch::can_add_all(const vector<string> needles) {

	for (int counter = 0; counter < needles.size(); counter++) {
		int needle_hash = hash(needles[counter], needle_len, fast_b,
				fast_m);
		
		if (lookup_table.find(needle_hash) != lookup_table.end())
			return(false); // collision
	}

	return(true); // None - yay!
}

bool RKSearch::add_all(const vector<string> needles, int response_number) {
	// we ignore the add function's return because this allows us to
	// pass multiple strings of same content (from inefficient clue 
	// functions) without error.
	
	if (!can_add_all(needles)) return(false);
	
	for (int counter = 0; counter < needles.size(); counter++) {
//		cout << "Adding " << needles[counter] << endl;
		add(needles[counter], response_number);
	}

	return(true);
}

RKSearch::RKSearch(bool case_sens, int length) {
	needle_len = length;
	case_sensitive = case_sens;
	fast_b = 257;
	fast_m = 16777216; // go higher and for some reason .png doesn't match
				// that's because 16777216*2 > 257^3. (four
				// bytes to match)

	// This enforces the limit mentioned above.
	while (fast_m > pow(fast_b, needle_len-1)) fast_m >>= 2;

	haystack_hash = 0;

	//cout << fast_m << endl;
	
	top_one = exp_mod (fast_b, needle_len, fast_m);
	count = 0;
};

int RKSearch::found (unsigned char next_char) {

	// Deque ops account for about 14% at light load (only one entry in the
	// hash). The calculations account for 11.6%, and lookup accounts for
	// the rest.
	
	unsigned char to_add;

	if (case_sensitive)
		to_add = next_char;
	else	to_add = tolower(next_char);

	c_queue.push(to_add);

	if (count <= needle_len) count++;
	
	haystack_hash *= fast_b;
	/* Only start removing from the queue if it's completely filled */
	if (count > needle_len) {
		haystack_hash -= ((c_queue.front() * top_one) & (fast_m - 1));
		c_queue.pop();
	}
	haystack_hash += to_add;
	haystack_hash &= (fast_m - 1);

	if (haystack_hash < 0)
		haystack_hash += fast_m;

	//cout << haystack_hash << endl;
	
	// If there aren't enough bytes, don't give him any false positives
	if (count < needle_len)
		return(-1);

	return (return_value(haystack_hash));
}

// QnD, doesn't actually match anything yet
// Does now!
int RKSearch::found (unsigned char * buffer, int position, int size) {
	int maxpos = size - 1;

	//cout << "Hello" << endl;

	if (position <= needle_len || !case_sensitive) 
		return(found(buffer[position]));

	if (maxpos - position <= needle_len) {
	//	cout << "MPP: " << maxpos-position << "\t" << needle_len << endl;
		if (maxpos - position == needle_len) {
	//		cout << "Cleaning up (maxpos " << maxpos << ")" << endl;
			while (!c_queue.empty()) c_queue.pop();
		}
		else {
	//		cout << "Pushing " << (int)buffer[position] << endl;
			c_queue.push(buffer[position]);
		}

	//	cout << "RKV: queue size: " << c_queue.size() << endl;
	}
	//cout << c_queue.size() << endl;

	haystack_hash *= fast_b;
	// Only start removing from the queue if it's completely filled 
	// Right
//	cout << "Top is " << (int)c_queue.front() << endl;
//	cout << "To delete is " << (int)buffer[position-needle_len] << endl;
	haystack_hash -= ((buffer[position-needle_len] * top_one) & (fast_m - 1));
	haystack_hash += buffer[position];
	haystack_hash &= (fast_m - 1);

	// No longer needed because we automatically wrap around
	/*if (haystack_hash < 0)
		haystack_hash += fast_m;*/

	// WARNING: Expensive call ahead!

	return (return_value(haystack_hash));
}



/*int main() {
	string haystack = "rParpKarpkarp";
	string needle = "karp";

	RKSearch testing(true, needle, 1);

	for (int counter = 0; counter < haystack.size(); counter++) {
		if (testing.found(haystack[counter]) == 1) cout << counter << endl;
	}

	//cout << stringSearchRKFast(needle, haystack) << endl;
}*/
