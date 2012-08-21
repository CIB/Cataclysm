#ifndef _CML_H
#define _CML_H

#include <vector>
#include <map>
#include <string>

/** This module defines a simple configuration language which I dub CML(cataclysm markup language).
 *  It's heavily inspired by Wesnoth's WML.
 *
 *  Example syntax:
 *  [foo]
 *    key=value
 *    [sub]
 *      desc="example string with trailing whitespace   "
 *    [/sub]
 *    [sub]
 *      desc="more than one of the same tag will be treated as array"
 *    [/sub]
 *  [/foo]
 *
 *  Simple rules:
 *  - Tags can be any combination of alphabetic characters, numbers and underscores(ASCII).
 *  - The same goes for keys.
 *  - Values can be any valid string, trailing and leading whitespace will be stripped.
 *  - Values can be quoted, but the only effect of this is that leading/trailing whitespace within the quotes will be parsed.
 *  - Values other than strings will be represented as string, and parsed by user-code into something else.
 *  - Multiple tags of the same name will be treated as array of elements.
**/

struct CML_node
{
private:
 const std::string name;
 std::multimap<std::string,CML_node> children;
 std::map<std::string,std::string> attributes;

public:
 CML_node(std::string name);

 /** Returns the name of this element. **/
 std::string get_name();

 /** Adds a child element to this element. Returns a reference to the created child**/
 CML_node& add_child(std::string name);

 /** Sets the string attribute of the specified key. **/
 void set_attribute(std::string key, std::string value);

 /** returns true if at least one child with the given key exists, false otherwise. **/
 bool has_child(std::string key);

 /** Gets a single child node with the given key. If there's more than one, just gets the first and ignores the rest.
  *
  *  If there is no child node with the given key, this will create an empty one and return it. This makes reading out default
  *  values of nodes that don't exist a little easier.**/
 CML_node& get_child(std::string key);

 /** Gets a list of children with the given key. **/
 std::vector<CML_node*> get_children(std::string key);

 /** Gets the attribute value of the specified key, or the default if not present. **/
 std::string get_attribute_or_default(std::string key, std::string def);
};

/** Parse a string into a CML tree. **/
CML_node CML_parse(std::string string);

/** Loads a CML tree from a file. **/
CML_node CML_load(std::string filename);

// Convenience typedef's to make iteration a bit more readable
typedef std::vector<CML_node*>::iterator CML_child_iter;
typedef std::vector<CML_node*> CML_children;

#endif // #ifndef _CML_H
