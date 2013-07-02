#include <list>
#include <string>
#include <cstring>
#include <iostream>
#include <algorithm>
namespace edn { 
  using std::cout;
  using std::endl;
  using std::string;
  using std::list;

  enum TokenType {
    TokenString,
    TokenAtom,
    TokenParen
  };

  struct EdnToken {
    TokenType type;
    int line;
    string value;
  };

  enum NodeType {
    EdnNil,
    EdnSymbol,
    EdnKeyword,
    EdnBool,
    EdnInt,
    EdnFloat,
    EdnString, 
    EdnChar, 

    EdnList,
    EdnVector,
    EdnMap,
    EdnSet,

    EdnDiscard,
    EdnTagged
  };

  struct EdnNode {
    NodeType type;
    int line;
    string value;
    list<EdnNode> values;
  };

  void createToken(TokenType type, int line, string value, list<EdnToken> &tokens) {
    EdnToken token;
    token.type = type;
    token.line = line;
    token.value = value;
    tokens.push_back(token);
  }


  list<EdnToken> lex(string edn) {
    string::iterator it;
    int line = 1;
    char escapeChar = '\\';
    bool escaping = false;
    bool inString = false;
    string stringContent = "";
    bool inComment = false;
    string token = "";
    string paren = "";
    list<EdnToken> tokens;

    for (it = edn.begin(); it != edn.end(); ++it) {
      if (*it == '\n' || *it == '\r') line++;

      if (!inString && *it == ';' && !escaping) inComment = true;

      if (inComment) {
        if (*it == '\n') {
          inComment = false;
          if (token != "") {
              createToken(TokenAtom, line, token, tokens);
              token = "";
          }
          continue;
        }
      }
    
      if (*it == '"' && !escaping) {
        if (inString) {
          createToken(TokenString, line, stringContent, tokens);
          inString = false;
        } else {
          stringContent = "";
          inString = true;
        }
        continue;
      }

      if (inString) { 
        if (*it == escapeChar && !escaping) {
          escaping = true;
          continue;
        }

        if (escaping) {
          escaping = false;
          if (*it == 't' || *it == 'n' || *it == 'f' || *it == 'r') stringContent += escapeChar;
        }
        stringContent += *it;   
      } else if (*it == '(' || *it == ')' || *it == '[' || *it == ']' || *it == '{' || 
                 *it == '}' || *it == '\t' || *it == '\n' || *it == '\r' || * it == ' ') {
        if (token != "") { 
          createToken(TokenAtom, line, token, tokens);
          token = "";
        } 
        if (*it == '(' || *it == ')' || *it == '[' || *it == ']' || *it == '{' || *it == '}') {
          paren = "";
          paren += *it;
          createToken(TokenParen, line, paren, tokens);
        }
      } else {
        if (escaping) { 
          escaping = false;
        } else if (*it == escapeChar) {
          escaping = true;
        }

        if (token == "#_") {
          createToken(TokenAtom, line, token, tokens); 
          token = "";
        }
        token += *it;
      }
    }
    if (token != "") {
      createToken(TokenAtom, line, token, tokens); 
    }

    return tokens;
  }

  //by default checks if first char is in range of chars
  bool strRangeIn(string str, const char* range, int start = 0, int stop = 1) {
    string strRange = str.substr(start, stop);
    return (std::strspn(strRange.c_str(), range) == strRange.length());
  }

  bool validSymbol(string value) {
    //first we uppercase the value
    
    std::transform(value.begin(), value.end(), value.begin(), ::toupper);
    if (std::strspn(value.c_str(), "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ.*+!-_?$%&=:#/") != value.length())
      return false;
    
    //if the value starts with a number that is not ok
    if (strRangeIn(value, "0123456789"))
      return false;

    //first char can not start with : # or / - but / by itself is valid
    if (strRangeIn(value, ":#/") && !(value.length() == 1 && value[0] == '/'))
      return false;

    //if the first car is - + or . then the next char must NOT be numeric, by by themselves they are valid
    if (strRangeIn(value, "-+.") && value.length() > 1 && strRangeIn(value, "0123456789", 1))
      return false;

    return true;
  }

  bool validKeyword(string value) { 
    return (value[0] == ':' && validSymbol(value.substr(1,value.length() - 1)));
  }

  bool validNil(string value) {
    return (value == "nil");
  }

  bool validBool(string value) {
    return (value == "true" || value == "false");
  }

  bool validInt(string value) {
    if (std::strspn(value.c_str(), "0123456789") != value.length())
      return false;

    return true;
  }

  bool validFloat(string value) {
    return false;
  }

  bool validChar(string value) {
    return (value.at(0) == '\\' && value.length() == 2);
  }

  EdnNode handleAtom(EdnToken token) {
    EdnNode node;
    node.line = token.line;
    node.value = token.value;

    if (validNil(token.value)) { 
      node.type = EdnNil;
    } else if (token.type == TokenString) { 
      node.type = EdnString;
    } else if (validKeyword(token.value)) {
      node.type = EdnKeyword;
    } else if (validSymbol(token.value)) { 
      node.type = EdnSymbol; 
    } else if (validChar(token.value)) {
      node.type = EdnChar;
    } else if (validBool(token.value)) { 
      node.type = EdnBool;
    } else if (validInt(token.value)) {
      node.type = EdnInt;
    } else if (validFloat(token.value)) {
      node.type = EdnFloat;
    } else {
      throw "Could not parse atom ";
    } 

    return node;
  }

  EdnNode handleCollection(EdnToken token, list<EdnNode> values) {
    EdnNode node;
    node.line = token.line;
    node.values = values;

    if (token.value == "(") {
      node.type = EdnList;
    }
    else if (token.value == "[") {
      node.type = EdnVector;
    }
    if (token.value == "{") {
      node.type = EdnMap;
    }
    return node;
  }

  EdnNode handleTagged(EdnToken token, EdnNode value) { 
    EdnNode node;
    node.line = token.line;

    string tagName = token.value.substr(1, token.value.length() - 1);
    if (tagName == "_") {
      node.type = EdnDiscard;
    } else if (tagName == "") {
      //special case where we return early as # { is a set - thus tagname is empty
      node.type = EdnSet;
      if (value.type != EdnMap) {
        throw "Was expection a { } after hash to build set";
      }
      node.values = value.values;
      return node;
    } else {
      node.type = EdnTagged;
    }
  
    if (!validSymbol(tagName)) {
      throw "Invalid tag name"; 
    }

    EdnToken symToken;
    symToken.type = TokenAtom;
    symToken.line = token.line;
    symToken.value = tagName;
  
    list<EdnNode> values;
    values.push_back(handleAtom(symToken)); 
    values.push_back(value);

    node.values = values;
    return node; 
  }

  EdnToken shiftToken(list<EdnToken> &tokens) { 
    EdnToken nextToken = tokens.front();
    tokens.pop_front();
    return nextToken;
  }

  EdnNode readAhead(EdnToken token, list<EdnToken> &tokens) {
    if (token.type == TokenParen) {

      EdnToken nextToken;
      list<EdnNode> L;
      string closeParen;
      if (token.value == "(") closeParen = ")";
      if (token.value == "[") closeParen = "]"; 
      if (token.value == "{") closeParen = "}"; 

      while (true) {
        if (tokens.empty()) throw "unexpected end of list";

        nextToken = shiftToken(tokens);

        if (nextToken.value == closeParen) {
          return handleCollection(token, L);
        } else {
          L.push_back(readAhead(nextToken, tokens));
        }
      }
    } else if (token.value == ")" || token.value == "]" || token.value == "}") {
      throw "Unexpected " + token.value;
    } else {
      if (token.value.at(0) == '#') {
        return handleTagged(token, readAhead(shiftToken(tokens), tokens));
      } else {
        return handleAtom(token);
      }
    }
  }

  string pprint(EdnNode node) {
    string output;
    if (node.type == EdnList || node.type == EdnSet || node.type == EdnVector || node.type == EdnMap) { 
      string vals = "";
      for (list<EdnNode>::iterator it=node.values.begin(); it != node.values.end(); ++it) {
        if (vals.length() > 0) { 
          vals += " ";
        } 
        vals += pprint(*it);
      }

      if (node.type == EdnList) output = "(" + vals + ")";
      else if (node.type == EdnMap) output = "{" + vals + "}";
      else if (node.type == EdnVector) output = "[" + vals + "]"; 
      
    } else if (node.type == EdnTagged) { 
      output = "#" + pprint(node.values.front()) + " " + pprint(node.values.back());
    } else if (node.type == EdnString) {
      output = "\"" + node.value + "\"";
    } else {
      output = "<";
      switch (node.type) { 
        case EdnSymbol: output += "EdnSymbol"; break;
        case EdnKeyword: output += "EdnKeyword"; break;
        case EdnInt: output += "EdnInt"; break;
        case EdnFloat: output += "EdnFloat"; break;
        case EdnChar: output += "EdnChar"; break;
        case EdnBool: output += "EdnBool"; break;
        case EdnNil: output += "EdnNil"; break;
        case EdnString: output += "EdnString"; break;
        case EdnTagged: output += "EdnTagged"; break;
        default: output += "Other"; break;
      }
      output += " " + node.value + ">";
    }
    return output;
  }

  EdnNode read(string edn) {
    list<EdnToken> tokens = lex(edn);
    
    if (tokens.size() == 0) {
      throw "No parsable tokens found in string";
    }

    return readAhead(shiftToken(tokens), tokens);
  }
}
