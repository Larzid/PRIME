// encyclopedia2c.cpp : Defines the entry point for the console application.
//

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace std;

void
writeQuotedString(ostream &os, const char *s)
{
    os << "\"";

    while (*s)
    {
	if (*s == '"')
	{
	    os << "\\\"";
	}
	else if (*s == '\\')
	{
	    os << "\\";
	}
	else
	    os << *s;

	s++;
    }

    os << "\"";
}

class LINE
{
public:
    explicit LINE(const char *line);
    ~LINE();

    LINE	*getNext() const { return myNext; }
    void	 setNext(LINE *line) { myNext = line; }

    const char	*getLine() const { return myLine; }

private:
    LINE	*myNext;
    char	*myLine;
};

LINE::LINE(const char *line)
{
    myNext = 0;
    myLine = strdup(line);
}

LINE::~LINE()
{
    free(myLine);
    delete myNext;
}

class ENTRY
{
public:
    explicit ENTRY(const char *key, const char *name);
    ~ENTRY();

    ENTRY	*getNext() const { return myNext; }
    void	 setNext(ENTRY *next) { myNext = next; }

    const char	*getKey() const { return myKey; }

    // This appends the given text, taking into account
    // merging adjacent lines & implicit paragraphs/
    // returns.
    void	 appendText(const char *text);

    void	 save(ostream &os, ostream &hs) const;

private:
    // This appends the given line as a new LINE structure.
    void	 appendLine(const char *line);

    LINE	*myLines;
    ENTRY	*myNext;
    char	*myKey, *myName;

    char	*myLastLine;
};

ENTRY::ENTRY(const char *key, const char *name)
{
    myKey = strdup(key);
    myName = strdup(name);
    myNext = 0;
    myLines = 0;
    myLastLine = 0;
}

ENTRY::~ENTRY()
{
    free(myKey);
    free(myName);
    delete myLines;
    delete myNext;
}

void
ENTRY::appendLine(const char *txtline)
{
    LINE	*line, *last;

    line = new LINE(txtline);

    for (last = myLines; last && last->getNext(); last = last->getNext());

    if (last)
	last->setNext(line);
    else
	myLines = line;
}

void
ENTRY::appendText(const char *text)
{
    size_t    linelen;

    linelen = strlen(text);

    if (!linelen)
    {
	// We want to do a new paragraph.
	// We thus finish any current line,
	// and append a blank line for
	// good measure.
	if (myLastLine)
	    appendLine(myLastLine);
	appendLine("");
	delete [] myLastLine;
	myLastLine = 0;

	return;
    }
    else
        appendLine(text);
#if 0
    // Append into our last line structure.
    if (myLastLine)
    {
	char	    *newlast;
	char	     lastchar;

	newlast = new char[linelen + strlen(myLastLine) + 5];
	strcpy(newlast, myLastLine);
	if (strlen(newlast))
	{
	    lastchar = newlast[strlen(newlast)-1];
	    // If we didn't have a trailing space, make sure
	    // we add one.  If we had a punctuation, we must
	    // make sure we do two spaces.
	    if (strchr(".!?", lastchar))
	    {
		strcat(newlast, "  ");
	    }
	    else if (lastchar != ' ')
		strcat(newlast, " ");
	}
	strcat(newlast, text);
	delete [] myLastLine;
	myLastLine = newlast;
    }
    else
    {
	myLastLine = new char[linelen+3];
	strcpy(myLastLine, text);
    }

    if (linelen <= 30)
    {
	// This is an implied return.
	appendLine(myLastLine);
	delete [] myLastLine;
	myLastLine = 0;
    }
#endif
}

void
ENTRY::save(ostream &os, ostream &hs) const
{
    int			 numline = 0;
    LINE		*line;

    // Save our line list.
    os << "const char *glb_encyclopedia_entry_" << myKey
       << "_lines[] =" << endl;
    os << "{" << endl;

    // Trim blank lines.
    bool		 hasblank = true;
    LINE		*last;
    while (hasblank)
    {
	hasblank = false;
	last = 0;
	for (line = myLines; line && line->getNext(); line = line->getNext())
	{
	    last = line;
	}

	// See if last line was blank...
	// Can't delete only line.
	if (line && last)
	{
	    if (!strcmp(line->getLine(), ""))
	    {
		hasblank = true;
		last->setNext(0);
		delete line;
	    }
	}
    }

    for (line = myLines; line; line = line->getNext())
    {
	os << "    ";
	writeQuotedString(os, line->getLine());
	if (line->getNext())
	    os << ",";
	os << endl;

	numline++;
    }

    os << "};" << endl;
    os << endl;

    // Now, save the actual entry.
    os << "const encyclopedia_entry glb_encyclopedia_entry_" << myKey
       << " =" << endl;
    os << "{" << endl;
    os << "    " << myKey << "," << endl;
    os << "    ";
    writeQuotedString(os, myName);
    os << "," << endl;
    os << "    " << numline << "," << endl;
    os << "    glb_encyclopedia_entry_" << myKey << "_lines" << endl;
    os << "};" << endl;
    os << endl;
}

class BOOK
{
public:
    explicit BOOK(const char *bookname);
    ~BOOK();

    static BOOK *getBook(const char *name);

    BOOK	*getNext() const { return myNext; }
    void	 setNext(BOOK *book) { myNext = book; }

    const char	*getName() const { return myName; }

    void	 appendEntry(ENTRY *entry);

    void	 save(ostream &os, ostream &hs) const;

private:
    ENTRY	*myEntries;
    char	*myName;
    BOOK	*myNext;
};

BOOK *glb_booklist = 0;

BOOK::BOOK(const char *bookname)
{
    myName = strdup(bookname);
    myEntries = 0;
    myNext = 0;
}

BOOK::~BOOK()
{
    delete myEntries;
    free(myName);
}

BOOK *
BOOK::getBook(const char *name)
{
    BOOK	*book, *last;

    last = 0;
    for (book = glb_booklist; book; book = book->getNext())
    {
	if (!strcmp(book->myName, name))
	    return book;
	last = book;
    }

    // We need a new book.
    book = new BOOK(name);
    if (last)
	last->setNext(book);
    else
	glb_booklist = book;
    return book;
}

void
BOOK::appendEntry(ENTRY *entry)
{
    ENTRY   *last;

    for (last = myEntries; last && last->getNext(); last = last->getNext());

    if (last)
	last->setNext(entry);
    else
	myEntries = entry;
}

void
BOOK::save(ostream &os, ostream &hs) const
{
    int	     numentry;
    ENTRY   *entry;

    numentry = 0;
    for (entry = myEntries; entry; entry = entry->getNext())
    {
	entry->save(os, hs);
	numentry++;
    }

    // Save our list of entry list.
    os << "const encyclopedia_entry *glb_encyclopedia_book_" << myName 
       << "_entrylist[] =" << endl;
    os << "{" << endl;
    for (entry = myEntries; entry; entry = entry->getNext())
    {
	os << "    &glb_encyclopedia_entry_" << entry->getKey();
	if (entry->getNext())
	    os << ",";
	os << endl;
    }
    os << "};" << endl;
    os << endl;

    // Save the book proper.
    os << "const encyclopedia_book glb_encyclopedia_book_" << myName
       << " =" << endl;
    os << "{" << endl;
    os << "    ";
    writeQuotedString(os, myName);
    os << "," << endl;
    os << "    " << numentry << "," << endl;
    os << "    glb_encyclopedia_book_" << myName << "_entrylist" << endl;
    os << "};" << endl;
    os << endl;
}

//
// Stream controls.
// These allow one to undo the fetching of a line.
//
bool glb_putbackline = false;
char raw_input[1024];
char tabless_input[8192];   // Enough for 1k of tabs.

// Removes tabs treating them as mod 8 events.
void
killtabs(char *raw, char *tabless)
{
    int	    off;

    for (off = 0; *raw; raw++)
    {
	if (*raw == '\t')
	{
	    tabless[off++] = ' ';
	    while (off & 7)
		tabless[off++] = ' ';
	}
	else
	{
	    tabless[off++] = *raw;
	}
    }
    tabless[off++] = '\0';
}

bool
input_waiting(ifstream &is)
{
    if (glb_putbackline) return true;

    if (is)
	return true;

    return false;
}

char *
input_getline(ifstream &is)
{
    if (glb_putbackline)
    {
	glb_putbackline = false;
	// Ensure we don't have corrupted input, retab.
	killtabs(raw_input, tabless_input);
	return tabless_input;
    }

    // read a line.
    is.getline(raw_input, 1020);
    killtabs(raw_input, tabless_input);
    return tabless_input;
}

void
input_putbackline()
{
    glb_putbackline = true;
}

int
main(int argc, char* argv[])
{
    if (argc < 2)
    {
	cerr << "Usage:" << endl;
	cerr << "\tencyclopedia2c encyclopedia.txt" << endl;
	cerr << "This will create encyclopedia.cpp and encyclopedia.h" << endl;
	return -1;
    }

    // Constrcut our output file names.
    char *headername, *cppname;

    headername = new char[strlen(argv[1]) + 10];
    cppname = new char[strlen(argv[1]) + 10];
    strcpy(headername, argv[1]);
    strcpy(cppname, argv[1]);
    if (strrchr(headername, '.'))
	*strrchr(headername, '.') = '\0';
    if (strrchr(cppname, '.'))
	*strrchr(cppname, '.') = '\0';

    strcat(headername, ".h");
    strcat(cppname, ".cpp");

    // See if we can open the incoming stream.
    ifstream	    is(argv[1]);

    if (!is)
    {
	cerr << "Unable to open " << argv[1] << " for reading." << endl;
	return -1;
    }

    // Open the output streams.
    ofstream	    os(cppname), hs(headername);

    if (!os)
    {
	cerr << "Unable to open " << cppname << " for writing." << endl;
	return -2;
    }
    if (!hs)
    {
	cerr << "Unable to open " << headername << " for writing." << endl;
	return -3;
    }

    // Standard header information.
    hs << "//" << endl;
    hs << "// " << headername << endl;
    hs << "// Automagically Generated by " << argv[0] << endl;
    hs << "//       DO NOT HAND EDIT." << endl;
    hs << "// (Yes, this does mean you!)" << endl;
    hs << "//" << endl;
    hs << endl;

    os << "//" << endl;
    os << "// " << cppname << endl;
    os << "// Automagically Generated by " << argv[0] << endl;
    os << "//       DO NOT HAND EDIT." << endl;
    os << "// (Yes, this does mean you!)" << endl;
    os << "//" << endl;
    os << endl;
    os << "#include \"Lore.h\"" << endl;
    os << "#include \"ObjectIlk.h\"" << endl;
    os << "#include \"MonsterIlk.h\"" << endl;
    os << endl;

    // Create our data structures.
    hs << "struct encyclopedia_entry" << endl;
    hs << "{" << endl;
    hs << "    int          key;" << endl;
    hs << "    const char  *name;" << endl;
    hs << "    int          numlines;" << endl;
    hs << "    const char **lines;" << endl;
    hs << "};" << endl;
    hs << endl;
    hs << "struct encyclopedia_book" << endl;
    hs << "{" << endl;
    hs << "    const char               *bookname;" << endl;
    hs << "    int                       numentries;" << endl;
    hs << "    const encyclopedia_entry**entries;" << endl;
    hs << "};" << endl;
    hs << endl;

    // Start reading in lines...
    while (input_waiting(is))
    {
	char	    *line;

	line = input_getline(is);
	if (*line == '#')
	{
	    // Ignore comments.
	    continue;
	}


	// Parse out the book/key/entry name.
	char	    bookname[1024], keyname[1024], entryname[1024];
	char	    *readhead;

	// If line is blank, ignore it.
	for (readhead = line; *readhead == ' '; readhead++);
	if (!*readhead)
	    continue;

	readhead = line;
	strcpy(bookname, line);
	if (strchr(bookname, ':'))
	    *strchr(bookname, ':') = '\0';

	for (readhead = line; *readhead; readhead++)
	{
	    if (*readhead == ':' && readhead[1] == ':')
		break;
	}
	if (!*readhead)
	{
	    cerr << "Improper formatted line:" << endl;
	    cerr << line;
	    continue;
	}
	readhead += 2;

	if (*readhead == '-')
	{
	    cerr << "Improper formatted line:" << endl;
	    cerr << line;
	    continue;
	}
	strcpy(keyname, bookname);
	//strcat(keyname, "_");
	strcat(keyname, readhead);
	if (*keyname != '-' && strchr(keyname, '-'))
	    strchr(keyname, '-')[-1] = '\0';

	for (; *readhead; readhead++)
	{
	    if (*readhead == '-' && readhead[-1] == ' ' && readhead[1] == ' ')
		break;
	}
	if (!readhead)
	{
	    cerr << "Improper formatted line:" << endl;
	    cerr << line;
	    continue;
	}
	strcpy(entryname, readhead+2);

	// Create a new entry matching this setup.
	ENTRY	    *entry;
	entry = new ENTRY(keyname, entryname);
	BOOK	    *book;

	book = BOOK::getBook(bookname);
	book->appendEntry(entry);

	// Now add every line that occurs...
	while (input_waiting(is))
	{
	    line = input_getline(is);

	    // Add blank lines.
	    for (readhead = line; *readhead == ' '; readhead++);
	    if (!*readhead)
	    {
		// Append a truly blank line.
		entry->appendText("");
		continue;
	    }

	    // Verify we have 4 spaces.
	    if (line[0] != ' ' &&
		line[1] != ' ' &&
		line[2] != ' ' &&
		line[3] != ' ')
	    {
		// Likely start of another entry.
		input_putbackline();
		break;
	    }

	    // Add the line.
	    entry->appendText(&line[4]);
	}
    }

    // We have now read in all the books: output.
    if (glb_booklist)
    {
	BOOK		*book;
	int		 numbook = 0;

	for (book = glb_booklist; book; book = book->getNext())
	{
	    book->save(os, hs);
	    numbook++;
	}

	// Now, build global list.
	hs << "#define NUM_ENCYCLOPEDIA_BOOKS " << numbook << endl;
	hs << "extern const encyclopedia_book *glb_encyclopedia[NUM_ENCYCLOPEDIA_BOOKS];" << endl;

	os << "const encyclopedia_book *glb_encyclopedia[NUM_ENCYCLOPEDIA_BOOKS] =" << endl;
	os << "{" << endl;

	for (book = glb_booklist; book; book = book->getNext())
	{
	    os << "    &glb_encyclopedia_book_" << book->getName();
	    if (book->getNext())
		os << ",";
	    os << endl;
	}
	os << "};" << endl;
    }

    return 0;
}

