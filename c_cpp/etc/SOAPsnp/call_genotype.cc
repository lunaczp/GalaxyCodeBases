/**
  *  call__genotype.cc
  *
  *  Copyright (C) 2008, BGI Shenzhen.
  *
  *
  */
#include "soap_snp.h"

int Call_win::initialize(ubit64_t start) {
	std::string::size_type i;
	for(i=0; i != read_len + win_size ; i++) {
		sites[i].pos = i+start;
	}
	return 1;
}

int Call_win::recycle() {
	std::string::size_type i;
	for(i=0; i != read_len ; i++) {
		sites[i].pos = sites[i+win_size].pos;
		sites[i].ori = sites[i+win_size].ori;
		sites[i].depth = sites[i+win_size].depth;
		sites[i].repeat_time = sites[i+win_size].repeat_time;
		sites[i].dep_uni = sites[i+win_size].dep_uni;
		memcpy(sites[i].base_info, sites[i+win_size].base_info, sizeof(small_int)*4*2*64*256); // 4 types of bases, 2 strands, max quality score is 64, and max read length 256
		memcpy(sites[i].count_uni, sites[i+win_size].count_uni, sizeof(int)*4);
		memcpy(sites[i].q_sum, sites[i+win_size].q_sum, sizeof(int)*4);
		memcpy(sites[i].count_all, sites[i+win_size].count_all, sizeof(int)*4);
	}
	for(i=read_len; i != read_len+win_size; i++) {
		sites[i].ori = 0xFF;
		sites[i].pos = sites[i-1].pos+1;
		sites[i].depth = 0;
		sites[i].repeat_time = 0;
		sites[i].dep_uni = 0;
		memset(sites[i].count_uni,0,sizeof(int)*4);
		memset(sites[i].q_sum,0,sizeof(int)*4);
		memset(sites[i].base_info,0,sizeof(small_int)*4*2*64*256);
		memset(sites[i].count_all,0,sizeof(int)*4);
	}
	return 1;
}


int Call_win::call_cns(Chr_name call_name, Chr_info* call_chr, ubit64_t call_length, Prob_matrix * mat, Parameter * para, std::ofstream & consensus) {
	std::string::size_type coord;
	small_int k;
	ubit64_t o_base, strand;
	char allele1, allele2, genotype, type, type1/*best genotype*/, type2/*suboptimal genotype*/, base1, base2, base3;
	int i, q_score, q_adjusted, qual1, qual2, qual3, q_cns, all_count1, all_count2, all_count3;
	double  rank_sum_test_value, binomial_test_value;
	bool is_out;
	double * real_p_prior = new double [16];
	//std::cerr<<"Call length="<<call_length<<endl;
	for(std::string::size_type j=0; j != call_length; j++){
		if(para->region_only && !call_chr->is_in_region(sites[j].pos)) {
			continue;
		}
		sites[j].ori = call_chr->get_bin_base(sites[j].pos);
		if( ((sites[j].ori&4) !=0)/*an N*/ && sites[j].depth == 0 && ! para->is_snp_only) {
			// CNS text format:
			// ChrID\tPos\tRef\tCns\tQual\tBase1\tAvgQ1\tCountUni1\tCountAll1\tBase2\tAvgQ2\tCountUni2\tCountAll2\tDepth\tRank_sum\tCopyNum\tSNPstauts\n"
			if(!para->glf_format) {
				consensus<<call_name<<'\t'<<(sites[j].pos+1)<<"\tN\tN\t0\tN\t0\t0\t0\tN\t0\t0\t0\t0\t1.000\t255.000\t0"<<endl;
			}
			else {
				//if ( sites[j].pos % 10000 ==0) {
				//	cerr<<sites[j].pos<<endl;
				//}
				consensus<<(unsigned char)(0xF<<4|0)<<(unsigned char)(0<<4|0xF)<<flush;
				for(type=0;type!=10;type++) {
					consensus<<(unsigned char)0;
				}
				consensus<<flush;
				if(!consensus.good()) {
					cerr<<"Broken ofstream after writting Position "<<(sites[j].pos+1)<<" at "<<call_name<<endl;
					exit(255);
				}
			}
			continue;
		}
		base1 = 0, base2 = 0, base3 = 0;
		qual1 = -1, qual2= -2, qual3 = -3;
		all_count1 = 0, all_count2 =0, all_count3 =0;
		if(sites[j].dep_uni) {
			// This position is uniquely covered by at least one nucleotide
			for(i=0;i!=4;i++) {
				// i is four kind of alleles
				if(sites[j].q_sum[i] >= qual1) {
					base3 = base2;
					qual3 = qual2;
					base2 = base1;
					qual2 = qual1;
					base1 = i;
					qual1 = sites[j].q_sum[i];
				}
				else if (sites[j].q_sum[i]>=qual2) {
					base3 = base2;
					qual3 = qual2;
					base2 = i;
					qual2  = sites[j].q_sum[i];
				}
				else if (sites[j].q_sum[i]>=qual3) {
					base3 = i;
					qual3  = sites[j].q_sum[i];
				}
				else {
					;
				}
			}
			if(qual1 == 0) {
				// Adjust the best base so that things won't look ugly if the pos is not covered
				base1 = (sites[j].ori&7);
			}
			else if(qual2 ==0 && base1 != (sites[j].ori&7)) {
				base2 = (sites[j].ori&7);
			}
			else {
				;
			}
		}
		else {
			// This position is covered by all repeats
			for(i=0;i!=4;i++) {
				if(sites[j].count_all[i] >= all_count1) {
					base3 = base2;
					all_count3 = all_count2;
					base2 = base1;
					all_count2 = all_count1;
					base1 = i;
					all_count1 = sites[j].count_all[i];
				}
				else if (sites[j].count_all[i]>=all_count2) {
					base3 = base2;
					all_count3 = all_count2;
					base2 = i;
					all_count2  = sites[j].count_all[i];
				}
				else if (sites[j].count_all[i]>=all_count3) {
					base3 = i;
					all_count3  = sites[j].count_all[i];
				}
				else {
					;
				}
			}
			if(all_count1 ==0) {
				base1 = (sites[j].ori&7);
			}
			else if(all_count2 ==0 && base1 != (sites[j].ori&7)) {
				base2 = (sites[j].ori&7);
			}
			else {
				;
			}
		}
		// Calculate likelihood
		for(genotype=0;genotype!=16;genotype++){
			mat->type_likely[genotype] = 0.0;
		}
		std::string::size_type global_dep_count(0), pcr_dep_count(0);
		for(o_base=0;o_base!=4;o_base++) {
			//cerr<<o_base<<endl;
			if(sites[j].count_uni[o_base]==0) {continue;}
			global_dep_count = 0;

			for(coord=0; coord != para->read_length; coord++) {
				for(strand=0; strand!=2; strand++) {
					pcr_dep_count =0;
					for(q_score=para->q_max-para->q_min; q_score != -1; q_score--) {
						for(k=0; k!=sites[j].base_info[o_base<<15|strand<<14|q_score<<8|coord];k++) {
							q_adjusted = int( pow(10, (log10(q_score) +pcr_dep_count*para->pcr_dependency +global_dep_count*para->global_dependency)) +0.5);
							if(q_adjusted < 1) {
								q_adjusted = 1;
							}
							if( 0==k ) {
								// First base located at the coordinate
								global_dep_count += 1;
							}
							pcr_dep_count += 1;
							for(allele1 = 0;allele1 != 4;allele1++ ) {
								for(allele2 = allele1; allele2 != 4; allele2++) {
									mat->type_likely[allele1<<2|allele2] += log10(0.5*mat->p_matrix[ ((ubit64_t)q_adjusted<<12) | (coord <<4) | (allele1<<2) | o_base] +0.5*mat->p_matrix[((ubit64_t)q_adjusted<<12) | (coord <<4) | (allele2<<2) | o_base]);
									//if(sites[j].pos == 52100) {
									//	cerr<<"Now:"<<abbv[allele1<<2|allele2]<<'\t'<<mat->p_matrix[ ((ubit64_t)q_adjusted<<12) | (coord <<4) | (allele1<<2) | o_base]<<'\t'<<mat->p_matrix[ ((ubit64_t)q_adjusted<<12) | (coord <<4) | (allele2<<2) | o_base]<<'\t'<<mat->type_likely[allele1<<2|allele2]<<endl;
									//}
								}
							}
							//if(sites[j].pos == 52100) {
							//	cerr<<"ACTG"[o_base]<<'\t'<<(int)k<<'\t'<<(int)sites[j].base_info[o_base<<15|strand<<14|q_score<<8|coord]<<'\t'<<sites[j].count_uni[o_base]<<endl;//fprintf(stderr, "Now:%c\t\t%le\t%le\t%le\n", abbv[allele1<<2|allele2], mat->p_matrix[ ((ubit64_t)q_adjusted<<12) | (coord <<4) | (allele1<<2) | o_base], mat->p_matrix[ ((ubit64_t)q_adjusted<<12) | (coord <<4) | (allele2<<2) | o_base], mat->type_likely[allele1<<2|allele2]);
							//}
						}
					}
				}
			}
		}
		if(1==para->glf_format) {
			// Generate GLFv2 format
			int copy_num;
			if(sites[j].depth == 0) {
				copy_num = 15;
			}
			else {
				copy_num = int(1.442695041*log(sites[j].repeat_time/sites[j].depth));
				if(copy_num>15) {
					copy_num = 15;
				}
			}
			if(sites[j].depth >255) {
				sites[j].depth = 255;
			}
			consensus<<(unsigned char)(glf_base_code[sites[j].ori&7]<<4|((sites[j].depth>>4)&0xF))<<(unsigned char)((sites[j].depth&0xF)<<4|copy_num&0xF)<<flush;
			type1 = 0;
			// Find the largest likelihood
			for (allele1=0; allele1!=4; allele1++) {
				for (allele2=allele1; allele2!=4; allele2++) {
					genotype = allele1<<2|allele2;
					if (mat->type_likely[genotype] > mat->type_likely[type1]) {
						type1 = genotype;
					}
					else {
						;
					}
				}
			}
			for(type=0;type!=10;type++) {
				if(mat->type_likely[type1]-mat->type_likely[glf_type_code[type]]>25.5) {
					consensus<<(unsigned char)255;
				}
				else {
					consensus<<(unsigned char)(unsigned int)(10*(mat->type_likely[type1]-mat->type_likely[glf_type_code[type]]));
				}
			}
			consensus<<flush;
			if(!consensus.good()) {
				cerr<<"Broken ofstream after writting Position "<<(sites[j].pos+1)<<" at "<<call_name<<endl;
				exit(255);
			}
			continue;
		}
		// Calculate Posterior Probability
		memcpy(real_p_prior, &mat->p_prior[((ubit64_t)sites[j].ori&0x7)<<4], sizeof(double)*16);
		if ( (sites[j].ori & 0x8) && para->refine_mode) {
			// a dbSNP
			snp_p_prior_gen(real_p_prior, call_chr->find_snp(sites[j].pos), para, sites[j].ori);
		}
		memset(mat->type_prob,0,sizeof(rate_t)*17);
		type2=type1=16;
		for (allele1=0; allele1!=4; allele1++) {
			for (allele2=allele1; allele2!=4; allele2++) {
				genotype = allele1<<2|allele2;
				if (para->is_monoploid && allele1 != allele2) {
					continue;
				}
				mat->type_prob[genotype] = mat->type_likely[genotype] + log10(real_p_prior[genotype]) ;

				if (mat->type_prob[genotype] >= mat->type_prob[type1] || type1 == 16) {
					type2 = type1;
					type1 = genotype;
				}
				else if (mat->type_prob[genotype] >= mat->type_prob[type2] || type2 ==16) {
					type2 = genotype;
				}
				else {
					;
				}
			}
		}
		if(2==para->glf_format) {
			// Generate GLFv2 format
			int copy_num;
			if(sites[j].depth == 0) {
				copy_num = 15;
			}
			else {
				copy_num = int(1.442695041*log(sites[j].repeat_time/sites[j].depth));
				if(copy_num>15) {
					copy_num = 15;
				}
			}
			if(sites[j].depth >255) {
				sites[j].depth = 255;
			}
			consensus<<(unsigned char)(glf_base_code[sites[j].ori&7]<<4|((sites[j].depth>>4)&0xF))<<(unsigned char)((sites[j].depth&0xF)<<4|copy_num&0xF)<<flush;
			type1 = 0;
			// Find the largest likelihood
			for (allele1=0; allele1!=4; allele1++) {
				for (allele2=allele1; allele2!=4; allele2++) {
					genotype = allele1<<2|allele2;
					if (mat->type_prob[genotype] > mat->type_prob[type1]) {
						type1 = genotype;
					}
					else {
						;
					}
				}
			}
			for(type=0;type!=10;type++) {
				if(mat->type_prob[type1]-mat->type_prob[glf_type_code[type]]>25.5) {
					consensus<<(unsigned char)255;
				}
				else {
					consensus<<(unsigned char)(unsigned int)(10*(mat->type_prob[type1]-mat->type_prob[glf_type_code[type]]));
				}
			}
			consensus<<flush;
			if(!consensus.good()) {
				cerr<<"Broken ofstream after writting Position "<<(sites[j].pos+1)<<" at "<<call_name<<endl;
				exit(255);
			}
			continue;
		}
		is_out = true; // Check if the position needs to be output, useful in snp-only mode

		rank_sum_test_value = rank_test(sites[j], type1, mat->p_rank, para);
		if(rank_sum_test_value==0.0) {
			// avoid double genotype overflow
			q_cns = 0;
		}
		else {
			q_cns = (int)(10*(mat->type_prob[type1]-mat->type_prob[type2])+10*log10(rank_sum_test_value));
		}

		if ( (type1&3) == ((type1>>2)&3)) { // Called Homozygous
			if (qual1>0 && base1 != (type1&3)) {
				//Wired: best base is not the consensus!
				q_cns = 0;
			}
			else if (/*qual2>0 &&*/ q_cns > qual1-qual2) {
				// Should not bigger than this
				q_cns = qual1-qual2;
			}
			else {
				;
			}
		}
		else {	// Called Heterozygous
			if(sites[j].q_sum[base1]>0 && sites[j].q_sum[base2]>0 && type1 == (base1<base2 ? (base1<<2|base2):(base2<<2|base1))) {
				// The best bases are in the heterozygote
				if (q_cns > qual2-qual3) {
					q_cns = qual2-qual3;
				}
			}
			else {	// Ok, wired things happened
				q_cns = 0;
			}
		}
		if(q_cns>99) {
			q_cns = 99;
		}
		if (q_cns<0) {
			q_cns = 0;
		}
		// ChrID\tPos\tRef\tCns\tQual\tBase1\tAvgQ1\tCountUni1\tCountAll1\tBase2\tAvgQ2\tCountUni2\tCountAll2\tDepth\tRank_sum\tCopyNum\tSNPstauts\n"
		if(! para->is_snp_only || (abbv[type1] != "ACTGNNNN"[(sites[j].ori&0x7)])) {
			if(base1<4 && base2<4) {
				consensus<<call_name<<'\t'<<(sites[j].pos+1)<<'\t'<<("ACTGNNNN"[(sites[j].ori&0x7)])<<'\t'<<abbv[type1]<<'\t'<<q_cns<<'\t'
					<<("ACTGNNNN"[base1])<<'\t'<<(sites[j].q_sum[base1]==0?0:sites[j].q_sum[base1]/sites[j].count_uni[base1])<<'\t'<<sites[j].count_uni[base1]<<'\t'<<sites[j].count_all[base1]<<'\t'
					<<("ACTGNNNN"[base2])<<'\t'<<(sites[j].q_sum[base2]==0?0:sites[j].q_sum[base2]/sites[j].count_uni[base2])<<'\t'<<sites[j].count_uni[base2]<<'\t'<<sites[j].count_all[base2]<<'\t'
					<<sites[j].depth<<'\t'<<showpoint<<rank_sum_test_value<<'\t'<<(sites[j].depth==0?255:(double)(sites[j].repeat_time)/sites[j].depth)<<'\t'<<((sites[j].ori&8)?1:0)<<endl;
			}
			else if(base1<4) {
				consensus<<call_name<<'\t'<<(sites[j].pos+1)<<'\t'<<("ACTGNNNN"[(sites[j].ori&0x7)])<<'\t'<<abbv[type1]<<'\t'<<q_cns<<'\t'
					<<("ACTGNNNN"[base1])<<'\t'<<(sites[j].q_sum[base1]==0?0:sites[j].q_sum[base1]/sites[j].count_uni[base1])<<'\t'<<sites[j].count_uni[base1]<<'\t'<<sites[j].count_all[base1]<<'\t'
					<<"N\t0\t0\t0\t"
					<<sites[j].depth<<'\t'<<showpoint<<rank_sum_test_value<<'\t'<<(sites[j].depth==0?255:(double)(sites[j].repeat_time)/sites[j].depth)<<'\t'<<((sites[j].ori&8)?1:0)<<endl;
			}
			else {
				consensus<<call_name<<'\t'<<(sites[j].pos+1)<<"\tN\tN\t0\tN\t0\t0\t0\tN\t0\t0\t0\t0\t1.000\t255.000\t0"<<endl;
			}
		}
		//if(sites[j].pos == 52100) {
		//	exit(1);
		//}
	}
	delete [] real_p_prior;
	return 1;
}

int Call_win::soap2cns(std::ifstream & alignment, std::ofstream & consensus, Genome * genome, Prob_matrix * mat, Parameter * para) {
	Soap_format soap;
	map<Chr_name, Chr_info*>::iterator current_chr, prev_chr;
	current_chr = prev_chr = genome->chromosomes.end();
	int coord, sub;
	int last_start(0);
	for(std::string line; getline(alignment, line);) {
		std::istringstream s(line);
		if( s>> soap ) {
			//cerr<<"A"<<soap.get_pos()<<endl;
			//exit(1);
			if(soap.get_pos() < 0) {
				continue;
			}
			if (current_chr == genome->chromosomes.end() || current_chr->first != soap.get_chr_name()) {
				if(current_chr != genome->chromosomes.end() ) {
					while(current_chr->second->length() > sites[win_size-1].pos) {
						call_cns(current_chr->first, current_chr->second, win_size, mat, para, consensus);
						recycle();
						last_start = sites[win_size-1].pos;
					}
					call_cns(current_chr->first, current_chr->second, current_chr->second->length()%win_size, mat, para, consensus);
				}
				current_chr = genome->chromosomes.find(soap.get_chr_name());
				initialize(0);
				last_start = 0;
				if(para->glf_format) {
					cerr<<"Processing "<<current_chr->first<<endl;
					int temp_int(current_chr->first.size()+1);
					consensus.write(reinterpret_cast<char *> (&temp_int), sizeof(temp_int));
					consensus.write(current_chr->first.c_str(), current_chr->first.size()+1);
					temp_int = current_chr->second->length();
					consensus.write(reinterpret_cast<char *> (&temp_int), sizeof(temp_int));
					consensus<<flush;
					if (!consensus.good()) {
						cerr<<"Broken IO stream after writing chromosome info."<<endl;
						exit(255);
					}
					assert(consensus.good());
				}
			}
			else {
				;
			}

			if(soap.get_pos() < last_start) {
				cerr<<"Errors in sorting:"<<soap.get_pos()<<"<"<<last_start<<endl;
				exit(255);
			}
			while (soap.get_pos()/win_size > last_start/win_size ) {
				// We should call the base here
				call_cns(current_chr->first, current_chr->second, win_size, mat, para, consensus);
				recycle();
				last_start = sites[win_size-1].pos;
			}
			last_start=soap.get_pos();
			for(coord=0; coord<soap.get_read_len(); coord++){
				if( (soap.get_pos()+coord)/win_size == soap.get_pos()/win_size ) {
					// In the same sliding window
					sub = (soap.get_pos()+coord) % win_size;
				}
				else {
					sub = (soap.get_pos()+coord) % win_size + win_size; // Use the tail to store the info so that it won't intervene the uncalled bases
				}
				sites[sub].depth += 1;
				sites[sub].repeat_time += soap.get_hit();
				if((soap.is_N(coord)) || soap.get_qual(coord)<para->q_min || sites[sub].dep_uni >= 0xFF) {
					// An N, low quality or meaningless huge depth
					continue;
				}
				if(soap.get_hit() == 1) {
					sites[sub].dep_uni += 1;
					// Update the covering info: 4x2x64x64 matrix, base x strand x q_score x read_pos, 2-1-6-6 bits for each
					if(soap.is_fwd()) {
						// Binary strand: 0 for plus and 1 for minus
						sites[sub].base_info[(((ubit64_t)(soap.get_base(coord)&0x6)|0))<<14 | ((ubit64_t)(soap.get_qual(coord)-para->q_min))<<8 | coord ] += 1;
					}
					else {
						sites[sub].base_info[(((ubit64_t)(soap.get_base(coord)&0x6)|1))<<14 | ((ubit64_t)(soap.get_qual(coord)-para->q_min))<<8 | (soap.get_read_len()-1-coord) ] += 1;

					}
					sites[sub].count_uni[(soap.get_base(coord)>>1)&3] += 1;
					sites[sub].q_sum[(soap.get_base(coord)>>1)&3] += (soap.get_qual(coord)-para->q_min);
				}
				else {
					;// Repeats
				}
				sites[sub].count_all[(soap.get_base(coord)>>1)&3] += 1;
			}
		}
	}
	//cerr<<"Name = "<<current_chr->first<<endl;
	while(current_chr->second->length() > sites[win_size-1].pos) {
		call_cns(current_chr->first, current_chr->second, win_size, mat, para, consensus);
		recycle();
		last_start = sites[win_size-1].pos;
	}
	call_cns(current_chr->first, current_chr->second, current_chr->second->length()%win_size, mat, para, consensus);
	alignment.close();
	consensus.close();
	return 1;
}