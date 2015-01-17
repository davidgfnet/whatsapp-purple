
#include "tree.h"
#include "databuffer.h"

Tree::Tree(std::string tag)
{
	this->tag = tag;
	forcedata = false;
}

Tree::Tree(std::string tag, std::map < std::string, std::string > attributes)
{
	this->tag = tag;
	this->attributes = attributes;
	forcedata = false;
}

Tree::~Tree()
{
}

void Tree::forceDataWrite()
{
	forcedata = true;
}

bool Tree::forcedData() const
{
	return forcedata;
}

void Tree::addChild(Tree t)
{
	children.push_back(t);
}

void Tree::setAttributes(std::map < std::string, std::string > attributes)
{
	this->attributes = attributes;
}

void Tree::readAttributes(DataBuffer * data, int size)
{
	int count = (size - 2 + (size % 2)) / 2;
	while (count--) {
		std::string key = data->readString();
		std::string value = data->readString();
		attributes[key] = value;
	}
}

void Tree::writeAttributes(DataBuffer * data)
{
	for (std::map < std::string, std::string >::iterator iter = attributes.begin(); iter != attributes.end(); iter++) {
		data->putString(iter->first);
		data->putString(iter->second);
	}
}

void Tree::setData(const std::string d)
{
	data = d;
}

void Tree::setChildren(std::vector < Tree > c)
{
	children = c;
}

std::vector < Tree > Tree::getChildren()
{
	return children;
}

std::map < std::string, std::string > &Tree::getAttributes()
{
	return attributes;
}

bool Tree::hasAttributeValue(std::string at, std::string val)
{
	if (hasAttribute(at)) {
		return (attributes[at] == val);
	}
	return false;
}

bool Tree::hasAttribute(std::string at)
{
	return (attributes.find(at) != attributes.end());
}

std::string Tree::getAttribute(std::string at)
{
	if (hasAttribute(at))
		return (attributes[at]);
	return "";
}

bool Tree::getChild(std::string tag, Tree & t)
{
	for (unsigned int i = 0; i < children.size(); i++) {
		if (children[i].getTag() == tag) {
			t = children[i];
			return true;
		}
		if (children[i].getChild(tag, t))
			return true;
	}
	return false;
}

bool Tree::hasChild(std::string tag)
{
	for (unsigned int i = 0; i < children.size(); i++) {
		if (children[i].getTag() == tag)
			return true;
		if (children[i].hasChild(tag))
			return true;
	}
	return false;
}

std::string Tree::toString(int sp)
{
	std::string ret;
	std::string spacing(sp, ' ');
	ret += spacing + "Tag: " + tag + "\n";
	for (std::map < std::string, std::string >::iterator iter = attributes.begin(); iter != attributes.end(); iter++) {
		ret += spacing + "at[" + iter->first + "]=" + iter->second + "\n";
	}
	std::string piece = data.substr(0,10) + " ...";
	ret += spacing + "Data: " + piece + "\n";

	for (unsigned int i = 0; i < children.size(); i++) {
		ret += children[i].toString(sp + 1);
	}
	return ret;
}


