#include<iostream>
#include <stdio.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
//#include <glibmm.h>
#include <glibmm/ustring.h>

std::string get_data_dir() { return "/data/dir"; }


int main(int argc, char** argv)
{
  Glib::ustring dataPath;
  dataPath = get_data_dir();
  xmlDocPtr doc; /* the resulting document tree */

  doc = xmlReadFile("file.xml", NULL, 0);
	std::cout<<"Test"<<std::endl;
	return 0;
}
