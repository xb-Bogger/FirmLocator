#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "global.h"
#include "disassemble.h"
#include "insthandler_arm.h"
#include "reverse_exe.h"
#include "inst_opd.h"
#include "reverse_log.h"
#include "re_alias.h"
#include <setjmp.h>
#ifdef VSA
#include "bin_alias.h"
#include "common.h"
#include "re_runtime.h"
#endif


#define REPLACE_HEAD(oldhead, newhead) \
	(oldhead)->next->prev = newhead;\
	(oldhead)->prev->next = newhead; 

static bool member_in_umemlist(re_list_t *umem){

	re_list_t* entry;

	list_for_each_entry(entry, &re_ds.head.umemlist, umemlist){
		if(entry == umem)
			return true;
	}

	return false;
}


static bool member_in_alias_umemlist(re_list_t *umem){

	re_list_t* entry;

	list_for_each_entry(entry, &re_ds.aliashead.umemlist, umemlist){
		if(entry == umem)
			return true;
	}

	return false;
}


void add_to_umemlist(re_list_t * exp){

	if(!member_in_umemlist(exp))
		list_add(&exp->umemlist, &re_ds.head.umemlist);
}


void remove_from_umemlist(re_list_t* exp){

	if(member_in_umemlist(exp))
		list_del(&exp->umemlist);
}


void add_to_alias_umemlist(re_list_t * exp){

	if(!member_in_alias_umemlist(exp))
		list_add(&exp->umemlist, &re_ds.aliashead.umemlist);
}


void remove_from_alias_umemlist(re_list_t* exp){
	if(member_in_alias_umemlist(exp))
		list_del(&exp->umemlist);
}

//check if there is any unknown memory write cannot be resolved between two targets
#ifdef VSA
bool alias_between_two_targets(re_list_t *entry, re_list_t *target){

	if(!resolve_alias(target, entry))
		return true;

	return false;	
}
#else
bool alias_between_two_targets(re_list_t *entry, re_list_t *target){

	int index;  

	assert(check_next_unknown_write(&re_ds.head, entry, target));

	if(!resolve_alias(target, entry))
		return true;

	return false; 		
}
#endif

bool obstacle_between_two_targets(re_list_t *listhead, re_list_t* entry, re_list_t *target){
	if (!check_next_unknown_write(listhead, entry, target))
		return false; 
#ifndef VSA
// FirmRCA dose this make sense?
	// if(!re_ds.resolving)
	// 	return true; 
#endif
  	return alias_between_two_targets(entry, target);	
} 

//target is the node added later
bool check_next_unknown_write(re_list_t *listhead, re_list_t *def, re_list_t *target){

	//check if there is any unknown write between def and target

	re_list_t* entry;
	re_list_t* head; 

	head = def ? def : listhead; 

	list_for_each_entry(entry, &target->list, list){
		if(entry == head)
			break;
			
		if(entry->node_type == DefNode && node_is_exp(entry, false) && !CAST2_DEF(entry->node)->address)
			return true;
	} 

	return false;
}

static void assign_elements_of_address(re_list_t* exp1, re_list_t* exp2, re_list_t* uselist){

	valset_u vt; 
	unsigned address;
	cs_arm_op *opd;
	re_list_t * index1, *index2, *base1, *base2; 

	get_element_of_exp(exp1, &index1, &base1);
	get_element_of_exp(exp2, &index2, &base2);

//exp2 is the expression with known address 
	assert( index2 == NULL || CAST2_USE(index2->node)->val_known);
	assert( base2 == NULL || CAST2_USE(base2->node)->val_known);	

	address = exp2->node_type == UseNode ?  CAST2_USE(exp2->node)->address : CAST2_DEF(exp2->node)->address; 

	CAST2_DEF(exp1->node)->address = address; 

	switch(exp_addr_status(base1, index1)){

		case KBaseKIndex:	
			break;

		case UBaseUIndex:
			break;

		case UBase:
			vt.dword = address - CAST2_USE(base1->node)->operand->mem.disp;	
			assign_use_value(base1, vt);
			add_to_uselist(base1, uselist);
			break;

		case UIndex:
			opd = CAST2_USE(index1->node)->operand;
			vt.dword = address - opd->mem.disp;
			assign_use_value(index1, vt);
			add_to_uselist(index1, uselist);

			break;

		case UBaseKIndex:
			opd = CAST2_USE(index1->node)->operand;
			vt.dword = address - CAST2_USE(index1->node)->val.dword  -
				opd->mem.disp;
			assign_use_value(base1, vt);
			add_to_uselist(base1, uselist);
			break;

		case KBaseUIndex:
			opd = CAST2_USE(index1->node)->operand;
			vt.dword = address - CAST2_USE(base1->node)->val.dword - opd->mem.disp;
			assign_use_value(index1, vt);
			add_to_uselist(index1, uselist);
			break;

		default:
			assert("Impossible" && 0);
			break;
	}
}

bool re_alias_resolve(re_list_t* exp1, re_list_t* exp2){

#ifdef VERBOSE
	LOG(stdout, "re_mem_alias.c/re_alias_resolve: exp1 = [ ");
	print_node_operand(exp1);
	LOG(stdout, " ] and exp2 = [ ");
	print_node_operand(exp2);
	LOG(stdout, " ]\n");
#endif
	valset_u vt; 
	re_list_t deflist, uselist, instlist;  	
	unsigned address, tempaddr;
	cs_arm_op *opd;

	INIT_LIST_HEAD(&deflist.deflist);
	INIT_LIST_HEAD(&uselist.uselist);
	INIT_LIST_HEAD(&instlist.instlist);	

	re_list_t * index1, *index2, *base1, *base2; 

	get_element_of_exp(exp1, &index1, &base1);
	get_element_of_exp(exp2, &index2, &base2);

//exp2 is the expression with known address

	address = exp2->node_type == UseNode ?  CAST2_USE(exp2->node)->address : CAST2_DEF(exp2->node)->address + re_ds.alias_offset; 

	assert(address);
	enum addrstat stat = exp_addr_status(base1, index1);
#ifdef VERBOSE
	LOG(stdout, "re_alias_resolve: exp2 address = %x, exp1 stat = %d\n", address, stat);
#endif
	switch(stat){
		case KBaseKIndex:	
			//check if two addresses are equal
			//if not, directly return; 
			//otherwise
			//assign address to the exp1
			//remove exp1 from umemlist
			opd = CAST2_DEF(exp1->node)->operand;
			
			tempaddr = base1 ? CAST2_USE(base1->node)->val.dword : 0;
			tempaddr += index1 ? CAST2_USE(index1->node)->val.dword : 0; 

			tempaddr += opd->mem.disp; 
			// tempaddr += op_with_gs_seg(opd) ? re_ds.coredata->corereg.gs_base : 0;  


			if(address != tempaddr){
				assert_address();
			}
			
			break;

		case UBaseUIndex:
			// do we need to set addres field for exp1?
			// if we do not, the memory expression later can't retrieve any value from coredump
			// only set address for this memory expression

			CAST2_DEF(exp1->node)->address = address;
			
			remove_from_umemlist(exp1);
			// set memory value
		/*

			if(assign_mem_val(exp1, &vt, &uselist)) {
				assign_def_after_value(exp1, vt);
				add_to_deflist(exp1, &deflist);
			}
		*/
			// remove from umemlist here
			break;

		case UBase:
			vt.dword = address - CAST2_USE(base1->node)->operand->mem.disp;	
			assign_use_value(base1, vt);
			add_to_uselist(base1, &uselist);

			// remove from umemlist here
			remove_from_umemlist(exp1);
			break;

		case UIndex:
			opd = CAST2_USE(index1->node)->operand;
			vt.dword = address - opd->mem.disp;
			assign_use_value(index1, vt);
			add_to_uselist(index1, &uselist);

			// remove from umemlist here
			remove_from_umemlist(exp1);
			break;

		case UBaseKIndex:

			opd = CAST2_USE(index1->node)->operand;
			vt.dword = address - CAST2_USE(index1->node)->val.dword - opd->mem.disp;
			assign_use_value(base1, vt);
			add_to_uselist(base1, &uselist);
			// remove from umemlist here
			remove_from_umemlist(exp1);
			break;

		case KBaseUIndex:

			opd = CAST2_USE(index1->node)->operand;
			vt.dword = address - CAST2_USE(base1->node)->val.dword - opd->mem.disp;
			assign_use_value(index1, vt);
			add_to_uselist(index1, &uselist);
			// remove from umemlist here
			remove_from_umemlist(exp1);
			break;
#ifdef VSA
		case NBaseNIndex:
			break;
#endif
		default:
			LOG(stderr, "Address Status of Expression is %d\n", stat);
			assert("Impossible" && 0);
			break;
	}

	re_resolve(&deflist, &uselist, &instlist);
}


//make a global copy of the mainlist and the umemlist
static void init_alias_config(re_t* oldre){

	INIT_LIST_HEAD(&re_ds.head.list);
	fork_corelist(&re_ds.head, &oldre->head);	

	// maintain umemlist in the old linked list

	INIT_LIST_HEAD(&re_ds.head.umemlist);
	fork_umemlist(&oldre->head);
}


static void save_re_ds(re_t *oldre) {
	memcpy(oldre, &re_ds, sizeof(re_t));
	
	REPLACE_HEAD((&re_ds.head.list), (&oldre->head.list));

	REPLACE_HEAD((&re_ds.head.umemlist), (&oldre->head.umemlist));
}


static void restore_re_ds(re_t *oldre) {
	memcpy(&re_ds, oldre, sizeof(re_t));

	LOG(stdout, "LOG: recursive count = %d\n", re_ds.rec_count);

	REPLACE_HEAD((&oldre->head.list), (&re_ds.head.list));

	REPLACE_HEAD((&oldre->head.umemlist), (&re_ds.head.umemlist));
}


void inc_rec_count(){
	re_ds.rec_count++;
	LOG(stdout, "LOG: recursive count = %d\n", re_ds.rec_count);
}

#ifdef VSA

static bool ht_verify_alias(re_list_t *exp1, re_list_t *exp2) {
	re_t oldre; 
	re_list_t *aexp1, *aexp2;
	int retval;

	LOG(stdout, "Current inst id is %d, source is %d and target is %d\n", re_ds.curinstid, find_inst_of_node(exp2)->id, find_inst_of_node(exp1)->id );

	if(find_inst_of_node(exp1)->id > re_ds.curinstid
		|| find_inst_of_node(exp2)->id > re_ds.curinstid)
		return true;

#ifdef WITH_SOLVER
	//check if the alias assumption violates the constraint system
	//take this as a fast path for alias check
	// if the constraint system tells non-alias, then quickly return...

	Z3_lbool aliascst1, aliascst2; 

	aliascst2 = check_alias_by_constraint(exp1, exp2, true, re_ds.alias_offset);

	if(aliascst2 == Z3_L_FALSE){
		return false; 
	}
#endif
}
#endif

#ifdef VSA

bool check_alias_pair(re_list_t* exp1, re_list_t* exp2){
	
	bool retval = true;
	assert(re_ds.alias_module != NO_ALIAS_MODULE);

#if defined (ALIAS_MODULE) && (ALIAS_MODULE == HT_MODULE)
	if (re_ds.alias_module == HYP_TEST_MODULE){
#if defined(TIME)
		bool dl_known = false;
		if (dl_region_verify_noalias(exp1, exp2)) {
			dl_known = true;
		}
#endif
#if defined(DL_AST)
		if (dl_region_verify_noalias(exp1, exp2)) {
			return false;
		}
#endif
#if defined(TIME)
		if (dl_known) LOG_ALIAS_TIME();
#endif
		retval = ht_verify_alias(exp1, exp2);
#if defined(TIME)
		if (dl_known) RECORD_ALIAS_TIME();
#endif
		return retval;
	}
#endif

#if defined (ALIAS_MODULE) && (ALIAS_MODULE == VSA_MODULE)
	if (re_ds.alias_module == VALSET_MODULE) {
#if defined(DL_AST)
		if (dl_region_verify_noalias(exp1, exp2)) {
			return false;
		}
#endif
		retval = valset_verify_noalias(exp1, exp2);
		if (retval) return false;
#if defined(HT_AST)
		return ht_verify_alias(exp1, exp2);
#endif
		return retval;
	}
#endif

#if defined (ALIAS_MODULE) && (ALIAS_MODULE == GT_MODULE)
	if (re_ds.alias_module == GR_TRUTH_MODULE) {
		//if (check_sysenter_block(exp1) || (check_sysenter_block(exp2)))
		//	return true;
		return !truth_verify_noalias(exp1, exp2);
	}
#endif
}
#else
bool check_alias_pair(re_list_t* exp1, re_list_t* exp2){
	
	re_t oldre; 
	re_list_t *aexp1, *aexp2;
	int retval;
	LOG(stdout, "re_mem_alias.c/check_alias_pair: re_ds.rec_count = %d\n",re_ds.rec_count);
//store the re_ds structure 
//this must be done before any alias resolving
	save_re_ds(&oldre);

//copy the execution context: head 
	init_alias_config(&oldre);

	aexp1 = get_new_exp_copy(exp1);
	aexp2 = get_new_exp_copy(exp2);

	retval = setjmp(re_ds.aliasret);
	switch (retval) {
		case REC_ADD:
			inc_rec_count();

			if (re_ds.rec_count == REC_LIMIT) {
				LOG(stdout, "LOG: Recursive count is enough\n");
				longjmp(re_ds.aliasret, 2);
			}

			re_alias_resolve(aexp1, aexp2);
			
//			if(!re_ds.resolving)
//				continue_exe_with_alias();

			//destroy the copied mainlist 
			delete_corelist(&re_ds.head);

			//recover the main data structure
			restore_re_ds(&oldre);

			return true; 

		case REC_DEC:
			delete_corelist(&re_ds.head);

			restore_re_ds(&oldre);

			return false; 

		case REC_LIM:
			delete_corelist(&re_ds.head);

			restore_re_ds(&oldre);

			return true; 

		default:
			assert(0);
			break;
	}
}
#endif

bool resolve_alias(re_list_t* exp, re_list_t *target){

	re_list_t * entry, *temp; 
	re_list_t * nextdef; 
	int dtype;

	if (re_ds.rec_count + 1  == REC_LIMIT)
		return false; 

	list_for_each_entry_safe(entry, temp, &re_ds.head.umemlist, umemlist){
		//all the unknown memory after nextdef
		if(target && node1_add_before_node2(entry, target))
			return true; 
		
		if(node1_add_before_node2(entry, exp)){

			if(!re_ds.resolving)
				return false;

			if(check_alias_pair(entry, exp)){
#ifdef VSA
				if(re_ds.rec_count == 0){
					LOG(stdout, "**** One pair of aliases cannot be resolved ****\n");
					print_instnode(find_inst_of_node(entry)->node);
					print_instnode(find_inst_of_node(exp)->node);
					LOG(stdout, "**** End of resolving one pair of aliases ****\n");
				}

#endif
				return false; 
			}
#ifdef VSA
#ifdef WITH_SOLVER
			//add the constraints that they are not alias
			if(!re_ds.rec_count){
				add_address_constraint(entry, exp, false, re_ds.alias_offset); 
			}
#endif
#endif
		}
	}
	return true; 
}	

void continue_exe_with_alias() {

	re_list_t *curinst = find_current_inst(&re_ds.head);
	int index, endindex;  

	index = CAST2_INST(curinst->node)->inst_index + 1;
	endindex = index + 50; 	

//	for (; index < re_ds.instnum; index++) {

	for (; index < endindex; index ++){
		curinst = add_new_inst(index);

		if (!curinst) {
			assert(0);
		}
/*
		LOG(stdout, "\n------------Start of one instruction assumption analysis------------\n");
		print_instnode(curinst->node);
*/

		int handler_index = insttype_to_index(re_ds.instlist[index].id);
		if (handler_index >= 0) {
			general_handler(curinst);
		} else {
			assert(0);
		} 
/*
		LOG(stdout, "-------------End of one instruction assumption analysis-------------\n");
*/

	}
	LOG(stdout, "Return from continue_exe_with_alias\n");
	//assert("continue to the end of trace" && 0);
}
