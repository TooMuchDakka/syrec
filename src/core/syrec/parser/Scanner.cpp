

#include <memory.h>
#include <string.h>
#include "core/syrec/parser/Scanner.h"

namespace parser {



// string handling, wide character

// TODO:
static unsigned int hashString(const std::string_view& stringToHash) {
	if (stringToHash.empty()) {
        return 0;   
    }

	unsigned int h = 0;
	for (const char& character: stringToHash) {
        if (character == 0) {
            break;
        }
        h = (h * 7) ^ character;
	}
    return h;
}

// TODO:
static std::string coco_string_create(const wchar_t* data) {
	const std::wstring wide_text_as_string(data);
    const std::size_t        size_of_wide_text = wide_text_as_string.size();

    if (size_of_wide_text == 0 || size_of_wide_text == SIZE_MAX) {
        return "";
    }

    char*             test_1 = new char[size_of_wide_text + 1];
    std::size_t       num_characters_converted;
        
    if (!wcstombs_s(&num_characters_converted, test_1, size_of_wide_text + 1, data, size_of_wide_text)) {
        return test_1;
    } else {
        return "";
    }
	return "";
}

// TODO:
wchar_t* coco_string_create(const std::string& data, size_t startPositionInData, size_t numBytesToCopy) {
    std::wstring wString   = std::wstring(data.begin(), data.end());
    wchar_t*     wideStringBuffer = new wchar_t[data.size()];

    wcsncpy(wideStringBuffer, &wString[startPositionInData], numBytesToCopy);
    return wideStringBuffer;
}

void coco_string_delete(wchar_t* &data) {
	delete [] data;
	data = NULL;
}

Token::Token() {
	kind = 0;
	pos  = 0;
	col  = 0;
	line = 0;
	val  = "";
	next = nullptr;
}

Buffer::Buffer(FILE* s, bool isUserStream) {
// ensure binary read on windows
#if _MSC_VER >= 1300
	_setmode(_fileno(s), _O_BINARY);
#endif
	stream = s; 
	this->isUserStream = isUserStream;
	size_t initialBufferLength = COCO_MIN_BUFFER_LENGTH;
	bufStart = 0;
	fileLen = 0;

	if (CanSeek()) {
		fseek(s, 0, SEEK_END);
		fileLen = ftell(s);
		fseek(s, 0, SEEK_SET);
		initialBufferLength = (fileLen < COCO_MAX_BUFFER_LENGTH) ? fileLen : COCO_MAX_BUFFER_LENGTH;
		bufStart = INT_MAX; // nothing in the buffer so far
	}

	buf.resize(initialBufferLength);
	if (fileLen > 0) 
		SetPos(0);          // setup  buffer to position 0 (start)
	else 
		bufPos = 0; // index 0 is already after the file, thus Pos = 0 is invalid

	if (initialBufferLength == fileLen && CanSeek()) 
		Close();
}

Buffer::Buffer(Buffer* buffer) {
	buf = buffer->buf;
	bufStart = buffer->bufStart;
	fileLen = buffer->fileLen;
	bufPos = buffer->bufPos;
	stream = buffer->stream;
	buffer->stream = nullptr;
	isUserStream = buffer->isUserStream;
}

Buffer::Buffer(const unsigned char* buf, size_t len) {
	this->buf.insert(this->buf.end(), buf, buf+len);
	bufStart = 0;
	fileLen = len;
	bufPos = 0;
	stream = nullptr;
}

Buffer::~Buffer() {
	Close(); 
}

void Buffer::Close() {
	if (!isUserStream && stream != NULL) {
		fclose(stream);
		stream = NULL;
	}
}

int Buffer::Read() {
	if (bufPos < this->buf.size()) {
		return buf[bufPos++];
	} else if (GetPos() < fileLen) {
		SetPos(GetPos()); // shift buffer start to Pos
		return buf[bufPos++];
	} else if ((stream != NULL) && !CanSeek() && (ReadNextStreamChunk() > 0)) {
		return buf[bufPos++];
	} else {
		return EoF;
	}
}

int Buffer::Peek() {
	size_t curPos = GetPos();
	int ch = Read();
	SetPos(curPos);
	return ch;
}

size_t Buffer::GetPos() {
	return bufPos + bufStart;
}

void Buffer::SetPos(size_t position) {
	if ((position >= fileLen) && (stream != NULL) && !CanSeek()) {
		// Wanted position is after buffer and the stream
		// is not seek-able e.g. network or console,
		// thus we have to read the stream manually till
		// the wanted position is in sight.
		while ((position >= fileLen) && (ReadNextStreamChunk() > 0));
	}

	if ((position < 0) || (position > fileLen)) {
		wprintf(L"--- buffer out of bounds access, position: %d\n", position);
		exit(1);
	}

	if ((position >= bufStart) && (position < (bufStart + this->buf.size()))) { // already in buffer
		bufPos = position - bufStart;
	} else if (stream != NULL) { // must be swapped in
		if (std::fseek(stream, position ,SEEK_SET)){
			wprintf(L"--- coult not seek to position %d\n", position);
			exit(1);
		}

		std::fread(&this->buf, sizeof this->buf[0], this->buf.capacity(), stream);
		bufStart = position;
		bufPos = 0;
	} else {
		bufPos = fileLen - bufStart; // make Pos return fileLen
	}
}

// Read the next chunk of bytes from the stream, increases the buffer
// if needed and updates the fields fileLen and bufLen.
// Returns the number of bytes read.
int Buffer::ReadNextStreamChunk() {
	size_t free = this->buf.capacity() - this->buf.size();
	if (free == 0) {
		// in the case of a growing input stream
		// we can neither seek in the stream, nor can we
		// foresee the maximum length, thus we must adapt
		// the buffer size on demand.
		this->buf.reserve(this->buf.size() * 2);
		free = this->buf.size() >> 1;
	}

	size_t numBytesRead = std::fread(&this->buf + this->buf.size(), sizeof this->buf[0], free, stream);
	if (numBytesRead > 0) 
		fileLen = this->buf.size();

	return numBytesRead;
}

bool Buffer::CanSeek() {
	return (stream != NULL) && (ftell(stream) != -1);
}

int UTF8Buffer::Read() {
	int ch;
	do {
		ch = Buffer::Read();
		// until we find a utf8 start (0xxxxxxx or 11xxxxxx)
	} while ((ch >= 128) && ((ch & 0xC0) != 0xC0) && (ch != EoF));
	if (ch < 128 || ch == EoF) {
		// nothing to do, first 127 chars are the same in ascii and utf8
		// 0xxxxxxx or end of file character
	} else if ((ch & 0xF0) == 0xF0) {
		// 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
		int c1 = ch & 0x07; ch = Buffer::Read();
		int c2 = ch & 0x3F; ch = Buffer::Read();
		int c3 = ch & 0x3F; ch = Buffer::Read();
		int c4 = ch & 0x3F;
		ch = (((((c1 << 6) | c2) << 6) | c3) << 6) | c4;
	} else if ((ch & 0xE0) == 0xE0) {
		// 1110xxxx 10xxxxxx 10xxxxxx
		int c1 = ch & 0x0F; ch = Buffer::Read();
		int c2 = ch & 0x3F; ch = Buffer::Read();
		int c3 = ch & 0x3F;
		ch = (((c1 << 6) | c2) << 6) | c3;
	} else if ((ch & 0xC0) == 0xC0) {
		// 110xxxxx 10xxxxxx
		int c1 = ch & 0x1F; ch = Buffer::Read();
		int c2 = ch & 0x3F;
		ch = (c1 << 6) | c2;
	}
	return ch;
}

Scanner::Scanner(const unsigned char* buf, size_t len) {
	buffer = std::make_unique<Buffer>(buf, len);
	Init();
}

Scanner::Scanner(const std::string& fileName) {
	/*
	FILE* stream;
	// TODO: https://stackoverflow.com/questions/5850358/is-there-a-preprocessor-define-that-is-defined-if-the-compiler-is-msvc
	#if _MSC_VER
		stream = fopen(widen(fileName, "rb"));
	#else
		stream = fopen(fileName, "rb");
	#endif
	*/

	FILE* stream = fopen(fileName.c_str(), "rb");
	if (!stream) {
		// TODO:
		//wprintf(L"--- Cannot open file %s\n", fileName);
		exit(1);
	}
	buffer = std::make_unique<Buffer>(stream, false);
	Init();
}

Scanner::Scanner(FILE* s) {
	buffer = std::make_unique<Buffer>(s, true);
	Init();
}

void Scanner::Init() {
	EOL    = '\n';
	eofSym = 0;
	maxT = 55;
	noSym = 55;
	int i;
	for (i = 65; i <= 90; ++i) start.set(i, 1);
	for (i = 95; i <= 95; ++i) start.set(i, 1);
	for (i = 97; i <= 122; ++i) start.set(i, 1);
	for (i = 48; i <= 57; ++i) start.set(i, 2);
	start.set(35, 3);
	start.set(36, 4);
	start.set(40, 5);
	start.set(43, 28);
	start.set(45, 29);
	start.set(42, 30);
	start.set(47, 6);
	start.set(41, 7);
	start.set(44, 8);
	start.set(91, 9);
	start.set(93, 10);
	start.set(59, 11);
	start.set(61, 12);
	start.set(126, 13);
	start.set(94, 16);
	start.set(60, 31);
	start.set(46, 18);
	start.set(58, 19);
	start.set(37, 20);
	start.set(38, 32);
	start.set(124, 33);
	start.set(62, 34);
	start.set(33, 35);
		start.set(Buffer::EoF, -1);
	keywords.set(L"module", 11);
	keywords.set(L"in", 13);
	keywords.set(L"out", 14);
	keywords.set(L"inout", 15);
	keywords.set(L"wire", 16);
	keywords.set(L"state", 17);
	keywords.set(L"call", 21);
	keywords.set(L"uncall", 22);
	keywords.set(L"for", 23);
	keywords.set(L"to", 25);
	keywords.set(L"step", 26);
	keywords.set(L"do", 27);
	keywords.set(L"rof", 28);
	keywords.set(L"if", 29);
	keywords.set(L"then", 30);
	keywords.set(L"else", 31);
	keywords.set(L"fi", 32);
	keywords.set(L"skip", 38);


	tvalLength = 128;
	tval.assign("", tvalLength);

	// TODO:
	//pos = -1; line = 1; col = 0; charPos = -1;
	pos = 0; 
	line = 1;
	col = 0;
	charPos = 0;

	oldEols = 0;
	NextCh();
	if (ch == 0xEF) { // check optional byte order mark for UTF-8
		NextCh(); int ch1 = ch;
		NextCh(); int ch2 = ch;
		if (ch1 != 0xBB || ch2 != 0xBF) {
			wprintf(L"Illegal byte order mark at start of file");
			exit(1);
		}

		// TODO: Rework ugly &* parameter
		buffer = std::make_unique<UTF8Buffer>(&*buffer);
		col = 0;

		// TODO:
		//charPos = -1;
		charPos = 0;
		NextCh();
	}


	currPeekedToken = peekedTokens = CreateToken(); // first token is a dummy
}

void Scanner::NextCh() {
	if (oldEols > 0) { ch = EOL; oldEols--; }
	else {
		pos = buffer->GetPos();
		// buffer reads unicode chars, if UTF8 has been detected
		ch = buffer->Read(); col++; charPos++;
		// replace isolated '\r' by '\n' in order to make
		// eol handling uniform across Windows, Unix and Mac
		if (ch == L'\r' && buffer->Peek() != L'\n') ch = EOL;
		if (ch == EOL) { line++; col = 0; }
	}

}

void Scanner::AddCh() {
	if (tlen >= tvalLength) {
		tvalLength *= 2;
		tval.resize(tvalLength);
	}
	if (ch != Buffer::EoF) {
		/*
		tval[tlen++] = ch;
		*/
		// TODO:
		tval[tlen++] = static_cast<char>(ch);
		NextCh();
	}
}


bool Scanner::Comment0() {
	int level = 1, pos0 = pos, line0 = line, col0 = col, charPos0 = charPos;
	NextCh();
	if (ch == L'/') {
		NextCh();
		for(;;) {
			if (ch == 10) {
				level--;
				if (level == 0) { oldEols = line - line0; NextCh(); return true; }
				NextCh();
			} else if (ch == buffer->EoF) return false;
			else NextCh();
		}
	} else {
		buffer->SetPos(pos0); NextCh(); line = line0; col = col0; charPos = charPos0;
	}
	return false;
}


std::shared_ptr<Token> Scanner::CreateToken() const {
	auto token = std::make_shared<Token>();
	token->val = "";
	token->next = nullptr;
	return token;
}

std::shared_ptr<Token> Scanner::NextToken() {
	while (ch == ' ' ||
			(ch >= 9 && ch <= 10) || ch == 13
	) NextCh();
	if ((ch == L'/' && Comment0())) return NextToken();
	int recKind = noSym;
	int recEnd = pos;

	t = CreateToken();
	t->pos = pos; t->col = col; t->line = line; t->charPos = charPos;
	int state = start.state(ch);
	tlen = 0; AddCh();

	switch (state) {
		case -1: { t->kind = eofSym; break; } // NextCh already done
		case 0: {
			case_0:
			if (recKind != noSym) {
				tlen = recEnd - t->pos;
				SetScannerBehindT();
			}
			t->kind = recKind; break;
		} // NextCh already done
		case 1:
			case_1:
			recEnd = pos; recKind = 1;
			if ((ch >= L'0' && ch <= L'9') || (ch >= L'A' && ch <= L'Z') || ch == L'_' || (ch >= L'a' && ch <= L'z')) {AddCh(); goto case_1;}
			else {t->kind = 1; wchar_t *literal = coco_string_create(tval, 0, tlen); t->kind = keywords.get(literal, t->kind); coco_string_delete(literal); break;}
		case 2:
			case_2:
			recEnd = pos; recKind = 2;
			if ((ch >= L'0' && ch <= L'9')) {AddCh(); goto case_2;}
			else {t->kind = 2; break;}
		case 3:
			{t->kind = 3; break;}
		case 4:
			{t->kind = 4; break;}
		case 5:
			{t->kind = 5; break;}
		case 6:
			{t->kind = 9; break;}
		case 7:
			{t->kind = 10; break;}
		case 8:
			{t->kind = 12; break;}
		case 9:
			{t->kind = 18; break;}
		case 10:
			{t->kind = 19; break;}
		case 11:
			{t->kind = 20; break;}
		case 12:
			{t->kind = 24; break;}
		case 13:
			{t->kind = 33; break;}
		case 14:
			case_14:
			{t->kind = 34; break;}
		case 15:
			case_15:
			{t->kind = 35; break;}
		case 16:
			{t->kind = 36; break;}
		case 17:
			case_17:
			{t->kind = 37; break;}
		case 18:
			{t->kind = 39; break;}
		case 19:
			{t->kind = 40; break;}
		case 20:
			{t->kind = 41; break;}
		case 21:
			case_21:
			{t->kind = 42; break;}
		case 22:
			case_22:
			{t->kind = 43; break;}
		case 23:
			case_23:
			{t->kind = 44; break;}
		case 24:
			case_24:
			{t->kind = 49; break;}
		case 25:
			case_25:
			{t->kind = 51; break;}
		case 26:
			case_26:
			{t->kind = 53; break;}
		case 27:
			case_27:
			{t->kind = 54; break;}
		case 28:
			recEnd = pos; recKind = 6;
			if (ch == L'+') {AddCh(); goto case_14;}
			else {t->kind = 6; break;}
		case 29:
			recEnd = pos; recKind = 7;
			if (ch == L'-') {AddCh(); goto case_15;}
			else {t->kind = 7; break;}
		case 30:
			recEnd = pos; recKind = 8;
			if (ch == L'>') {AddCh(); goto case_21;}
			else {t->kind = 8; break;}
		case 31:
			recEnd = pos; recKind = 47;
			if (ch == L'=') {AddCh(); goto case_36;}
			else if (ch == L'<') {AddCh(); goto case_26;}
			else {t->kind = 47; break;}
		case 32:
			recEnd = pos; recKind = 45;
			if (ch == L'&') {AddCh(); goto case_22;}
			else {t->kind = 45; break;}
		case 33:
			recEnd = pos; recKind = 46;
			if (ch == L'|') {AddCh(); goto case_23;}
			else {t->kind = 46; break;}
		case 34:
			recEnd = pos; recKind = 48;
			if (ch == L'=') {AddCh(); goto case_25;}
			else if (ch == L'>') {AddCh(); goto case_27;}
			else {t->kind = 48; break;}
		case 35:
			recEnd = pos; recKind = 52;
			if (ch == L'=') {AddCh(); goto case_24;}
			else {t->kind = 52; break;}
		case 36:
			case_36:
			recEnd = pos; recKind = 50;
			if (ch == L'>') {AddCh(); goto case_17;}
			else {t->kind = 50; break;}

	}

	t->val.append(tval);
	return t;
}

void Scanner::SetScannerBehindT() {
	buffer->SetPos(t->pos);
	NextCh();
	line = t->line; col = t->col; charPos = t->charPos;
	for (int i = 0; i < tlen; i++) NextCh();
}

// get the next token (possibly a token already seen during peeking)
std::shared_ptr<Token> Scanner::Scan() {
	if (peekedTokens->next == nullptr) {
		return currPeekedToken = peekedTokens = NextToken();
	} else {
		currPeekedToken = peekedTokens = peekedTokens->next;
		return peekedTokens;
	}
}

// peek for the next token, ignore pragmas
std::shared_ptr<Token> Scanner::Peek() {
	do {
		if (currPeekedToken->next == nullptr) {
			currPeekedToken->next = NextToken();
		}
		currPeekedToken = currPeekedToken->next;
	} while (currPeekedToken->kind > maxT); // skip pragmas

	return currPeekedToken;
}

// make sure that peeking starts at the current scan position
void Scanner::ResetPeek() {
	currPeekedToken = peekedTokens;
}

} // namespace

