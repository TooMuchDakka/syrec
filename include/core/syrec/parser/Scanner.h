

#if !defined(parser_COCO_SCANNER_H__)
#define parser_COCO_SCANNER_H__

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

// io.h and fcntl are used to ensure binary read from streams on windows
#if _MSC_VER >= 1300
#include <io.h>
#include <fcntl.h>
#endif

#if _MSC_VER >= 1400
#define coco_swprintf swprintf_s
#elif _MSC_VER >= 1300
#define coco_swprintf _snwprintf
#elif defined __MINGW32__
#define coco_swprintf _snwprintf
#else
// assume every other compiler knows swprintf
#define coco_swprintf swprintf
#endif

#define COCO_WCHAR_MAX 65535
#define COCO_MIN_BUFFER_LENGTH 1024
#define COCO_MAX_BUFFER_LENGTH (64*COCO_MIN_BUFFER_LENGTH)
#define COCO_CPP_NAMESPACE_SEPARATOR L':'

namespace parser {


unsigned int hashString(const std::string_view& stringToHash);

std::string createFrom(const wchar_t *value);
wchar_t* coco_string_create(const std::string& data, size_t startPositionInData, size_t numBytesToCopy);
std::string coco_string_create(const wchar_t *value);

void coco_string_delete(wchar_t* &value);

// TODO:
/*
#if _MSC_VER
std::wstring widen(const std::string& str) {
	return L"";
}
#endif
*/

// TODO: Check where shared pointers can be replaced with unique ones 

class Token  
{
public:
	int kind;     // token kind
	size_t pos;      // token position in bytes in the source text (starting at 0)
	size_t charPos;  // token position in characters in the source text (starting at 0)
	size_t col;      // token column (starting at 1)
	size_t line;     // token line (starting at 1)
	std::string val; // token value

	// TODO: Can be unique ptr ?
	std::shared_ptr<Token> next;  // ML 2005-03-11 Peek peekedTokens are kept in linked list

	Token();
	~Token() = default;
};

class Buffer {
// This Buffer supports the following cases:
// 1) seekable stream (file)
//    a) whole stream in buffer
//    b) part of stream in buffer
// 2) non seekable stream (network, console)
private:
	std::vector<unsigned char> buf; // input buffer
	size_t bufStart;       // position of first byte in buffer relative to input stream
	size_t fileLen;        // length of input stream (may change if the stream is no file)
	size_t bufPos;         // current position in buffer
	FILE* stream;       // input stream (seekable)
	bool isUserStream;  // was the stream opened by the user?
	
	int ReadNextStreamChunk();
	bool CanSeek();     // true if stream can be seeked otherwise false
	
public:
	static const int EoF = COCO_WCHAR_MAX + 1;

	Buffer(FILE* s, bool isUserStream);
	Buffer(const unsigned char* buf, size_t len);
	Buffer(Buffer* buffer);
	virtual ~Buffer();
	
	virtual void Close();
	virtual int Read();
	virtual int Peek();
	virtual size_t GetPos();
	virtual void SetPos(size_t position);
};

class UTF8Buffer : public Buffer {
public:
	UTF8Buffer(Buffer* buffer) : Buffer(buffer) {};
	virtual int Read();
};

//-----------------------------------------------------------------------------------
// StartStates  -- maps characters to start states of peekedTokens
//-----------------------------------------------------------------------------------
class StartStates {
private:
	class Elem {
	public:
		int key, val;
		std::shared_ptr<Elem> next;

		Elem(int key, int val) { 
			this->key = key;
			this->val = val;
			next = nullptr; 
		}
	};

	const size_t numBuckets = 128;
	std::vector<std::shared_ptr<Elem>> tab;

	size_t getBucketForKey(int key) const {
		return ((unsigned int) key) % numBuckets;
	}

public:
	StartStates() : tab(numBuckets, nullptr) { }
	~StartStates() = default;

	void set(int key, int val) {
		size_t bucketKey = getBucketForKey(key);
		std::shared_ptr<Elem> newTabEntry = std::make_shared<Elem>(key, val);
		newTabEntry->next = tab[bucketKey];
		tab[bucketKey] = newTabEntry;
	}

	int state(int key) {
		std::shared_ptr<Elem> tabEntry = tab[getBucketForKey(key)];
        while (tabEntry != nullptr && tabEntry->key != key) {
            tabEntry = tabEntry->next;
		}

		return tabEntry == nullptr ? 0 : tabEntry->val;
	}
};

//-------------------------------------------------------------------------------------------
// KeywordMap  -- maps strings to integers (identifiers to keyword kinds)
//-------------------------------------------------------------------------------------------
class KeywordMap {
private:
	class Elem {
	public:
		std::string key;
		int val;
		Elem *next;
		Elem(const std::string& key, int val) { this->key = key; this->val = val; next = NULL; }
		virtual ~Elem() {}
	};

	Elem **tab;

public:
	KeywordMap() { tab = new Elem*[128]; memset(tab, 0, 128 * sizeof(Elem*)); }
	virtual ~KeywordMap() {
		for (int i = 0; i < 128; ++i) {
			Elem *e = tab[i];
			while (e != NULL) {
				Elem *next = e->next;
				delete e;
				e = next;
			}
		}
		delete [] tab;
	}

	void set(const wchar_t *key, int val) {
		Elem *e = new Elem(coco_string_create(key), val);
		int k = hashString(coco_string_create(key)) % 128;
		e->next = tab[k]; tab[k] = e;
	}

	int get(const wchar_t *key, int defaultVal) {
		Elem *e = tab[hashString(coco_string_create(key)) % 128];
		// TODO: 
		while (e != NULL && !(std::wstring(e->key.begin(), e->key.end()) == key)) e = e->next;
		return e == NULL ? defaultVal : e->val;
	}
};

class Scanner {
private:
	unsigned char EOL;
	int eofSym;
	int noSym;
	int maxT;
	int charSetSize;
	StartStates start;
	KeywordMap keywords;

	std::shared_ptr<Token> t;         // current token
	std::string tval; // text of current token
	std::size_t tvalLength;   // length of text of current token
	std::size_t tlen;         // length of current token

	std::shared_ptr<Token> peekedTokens;    // list of peekedTokens already peeked (first token is a dummy)
	std::shared_ptr<Token> currPeekedToken;        // current peek token

	int ch;           // current input character

	size_t pos;          // byte position of current character
	size_t charPos;      // position by unicode characters starting with 0
	size_t line;         // line number of current character
	size_t col;          // column number of current character
	int oldEols;      // EOLs that appeared in a comment;

	std::shared_ptr<Token> CreateToken() const;
	void SetScannerBehindT();

	void Init();
	void NextCh();
	void AddCh();
	bool Comment0();

	std::shared_ptr<Token> NextToken();

public:
	std::unique_ptr<Buffer> buffer; // Scanner buffer
	
	Scanner(const unsigned char* buf, size_t bufferSize);
	Scanner(const std::string& fileName);
	Scanner(FILE* s);
	~Scanner() = default;
	std::shared_ptr<Token> Scan();
	std::shared_ptr<Token> Peek();
	void ResetPeek();

}; // end Scanner

} // namespace


#endif

