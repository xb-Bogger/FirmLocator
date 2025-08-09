#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <capstone/capstone.h>
#include "global.h"
#include "disassemble.h"
#include "insthandler_arm.h"
#include "reverse_exe.h"
#include "analyze_result.h"
#include "re_runtime.h"
#ifdef VSA
#include "bin_alias.h"
#include "common.h"
#endif
unsigned long reverse_instructions(void){

	unsigned index; 
	re_list_t *curinst, *previnst; 
	re_list_t *entry; 

	re_list_t re_deflist, re_uselist, re_instlist;  	

	//init the main linked list
	INIT_LIST_HEAD(&re_ds.head.list);
	INIT_LIST_HEAD(&re_ds.head.umemlist);
#ifdef VSA
    INIT_LIST_HEAD(&re_ds.head.memlist);
    re_ds.alias_module = NO_ALIAS_MODULE;
#endif
	re_ds.resolving = false; 	

	LOG_RUNTIME("start reverse execution");
	for(index = 0; index < re_ds.instnum; index++){

		if (verify_useless_inst(re_ds.instlist + index)) {
			continue;
		}

		//insert the instruction data into the current linked list
		curinst = add_new_inst(index);
		if( !curinst){
			assert(0);
		}

		

		LOG(stdout, "------------------Start of one instruction analysis-----------------\n");
		print_instnode(curinst->node);

		int handler_index = insttype_to_index(re_ds.instlist[index].id);
		if (handler_index >= 0) {
			LOG(stdout, "DEBUG: Entering %s handler\n", cs_insn_name(re_ds.handle, re_ds.instlist[index].id));
			// inst_handler[handler_index](curinst);
			general_handler(curinst);

			LOG(stdout, "DEBUG: Leaving %s handler\n", cs_insn_name(re_ds.handle, re_ds.instlist[index].id));
		} else {
			LOG(stdout, "ERROR: %s handler\n", cs_insn_name(re_ds.handle,re_ds.instlist[index].id));
			assert(0);
		}

		print_info_of_current_inst(curinst);

		LOG(stdout, "------------------ End of one instruction analysis------------------\n\n");
	}
// /*
	LOG_RUNTIME("finish reverse execution");

#ifdef VSA
    calculate_truth_address();
    #if defined (ALIAS_MODULE) && (ALIAS_MODULE == NO_MODULE)
	re_statistics();

	LOG_RUNTIME("Finish Reverse Execution Evaluation");

	//analyze_corelist();

	destroy_corelist();

	return 0;
    #endif

#if defined (ALIAS_MODULE) && (ALIAS_MODULE == HT_MODULE)
	re_ds.alias_module = HYP_TEST_MODULE;
#endif

#if defined (ALIAS_MODULE) && (ALIAS_MODULE == VSA_MODULE)
	LOG_RUNTIME("start vsa analysis");
	init_vsa_analysis();
	list_for_each_entry(entry, &re_ds.head.list, list)
	{

		if (entry->node_type != InstNode) continue;
		// handle all the instructions according to semantics

		LOG(stdout, "\n");
		LOG(stdout, "------------------Start of forward valset analysis-------------------\n");

		//print_info_of_current_inst(entry);

		index = CAST2_INST(entry->node)->inst_index;

		int handler_index = insttype_to_index(re_ds.instlist[index].id);

		if (handler_index >= 0) {
			vs_handler[handler_index](entry);
		} else {
			LOG(stdout, "instruction type %x\n", re_ds.instlist[index].id);
			assert(0);
		}

		print_info_of_current_inst(entry);

		LOG(stdout, "------------------ End of one forward valset analysis----------------\n");
	}

	LOG_RUNTIME("finish vsa analysis");

	//print_valsetlist(&re_ds.head);
	//print_uvalset_list(&re_ds.head);

	valset_correctness_check();
	//LOG_RUNTIME("Finish Value Set Correctness Check");

	valset_statistics();
	// ratio_noalias_pair();
	//LOG_RUNTIME("Finish Value Set Evaluation");

	re_ds.alias_module = VALSET_MODULE;
#endif

	// note This loading seems to be executed in all modes.
	// load_region_from_DL(get_region_path());
	// LOG_RUNTIME("Finish Loading Region from DeepLearning Data");

#if defined (ALIAS_MODULE) && (ALIAS_MODULE == GT_MODULE)
	re_ds.alias_module = GR_TRUTH_MODULE;
#endif

	//make another assumption here
	//We assume the registers are recorded at the begining of the trace
	//this somehow makes sense


	// note: This does not make sense. DeepVSA DOES NOT leverage the register values of every instructions
	// It just use DL method to infer variable regions
	// list_for_each_entry(entry, &re_ds.head.list, list) {

	// 	INIT_LIST_HEAD(&re_deflist.deflist);
	// 	INIT_LIST_HEAD(&re_uselist.uselist);
	// 	INIT_LIST_HEAD(&re_instlist.instlist);	
		
	// 	if(entry->node_type != UseNode)
	// 		continue; 

	// 	init_reg_use(entry, &re_uselist);	
	// 	re_resolve(&re_deflist, &re_uselist, &re_instlist);
	// }
	LOG_RUNTIME("start resolve");
	one_round_of_re();
	LOG_RUNTIME("finish resolve");

	//re_statistics();
	//LOG_RUNTIME("Finish Reverse Execution Evaluation");

	set_segment_for_region();
	print_region_info(&re_ds.region_head);

	unsigned round_num = 1;
	unsigned long infer_num;

	while ((infer_num = infer_valset_from_re()) != 0) {

		print_region_info(&re_ds.region_head);

		LOG(stderr, "=========> Start %d round of Combining VSA and RE \n", round_num);

		// do third round of value set analysis
		one_round_of_vsa();

		LOG_RUNTIME("Finish One Round of Value Set Analysis");

		//valset_statistics();
		// ratio_noalias_pair();
		//LOG_RUNTIME("Finish Value Set Evaluation");

		// do third round of reverse execution 
		one_round_of_re();
		
		LOG_RUNTIME("Finish One Round of Reverse Execution");

		//re_statistics();

		//LOG_RUNTIME("Finish Reverse Execution Evaluation");

		round_num++;

		if (round_num > ROUND_LIMIT) break;
	}
	re_statistics();

	valset_correctness_check();
	//LOG_RUNTIME("Finish Value Set Correctness Check");

	valset_statistics();
	//print_corelist(&re_ds.head);

#if defined (ALIAS_MODULE) && (ALIAS_MODULE == HT_MODULE) && defined(DL_AST)
	ratio_noalias_pair_ht();
	LOG_RUNTIME("Finish No Alias Pair Evaluation");
#endif

#if defined (ALIAS_MODULE) && (ALIAS_MODULE == VSA_MODULE) && defined(DL_AST)
	ratio_noalias_pair_vsa();
	LOG_RUNTIME("Finish No Alias Pair Evaluation");
#endif

#if defined (ALIAS_MODULE) && defined(TIME)
	print_total_alias_time();
#endif
	LOG_RUNTIME("start backward tainting");
	analyze_corelist();
	LOG_RUNTIME("finish backward tainting");
	print_umemlist(&re_ds.head);

	destroy_corelist();

	return 0; 
#endif
	re_ds.resolving = true; 	
	LOG_RUNTIME("start resolve");
	list_for_each_entry_reverse(entry, &re_ds.head.list, list) {

		print_node(entry);

		if (entry->node_type == InstNode) {

			// print_node(entry);

			INIT_LIST_HEAD(&re_deflist.deflist);
			INIT_LIST_HEAD(&re_uselist.uselist);
			INIT_LIST_HEAD(&re_instlist.instlist);	
		
			add_to_instlist(entry, &re_instlist);
			re_resolve(&re_deflist, &re_uselist, &re_instlist);

			resolve_heuristics(entry, &re_deflist, &re_uselist, &re_instlist);
				
			re_resolve(&re_deflist, &re_uselist, &re_instlist);
		}
	}
// */
	LOG_RUNTIME("finish resolve");


	LOG(stdout, "Max Function ID is %d\n", maxfuncid());
	// print_corelist(&re_ds.head);
	LOG_RUNTIME("start backward tainting");
	analyze_corelist();
    LOG_RUNTIME("finish backward tainting");
    print_umemlist(&re_ds.head);
    destroy_corelist();
	return 0;    
}
