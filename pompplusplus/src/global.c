#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "access_memory.h"
 #include "global.h"
#include "elf_core.h"
#include "elf_binary.h"
#include "reverse_log.h"
#include "insthandler_arm.h"
#include "disassemble.h"
char *core_path;
char *bin_path; 
char *inst_path; 
char *memac_path;
char *log_path;
char *region_path;
char *heu_path;
// elf_core_info *core_info;
elf_binary_info *binary_info;
int max_rev_ins_num;
int root_cause_rev_idx;

void set_core_path(char *path){
	core_path = path;
}

void set_bin_path(char *path){
    bin_path = path;
}

void set_inst_path(char *path){
    inst_path = path;
}

void set_memac_path(char *path){
    memac_path = path;
}

void set_log_path(char *path){
    log_path = path;
}
void set_heu_path(char *path){
    heu_path = path;
}

void set_region_path(char *path){
    region_path = path;
}

void set_max_rev_ins_num(int num){
    max_rev_ins_num = num;
}

void set_root_cause_rev_idx(int idx){
    root_cause_rev_idx = idx;
}

char *get_core_path(void){
    return core_path;
}

char *get_bin_path(void){
    return bin_path;
}

char *get_inst_path(void){
    return inst_path;
}

char *get_memac_path(void){
    return memac_path;
}

char *get_log_path(void){
    return log_path;
}

char *get_region_path(void){
    return region_path;
}

int get_max_rev_ins_num(void){
    return max_rev_ins_num;
}

int get_root_cause_rev_idx(void){
    return root_cause_rev_idx;
}
// void set_core_info(elf_core_info *coreinfo){
//     core_info = coreinfo;
// }

void set_bin_info(elf_binary_info *binaryinfo){
    binary_info = binaryinfo;
}

// elf_core_info *get_core_info(void){
//     return core_info;
// }

elf_binary_info *get_bin_info(void){
    return binary_info;
}

// void set_input_data(char *path){
// 	int length = strlen(path);
// 	int new_len = length + 20;

// 	memset(&input_data, 0, sizeof(input_st));
	
// 	input_data.case_path = (char *) malloc(new_len);
// 	memset(input_data.case_path, 0, new_len);
// 	sprintf(input_data.case_path, "%s", path);

// 	input_data.core_path = (char *) malloc(new_len);
// 	memset(input_data.core_path, 0, new_len);
// 	sprintf(input_data.core_path, "%s%s", path, "core");
	
// 	input_data.inst_path = (char *) malloc(new_len);
// 	memset(input_data.inst_path, 0, new_len);
// 	sprintf(input_data.inst_path, "%s%s", path, "inst");
	
// 	input_data.libs_path = (char *) malloc(new_len);
// 	memset(input_data.libs_path, 0, new_len);
// 	sprintf(input_data.libs_path, "%s%s", path, "libs/");
	
// 	input_data.log_path  = (char *) malloc(new_len);
// 	memset(input_data.log_path, 0, new_len);
// 	sprintf(input_data.log_path, "%s%s", path, "regs");
	
// 	input_data.xmm_path  = (char *) malloc(new_len);
// 	memset(input_data.xmm_path, 0, new_len);
// 	sprintf(input_data.xmm_path, "%s%s", path, "xmm.log");
	
// 	input_data.bin_path  = (char *) malloc(new_len);
// 	memset(input_data.bin_path, 0, new_len);
// 	sprintf(input_data.bin_path, "%s%s", path, "binary");
// /*	
// 	input_data.memop_path = (char *) malloc(new_len);
// 	sprintf(input_data.memop_path, "%s%s", path, "memop");
// */	
// 	input_data.region_path = (char *) malloc(new_len);
// 	memset(input_data.region_path, 0, new_len);
// 	sprintf(input_data.region_path, "%s%s", path, "region_DL");
	
// #if defined (ALIAS_MODULE) && (ALIAS_MODULE == VSA_MODULE)
// 	input_data.heu_path = (char *) malloc(new_len);
// 	memset(input_data.heu_path, 0, new_len);
// 	sprintf(input_data.heu_path, "%s%s", path, "vsa_heuristic");
// #endif
// }

// void clean_input_data(input_st input_data) {

// 	if (input_data.case_path) {
// 		free(input_data.case_path);
// 		input_data.case_path = NULL;
// 	}

// 	if (input_data.core_path) {
// 		free(input_data.core_path);
// 		input_data.core_path = NULL;
// 	}

// 	if (input_data.inst_path) {
// 		free(input_data.inst_path);
// 		input_data.inst_path = NULL;
// 	}

// 	if (input_data.libs_path) {
// 		free(input_data.libs_path);
// 		input_data.libs_path = NULL;
// 	}

// 	if (input_data.log_path) {
// 		free(input_data.log_path);
// 		input_data.log_path = NULL;
// 	}

// 	if (input_data.xmm_path) {
// 		free(input_data.xmm_path);
// 		input_data.xmm_path = NULL;
// 	}

// 	if (input_data.bin_path) {
// 		free(input_data.bin_path);
// 		input_data.bin_path = NULL;
// 	}
// /*
// 	if (input_data.memop_path) {
// 		free(input_data.memop_path);
// 		input_data.memop_path = NULL;
// 	}
// */
// 	if (input_data.region_path) {
// 		free(input_data.region_path);
// 		input_data.region_path = NULL;
// 	}

// #if defined (ALIAS_MODULE) && (ALIAS_MODULE == VSA_MODULE)
// 	if (input_data.heu_path) {
// 		free(input_data.heu_path);
// 		input_data.heu_path = NULL;
// 	}
// #endif
// }

// char * get_core_path() {
// 	return input_data.core_path;	
// }

// char * get_inst_path() {
// 	return input_data.inst_path;	
// }

// char * get_libs_path() {
// 	return input_data.libs_path;	
// }

// char * get_log_path() {
// 	return input_data.log_path;	
// }

// char * get_xmm_path() {
// 	return input_data.xmm_path;	
// }

// char * get_bin_path() {
// 	return input_data.bin_path;	
// }
// /*
// char * get_memop_path() {
// 	return input_data.memop_path;	
// }
// */
// char * get_region_path() {
// 	return input_data.region_path;	
// }

#if defined (ALIAS_MODULE) && (ALIAS_MODULE == VSA_MODULE)
char * get_heu_path() {
	return heu_path;	
}
#endif
