/**
 * synopsis: Parse a gpx file and delete all dissent elevations
 * purpose: Parse a gpx file and delete all dissent elevations
 * usage: just parse as first argument the xml file or use -h for more help
 * author: Christopher Loessl
 * mail: cloessl@x-berg.de
 * copyright: GPLv2 or later
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#ifdef LIBXML_TREE_ENABLED

#define DEBUG
#define MIN -8000

#ifndef DEBUG
  #define printDebugMsg(msg) ((void)0)
  #define printDebugMsgv(msg) ((void)0)
#else
  #define printDebugMsg(msg) (printDebug(msg))
  #define printDebugMsgv(msg) (printDebugv(msg, __FILE__, __LINE__))
#endif

void printDebugv( const char *msg, const char *file, const int line) {
	fprintf(stderr, "%s : line %d in %s\n", msg, line, file);
}
void printDebug( const char *msg) {
	fprintf(stderr, "%s\n", msg);
}

typedef struct _config {
	int radius;
	int factor;
	int printOutLatLonEle;
} config;

/*
 *To compile this file using gcc you can type
 *gcc `xml2-config --cflags --libs` -o ctoa ctoa.c
 */


/* 
 * addTagValues:
 * @oscptNode: is a Tag-Node to parse and add to SQL Output
 * @nodeID: ID of parent node
 * @i: conter - is this the first run on this node or higher?
 *
 * parses a xml-node representing a tag and returns the Values in SQL INSERT format to stdout
 *
 */
static int
addTagValues(xmlNode *oscptNode, int nodeID, int i) {
	//printf("in addTagValues\n");

	if (i>0) printf(", "); //value blocks seperator

	printf("('%d', '%s', '%s')",
		nodeID,
		xmlGetProp(oscptNode, (const xmlChar *)"k"),
		xmlGetProp(oscptNode, (const xmlChar *)"v")
	);

	return 1;//important! used as counter
}

/* 
 * addNode:
 * @oscptNode: is a Node to parse and add to SQL Output
 *
 * parses a xml-node and returns it in SQL INSERT format to stdout
 *
 */
static int
addNode(xmlNode *oscptNode) {
	int i = 0;	
	xmlNode *tmp_node;

	printf("in addNode\n");

	if ( (!xmlStrcmp(oscptNode->name, (const xmlChar *)"node")) ) 
	{
		int nodeID = atoi(xmlGetProp(oscptNode, (const xmlChar *)"id"));
		//add general node infos
		printf("INSERT INTO nodes (id, lat, lon, visible, user, timestamp) VALUES ('%d', '%s', '%s', NULL, '%s', '%s');\n",
					nodeID,
					xmlGetProp(oscptNode, (const xmlChar *)"lat"),
					xmlGetProp(oscptNode, (const xmlChar *)"lon"),
					xmlGetProp(oscptNode, (const xmlChar *)"user"),
					xmlGetProp(oscptNode, (const xmlChar *)"timestamp")
					); 
	
		//check if childs exists
		if (oscptNode->children) 
		{
			//init tag insert
			printf("INSERT INTO node_tags (id, k, v) VALUES ");
		}

		// Find the childs to parse them
		for (tmp_node = oscptNode->children; tmp_node; tmp_node = tmp_node->next) 
		{
			if ( (!xmlStrcmp(tmp_node->name, (const xmlChar *)"tag")) ) 
				i += addTagValues(tmp_node, nodeID, i);
		}
		if (i>1) printf(";\n"); //new line if there are tag(s)

	} else 
		fprintf(stderr, "Error: not matching Node Type (%s) !\n", oscptNode->name);

	xmlFree(tmp_node);
	return i; //number of tags
}


/* 
 * addWay:
 * @oscptNode: is a Way-Node to parse and add to SQL Output
 *
 * parses a xml-node representing a way and returns it in SQL INSERT format to stdout
 *
 */
static int
addWay(xmlNode *oscptNode) {
	int i = 0;	
	xmlNode *tmp_node;

	printf("in addWay\n");

	if ( (!xmlStrcmp(oscptNode->name, (const xmlChar *)"way")) ) 
	{
		int wayID = atoi(xmlGetProp(oscptNode, (const xmlChar *)"id"));
		//add general node infos
		printf("INSERT INTO ways (id, visible, user, timestamp) VALUES ('%d', NULL, '%s', '%s');\n",
					wayID,
					xmlGetProp(oscptNode, (const xmlChar *)"user"),
					xmlGetProp(oscptNode, (const xmlChar *)"timestamp")
					); 
	
		//check if childs exists
		if (oscptNode->children) 
		{
			//init tag insert
			printf("INSERT INTO way_tags (id, k, v) VALUES ");
		}

		// Find the childs to parse them
		for (tmp_node = oscptNode->children; tmp_node; tmp_node = tmp_node->next) {
			//handle tags
			if ( (!xmlStrcmp(tmp_node->name, (const xmlChar *)"tag")) ) 
				i += addTagValues(tmp_node, wayID, i);
			//TODO handle <nd ref="id">
		}
		if (i>1) printf(";\n"); //new line if there are tag(s)
	} else 
		fprintf(stderr, "Error: not matching Node Type (%s) !\n", oscptNode->name);

	xmlFree(tmp_node);
	return i; //number of tags
}


static int
parseCreate(xmlNode *oscptNode) {

	printf("in parse create\n");

	xmlNode *tmp_node;
	int i = 0;
	if ( (!xmlStrcmp(oscptNode->name, (const xmlChar *)"create")) && (oscptNode->children) ) {
		//create s.th.
		
		// Find the childs to parse them
		for (tmp_node = oscptNode->children; tmp_node; tmp_node = tmp_node->next) {
			i++; // object counter
			if ( (!xmlStrcmp(tmp_node->name, (const xmlChar *)"node")) ) 
				addNode(tmp_node);
			else if ( (!xmlStrcmp(tmp_node->name, (const xmlChar *)"way")) ) 
				addWay(tmp_node);
			//else if ( (!xmlStrcmp(tmp_node->name, (const xmlChar *)"relation")) ) 
				//addRelation(tmp_node);
			else
				fprintf(stderr, "Warning: Found unknown Object (%s) !\n", tmp_node->name);
		}//end Loop

	} else 
		fprintf(stderr, "Error: not matching Operation Type (%s) !\n", oscptNode->name);

	xmlFree(tmp_node);
	return i;
}


/* 
 * deleteNode:
 * @oscptNode: is a Node to parse and add to SQL Output
 *
 * parses a xml-node representing a Node and returns it in SQL DELETE format to stdout
 *
 */
static int
deleteNode(xmlNode *oscptNode) {
	int i = 0;	
	xmlNode *tmp_node;

	printf("in deleteNode\n");

	if ( (!xmlStrcmp(oscptNode->name, (const xmlChar *)"node")) ) 
	{
		int nodeID = atoi(xmlGetProp(oscptNode, (const xmlChar *)"id"));
		//delete general node info
		printf("DELETE FROM nodes WHERE id='%d';\n", nodeID ); //TODO check syntax
	
		//delete tags
		printf("DELETE FROM node_tags WHERE id='%d';\n", nodeID ); //TODO check syntax

		//TODO ways, relations
	} else 
		fprintf(stderr, "Error: not matching Node Type (%s) !\n", oscptNode->name);

	xmlFree(tmp_node);
	return i; //number of tags
}

static int
parseDelete(xmlNode *oscptNode) {

	printf("in parse delete\n");

	xmlNode *tmp_node;
	int i = 0;
	if ( (!xmlStrcmp(oscptNode->name, (const xmlChar *)"delete")) && (oscptNode->children) ) {
		//delete s.th.
		
		// Find the childs to parse them
		for (tmp_node = oscptNode->children; tmp_node; tmp_node = tmp_node->next) {
			if ( (!xmlStrcmp(tmp_node->name, (const xmlChar *)"node")) ) {
				i++;
				deleteNode(tmp_node);
				//TODO way, relation
			} else 
				fprintf(stderr, "Warning: Found unknown Object (%s) !\n", tmp_node->name);
		}//end Loop

	} else 
		fprintf(stderr, "Error: not matching Operation Type (%s) !\n", oscptNode->name);

	xmlFree(tmp_node);
	return i;
}

static int
parseModify(xmlNode *oscptNode) {

	printf("in parse Modify\n");

	xmlNode *tmp_node;
	int i = 0;
	if ( (!xmlStrcmp(oscptNode->name, (const xmlChar *)"modify")) && (oscptNode->children) ) {
		//modify s.th.
		
		// Find the childs to parse them
		for (tmp_node = oscptNode->children; tmp_node; tmp_node = tmp_node->next) {
			if ( (!xmlStrcmp(tmp_node->name, (const xmlChar *)"node")) ) {
				i++;
				//delete first
				deleteNode(tmp_node);
				//create a new one with new values
				addNode(tmp_node);

				//TODO way, relation
			}	else
				fprintf(stderr, "Warning: Found unknown Object (%s) !\n", tmp_node->name);
		}//end Loop

	} else 
		fprintf(stderr, "Error: not matching Operation Type (%s) !\n", oscptNode->name);

	xmlFree(tmp_node);
	return i;
}

/**
 * traverse_tree:
 * @a_node: the initial xml node to consider.
 * @config: config
 *
 * Prints the lat, lon, ele of all the xml elements
 * that are siblings or children of a given xml node.
 */
static void
traverse_tree(xmlNode * a_node, const config *config) {

	xmlNode *cur_node = NULL;
	xmlNode *free_node = NULL;
	int i;

	for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
		//ele = 0;
		/*
		 * XML_ELEMENT_NODE = 1
		 * XML_ATTRIBUTE_NODE = 2
		 * XML_TEXT_NODE = 3
		*/
		//printf("Type: %d  String: %s  name: %s  parent: %s\n", cur_node->type, cur_node->content, cur_node->name, cur_node->parent->name);
		if (cur_node->type == XML_ELEMENT_NODE) {

			if ( (!xmlStrcmp(cur_node->name, (const xmlChar *)"delete")) ) {
				printf("delete it \n");
				parseDelete(cur_node);
			} else if ( (!xmlStrcmp(cur_node->name, (const xmlChar *)"modify")) ) {
				printf("modify it \n");
			} else if ( (!xmlStrcmp(cur_node->name, (const xmlChar *)"create")) ) {
				printf("create it \n");
				parseCreate(cur_node);
			}

		}

		traverse_tree(cur_node->children, config);
	}

	if ( free_node != NULL) {
		xmlUnlinkNode(free_node);
		xmlFreeNode(free_node);
	}

}


/**
 * Simple example
 * walk down the DOM, and print lat, lon, ele
 */
int
main(int argc, char **argv)
{
	config config = {8, 32, 0};
	extern char *optarg;
	extern int optind, opterr, optopt;
	int opt;
	char *infile = NULL, *outfile = NULL;

	xmlNode *root_element;
	xmlDoc *doc = NULL;

	/*
	 * this initialize the library and check potential ABI mismatches
	 * between the version it was compiled for and the actual shared
	 * library used.
	 */
	LIBXML_TEST_VERSION

	while ((opt = getopt(argc, argv, "hi:o:r:f:v")) != -1) {
		switch (opt) {
			case 'h': // help
				fprintf(stderr, "Usage: %s -i <infile> -o <outfile> [-v] [-r radius] [-f factor]\n", argv[0]);
				exit (EXIT_FAILURE);
				break;
			case 'i': // infile
				infile = optarg;
				break;
			case 'o': // outfile
				outfile = optarg;
				break;
			case 'r': // radius
				config.radius = atoi(optarg);
				break;
			case 'f': // factor
				config.factor = atoi(optarg);
				break;
			case 'v':
				config.printOutLatLonEle = 1;
				break;
			default: // ?
				fprintf(stderr, "Usage: %s -i <infile> -o <outfile> [-v] [-r radius] [-f factor]\n", argv[0]);
				exit (EXIT_FAILURE);
				break;
		}
	}

	/*parse the file and get the DOM */
	doc = xmlReadFile(infile, NULL, 0);

	if (doc == NULL) {
		printf("error: could not parse file: \"%s\"\n", infile);
		exit (EXIT_FAILURE);
	}

	/*Get the root element node */
	root_element = xmlDocGetRootElement(doc);

	traverse_tree(root_element, &config);

	/* Save file */
	if ( outfile != NULL ) {
		xmlSaveFile(outfile, doc);
	}

	/*free the document */
	xmlFreeDoc(doc);

	/*
	 *Free the global variables that may
	 *have been allocated by the parser.
	 */
	xmlCleanupParser();

	return 0;
}
#else
int main(void) {
	fprintf(stderr, "Tree support not compiled in\n");
	exit(1);
}
#endif
