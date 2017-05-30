#include<iostream>
#include <stdio.h>
#include <libxml/parser.h>
#include <libxml/tree.h>


int main(int argc, char** argv)
{
  xmlDocPtr doc; /* the resulting document tree */

  doc = xmlReadFile("file.xml", NULL, 0);
	std::cout<<"Test"<<std::endl;
	return 0;
}
