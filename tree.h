
#ifndef __TREE__H__
#define __TREE__H__

#include <vector>
#include <string>
#include <map>

class DataBuffer;

class Tree {
private:
	std::map < std::string, std::string > attributes;
	std::vector < Tree > children;
	std::string tag, data;
	bool forcedata;
public:
	Tree(std::string tag = "");
	Tree(std::string tag, std::map < std::string, std::string > attributes);
	~Tree();

	std::string getData() { return data; }
	std::string getTag() { return tag; }
	std::vector < Tree > getChildren();
	Tree getChild(std::string tag);
	std::map < std::string, std::string > &getAttributes();
	std::string getAttribute(std::string at);

	void setTag(std::string tag) { this->tag = tag; }
	void setAttributes(std::map < std::string, std::string > attributes);
	void setData(const std::string d);
	void setChildren(std::vector < Tree > c);
	void addChild(Tree t);

	void readAttributes(DataBuffer * data, int size);
	void writeAttributes(DataBuffer * data);

	bool hasAttributeValue(std::string at, std::string val);
	bool hasAttribute(std::string at);
	bool hasChild(std::string tag);

	void forceDataWrite();
	bool forcedData() const;

	std::string toString(int sp = 0);
};

#endif

