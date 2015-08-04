#include "config.h"
#include <stdio.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

struct perfmodel_config perfmodel;

static void print_element_names(xmlNode * a_node)
{
	xmlNode *cur_node = NULL;

	for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
		if (cur_node->type == XML_ELEMENT_NODE) {

			//sample
			if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"sample"))) {
					xmlChar *str = xmlGetProp(cur_node, (const xmlChar*)"length");
					perfmodel.sample.length = atoi((char*)str);
					str = xmlGetProp(cur_node, (const xmlChar*)"default_period");
					perfmodel.sample.default_period = atoi((char*)str);
					str = xmlGetProp(cur_node, (const xmlChar*)"initial_cpi");
					perfmodel.sample.initial_cpi = atof((char*)str);
					
					xmlFree(str);
			}

			//core
			if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"core"))) {
					xmlChar *str = xmlGetProp(cur_node, (const xmlChar*)"dispatch_width");
					perfmodel.core.dispatch_width = atoi((char*)str);
					str = xmlGetProp(cur_node, (const xmlChar*)"issue_width");
					perfmodel.core.issue_width = atoi((char*)str);
					str = xmlGetProp(cur_node, (const xmlChar*)"window_size");
					perfmodel.core.window_size = atoi((char*)str);
					xmlFree(str);
			}

			//numfu
			if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"numfu"))) {
					xmlChar *str = xmlGetProp(cur_node,  (const xmlChar *)"intalu");
					perfmodel.numfu.intalu = atoi((char*)str);
					str = xmlGetProp(cur_node,  (const xmlChar *)"intmultdiv");
					perfmodel.numfu.intmultdiv = atoi((char*)str);
					str = xmlGetProp(cur_node,  (const xmlChar *)"mem_rwport");
					perfmodel.numfu.mem_rwport = atoi((char*)str);

					xmlFree(str);
			}

			//latency
			if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"latency"))) {
					xmlChar *str = xmlGetProp(cur_node,  (const xmlChar *)"intalu");
					perfmodel.latency.intalu = atoi((char*)str);
					str = xmlGetProp(cur_node,  (const xmlChar *)"branch");
					perfmodel.latency.branch = atoi((char*)str);
					str = xmlGetProp(cur_node,  (const xmlChar *)"intmult");
					perfmodel.latency.intmult = atoi((char*)str);
					str = xmlGetProp(cur_node,  (const xmlChar *)"intdiv");
					perfmodel.latency.intdiv = atoi((char*)str);
					str = xmlGetProp(cur_node,  (const xmlChar *)"mem");
					perfmodel.latency.mem = atoi((char*)str);
					str = xmlGetProp(cur_node,  (const xmlChar *)"coproc");
					perfmodel.latency.coproc = atoi((char*)str);
					xmlFree(str);
			}

			//etc
			if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"etc"))) {
					xmlChar *str = xmlGetProp(cur_node,  (const xmlChar *)"syscall_penalty");
					perfmodel.etc.syscall_penalty = atoi((char*)str);
					xmlFree(str);
			}


			//dl1_cache
			if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"dl1_cache"))) {
					perfmodel.existDL1 = 1;
					xmlChar *str = xmlGetProp(cur_node,  (const xmlChar *)"perfect");
					perfmodel.dl1.perfect = atoi((char*)str);
					str = xmlGetProp(cur_node,  (const xmlChar *)"num_set");
					perfmodel.dl1.num_set = atoi((char*)str);
					str = xmlGetProp(cur_node,  (const xmlChar *)"size_blk");
					perfmodel.dl1.size_blk = atoi((char*)str);
					str = xmlGetProp(cur_node,  (const xmlChar *)"assoc");
					perfmodel.dl1.assoc = atoi((char*)str);
					str = xmlGetProp(cur_node,  (const xmlChar *)"latency");
					perfmodel.dl1.latency = atoi((char*)str);
					str = xmlGetProp(cur_node,  (const xmlChar *)"replacement_policy");
					strncpy(perfmodel.dl1.replacement_policy, (char*)str, 8);

					xmlFree(str);
			}

			//il1_cache
			if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"il1_cache"))) {
					perfmodel.existIL1 = 1;
					xmlChar *str = xmlGetProp(cur_node,  (const xmlChar *)"perfect");
					perfmodel.il1.perfect = atoi((char*)str);
					str = xmlGetProp(cur_node,  (const xmlChar *)"num_set");
					perfmodel.il1.num_set = atoi((char*)str);
					str = xmlGetProp(cur_node,  (const xmlChar *)"size_blk");
					perfmodel.il1.size_blk = atoi((char*)str);
					str = xmlGetProp(cur_node,  (const xmlChar *)"assoc");
					perfmodel.il1.assoc = atoi((char*)str);
					str = xmlGetProp(cur_node,  (const xmlChar *)"latency");
					perfmodel.il1.latency = atoi((char*)str);
					str = xmlGetProp(cur_node,  (const xmlChar *)"replacement_policy");
					strncpy(perfmodel.il1.replacement_policy, (char*)str, 8);

					xmlFree(str);
			}

			//ul2_cache
			if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"ul2_cache"))) {
					perfmodel.existUL2 = 1;
					xmlChar *str = xmlGetProp(cur_node,  (const xmlChar *)"perfect");
					perfmodel.ul2.perfect = atoi((char*)str);
					str = xmlGetProp(cur_node,  (const xmlChar *)"num_set");
					perfmodel.ul2.num_set = atoi((char*)str);
					str = xmlGetProp(cur_node,  (const xmlChar *)"size_blk");
					perfmodel.ul2.size_blk = atoi((char*)str);
					str = xmlGetProp(cur_node,  (const xmlChar *)"assoc");
					perfmodel.ul2.assoc = atoi((char*)str);
					str = xmlGetProp(cur_node,  (const xmlChar *)"latency");
					perfmodel.ul2.latency = atoi((char*)str);
					str = xmlGetProp(cur_node,  (const xmlChar *)"replacement_policy");
					strncpy(perfmodel.ul2.replacement_policy, (char*)str, 8);

					xmlFree(str);
			}

			//memory
			if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"memory"))) {
					xmlChar *str = xmlGetProp(cur_node,  (const xmlChar *)"latency");
					perfmodel.mem.latency = atoi((char*)str);
					xmlFree(str);
			}


			//bpred
			if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"bpred"))) {
					xmlChar *str = xmlGetProp(cur_node,  (const xmlChar *)"bimod_table_size");
					perfmodel.bpred.bimod_table_size = atoi((char*)str);
					str = xmlGetProp(cur_node,  (const xmlChar *)"mispredict_penalty");
					perfmodel.bpred.mispredict_penalty = atoi((char*)str);
					xmlFree(str);
			}

		}

		print_element_names(cur_node->children);
	}
}


void init(void){
	perfmodel.existDL1 = 0;
	perfmodel.existIL1 = 0;
	perfmodel.existUL2 = 0;

}


int load_configfile(const char* filename){

	init();

	xmlDoc *doc = NULL;
	xmlNode *root_element = NULL;


	/*parse the file and get the DOM */
	doc = xmlReadFile(filename, NULL, 0);

	if (doc == NULL) {
		printf("error: could not parse file %s\n", filename);
		return -1;
	}

	root_element = xmlDocGetRootElement(doc);
	print_element_names(root_element);
	xmlFreeDoc(doc);
	xmlCleanupParser();

	return 0;


}


