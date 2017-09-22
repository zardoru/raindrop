#include "pch.h"


#include "Logging.h"
#include "GameState.h"

using std::string;
using std::map;
using std::vector;
using std::shared_ptr;
using std::runtime_error;
using std::make_shared;

typedef map<string, string> symbolVal;
typedef map<string, symbolVal> symbolMap;
typedef shared_ptr<vector<string>> tokenList;

symbolMap Values;

class cfgMap
{
private:
    int line; int linepos;
    string input;
    size_t offset;
    tokenList tokout;

    void incoffs()
    {
        if (input[offset] == '\n')
        {
            line++; linepos = 0;
        }

        offset++;
        linepos++;
    }

    // assert we didn't get to the eof
    void assertnoteof()
    {
        if (offset >= input.length())
            throw syntax_error("unexpected eof", line, linepos);
    }

    // see if C is a token
    bool istok(char C)
    {
        char tokens[] = ",;{}:";
        for (size_t i = 0; i < sizeof(tokens); i++)
            if (C == tokens[i])
                return true;

        return false;
    }

    // skip whitespace
    void skipws()
    {
        while (isspace(input[offset]) && offset < input.length())
            incoffs();
    }

    // read a name
    void readid()
    {
        string tok;
        skipws();

        if (offset >= input.length() || istok(input[offset])) throw syntax_error("expected identifier", line, linepos);

        while (offset < input.length() && !isspace(input[offset]) && !istok(input[offset]))
        {
            tok += input[offset];
            incoffs();
        }

        tokout->push_back(tok);
    }

    // get to the next non-whitespace that equals tok
    void match(char tok)
    {
        skipws();
        if (tok != input[offset])
            throw syntax_error(string("expected ") + tok, line, linepos);

        incoffs();
        char s[2];
        s[0] = tok; s[1] = 0;
        tokout->push_back(s);
    }

    void readstmt()
    {
        // xx: yy;
        skipws();
        while (input[offset] != '}')
        {
            readid(); 
			while (input[offset] == ',') {
				offset += 1;
				readid();
			}
			
			match(':'); readid(); match(';');
            skipws();
        }
    }

public:

    cfgMap() {}

    class syntax_error : public runtime_error
    {
    public:
        int line; int offs;
        syntax_error(string err, int ln, int lnoff) : runtime_error(err)
        {
            line = ln; offs = lnoff;
        }
    };

    void tokenize(string input)
    {
        this->input = input;
        offset = 0;
        line = 1; linepos = 1;
        tokout = make_shared<vector<string>>();

        while (offset < input.length())
        {
            readid();
            match('{');
            assertnoteof();
            readstmt(); // this throws by itself.
            assertnoteof();
            match('}');
            skipws();
        }
    }

    void getMap(symbolMap &out)
    {
        for (auto it = tokout->begin(); it != tokout->end(); it++)
        {
			vector<string> sym;
			sym.push_back(*it);
            it++; 
			while (*it != "{") { // the commas are ommitted
				sym.push_back(*it);
			}

			it++; // skip {
            while (*it != "}")
            {
                string key = *it; it++; it++; // skip :
                string val = *it; it++; it++; // skip ;

				for (auto &id: sym)
					out[id][key] = val;
            }
        }
    }
};

namespace Configuration
{
    void LoadTextureParameters()
    {
        std::ifstream istr(GameState::GetInstance().GetSkinFile("texparams.rcf").string());
        std::string inp, line;

        if (!istr.is_open())
        {
            Log::Printf("Couldn't open texparams.rcf.\n");
            return;
        }

        while (std::getline(istr, line))
            inp += line;

        try
        {
            cfgMap Map;
            Map.tokenize(inp);
            Map.getMap(Values);
        }
        catch (cfgMap::syntax_error &err)
        {
            Log::Printf("Syntax Error (Line %d@%d): %s\n", err.line, err.offs, err.what());
        }
    }

    bool HasTextureParameters(std::string filename)
    {
        return Values.find(filename) != Values.end();
    }

    std::string GetTextureParameter(std::string filename, std::string parameter)
    {
        return Values[filename][parameter];
    }

    bool TextureParameterExists(std::string filename, std::string parameter)
    {
        return Values[filename].find(parameter) != Values[filename].end();
    }
}
