#include "CML.h"
#include <vector>
#include <fstream>

// just to make these multimap expressions a little simpler
using namespace std;

// === CML NODE FUNCTIONALITY ===
// ==============================
std::string CML_node::get_name()
{
 return this->name;
}
CML_node::CML_node(std::string name) : name(name)
{
}

CML_node& CML_node::add_child(std::string name)
{
 CML_node node(name);
 this->children.insert(pair<std::string,CML_node>(name,node));
 return this->children.find(name)->second;
}

void CML_node::set_attribute(std::string key, std::string value)
{
 this->attributes[key] = value;
}

std::string CML_node::get_attribute_or_default(std::string key, std::string def)
{
 std::map<std::string,std::string>::iterator it = attributes.find(key);
 if(it == attributes.end())
 {
  return def;
 }
 else
 {
  return it->second;
 }
}

CML_node& CML_node::get_child(std::string key)
{
 std::multimap<std::string,CML_node>::iterator found = children.find(key);

 if(found != children.end())
 {
  return (found->second);
 }
 else
 {
  // Create a new empty node that'll be inserted.
  return this->add_child(key);
 }
}

std::vector<CML_node*> CML_node::get_children(std::string key)
{
 std::vector<CML_node*> rval;

 // extract all objects in the multimap that have the given key and add them to the rval vector
 pair<multimap<std::string,CML_node>::iterator,multimap<std::string,CML_node>::iterator> range;
 range = children.equal_range(key);
 for (multimap<std::string,CML_node>::iterator it=range.first; it!=range.second; ++it)
 {
  rval.push_back(&(it->second));
 }

 return rval;
}

// === CML PARSER FUNCTIONALITY ===
// ================================

class CMLParserException: public exception
{
public:
 CMLParserException(std::string message)
 {
   this->message = message;
 }
 ~CMLParserException() throw() {};
private:
 std::string message;
 virtual const char* what() const throw()
 {
   return this->message.c_str();
 }
};

// helper struct used to parse internal parsing info around
struct CML_parse_info
{
 // input string we're parsing
 std::string input;
 // position within the text document
 unsigned int pos;
 std::vector<CML_node*> stack;

 // Scan the next char, returns 0 if none
 char next_char()
 {
  if(at_end()) return 0;

  return input[pos];
 }

 // Check if we're at the end of the file
 bool at_end()
 {
  return pos >= input.size();
 }

 // Check if @str is next in the buffer
 bool has_next(std::string str)
 {
  return input.substr(pos, str.size()) == str;
 }

 // Check if @str is next in the buffer, and if so, skip it
 bool consume(std::string str)
 {
  if(has_next(str))
  {
   pos += str.size();
   return true;
  }
  else {
   return false;
  }
 }

 // Skip whitespace, return true if any skipped
 bool skip_whitespace()
 {
  bool rval = false;
  while(next_char() == ' ' || next_char() == '\t')
  {
   pos++;
   rval = true;
  }
  return rval;
 }

 // Skip to end of line
 void skip_to_lineend()
 {
  while(!consume("\n") && !at_end())
  {
   pos++;
  }
 }

 /** Consume a single char that is a valid part of an identifier, aka (A-Za-z0-9_) **/
 bool next_identifier()
 {
  char c = next_char();
  if( (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')
  {
   return true;
  }
  else
  {
   return false;
  }
 }

 void parse_line()
 {
  // skip leading whitespace on the line
  skip_whitespace();

  // check what kind of line this is
  if(next_char() == '#')
  {
   // comment, skip this line
   skip_to_lineend();
   return;
  }
  else if(consume("[/"))
  {
   // closing tag

   // extract the name of the tag
   int identifier_start_pos = pos;
   while(!consume("]") && !at_end())
   {
    pos++;
   }

   std::string tagname = input.substr(identifier_start_pos, pos-identifier_start_pos-1);

   // now try to end the line
   skip_whitespace();
   if(!consume("\n") && !at_end())
   {
    throw CMLParserException("Line-end expected after closing tag.");
   }

   if(tagname != stack.back()->get_name())
   {
    throw CMLParserException("Mis-matching closing tag: Expecting "+stack.back()->get_name()+", but trying to close "+tagname+".");
   }
   stack.pop_back();
   return;
  }
  else if(consume("["))
  {
   // opening tag

   // extract the name of the tag
   int identifier_start_pos = pos;
   while(!consume("]"))
   {
    pos++;
    if(at_end())
    {
     throw CMLParserException("Corrupt opening tag.");
    }
   }

   std::string tagname = input.substr(identifier_start_pos, pos-identifier_start_pos-1);

   // now try to end the line
   skip_whitespace();
   if(!consume("\n") && !at_end())
   {
    throw CMLParserException("Line-end expected after opening tag.");
   }

   CML_node& new_node = stack.back()->add_child(tagname);
   stack.push_back(&new_node);
   return;
  }
  else if(next_identifier())
  {
   // should be a key-value pair
   int identifier_start = pos;
   while(next_identifier())
   {
    pos++;
   }

   std::string key = input.substr(identifier_start, pos-identifier_start);
   skip_whitespace();

   if(!consume("="))
   {
    throw CMLParserException("Expected = after key.");
   }

   skip_whitespace();

   std::string value;

   // check if we're in quote mode
   if(consume("\""))
   {
    // quotation mode
    int value_start = pos; // don't include the quote in the value
    while(!consume("\""))
    {
     pos++;
     if(at_end())
     {
      throw CMLParserException("Unclosed quote.");
     }
    }
    value = input.substr(value_start, pos-value_start-1);
   }
   else
   {
    int value_start = pos;
    // non-quotation mode
    while(!has_next("\n") && !at_end())
    {
     pos++;
    }

    // strip trailing whitespace(can't think of anything better without string utilities)
    value = input.substr(value_start, pos-value_start);
    int last_non_whitespace = value.size()-1;
    while(value[last_non_whitespace] == ' ' || value[last_non_whitespace] == '\t')
    {
     last_non_whitespace--;
     if(last_non_whitespace == -1) break;
    }
    value = value.substr(0, last_non_whitespace+1);
   }

   skip_whitespace();
   // try to end the line
   if(!at_end() && !consume("\n"))
   {
    throw CMLParserException("Expected line-end.");
   }

   stack.back()->set_attribute(key, value);
   return;
  } else
  {
   // probably empty line
   skip_whitespace();
   if(!consume("\n") && !at_end())
   {
    throw CMLParserException("Malformed line.");
   }
  }
 }

};

CML_node CML_parse(std::string string)
{
 CML_parse_info* info = new CML_parse_info();
 CML_node root("root");

 info->pos = 0;
 info->input = string;
 info->stack.push_back(&root);

 /** Parse line by line **/
 while(!info->at_end())
 {
  info->parse_line();
 }

 if(info->stack.size() > 1)
 {
     throw CMLParserException("Missing closing tag for "+info->stack.back()->get_name());
 }

 return root;
}

CML_node CML_load(std::string filename)
{
 std::ifstream t;
 t.open(filename.c_str(), ifstream::in);
 std::string buffer = "";
 std::string line;
 while(t){
  std::getline(t, line);
  buffer += line + "\n";
 }
 t.close();
 return CML_parse(buffer);
}
