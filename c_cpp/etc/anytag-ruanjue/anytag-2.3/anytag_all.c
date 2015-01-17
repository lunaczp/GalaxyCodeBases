/*
 * 
 * Copyright (c) 2011, Jue Ruan <ruanjue@gmail.com>
 *
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#define _GNU_SOURCE
#include <unistd.h>
#include "sr_aln.h"
#include "local_assembly.h"
#include "file_reader.h"
#include "thread.h"
#include "anytag_aux.h"

int usage_all();

typedef struct {
	uint32_t rid;
	uint16_t offset:9, sign:1, n_mm:4, dir1:1, dir2:1;
} SimpleHit;

define_list(hitv, SimpleHit);

static inline void sr_hit2simp(SR_AlnHit *hit, SimpleHit *h, uint32_t ar_id){
	if(ar_id == hit->rid1){
		h->dir1 = hit->dir1; h->dir2 = hit->dir2; h->offset = hit->off; h->sign = 0; h->n_mm = hit->n_mm; h->rid = hit->rid2;
	} else {
		h->dir1 = !hit->dir2; h->dir2 = !hit->dir1; h->offset = hit->off; h->sign = 1; h->n_mm = hit->n_mm; h->rid = hit->rid1;
	}
}

static inline void write_sr_hit2simp(SR_AlnHit *hit, uint32_t ar_id, FILE *out){
	uint8_t byte;
	uint32_t rid;
	if(ar_id == hit->rid1){
		byte = (0x80U) | (hit->n_mm << 2) | ((hit->dir1 & 0x01) << 1) | (hit->dir2 & 0x01); fwrite(&byte, 1, 1, out);
		byte = hit->off; fwrite(&byte, 1, 1, out);
		rid = hit->rid2;
	} else {
		byte = (0x80U) | (hit->n_mm << 2) | ((!hit->dir2) << 1) | (!hit->dir1); fwrite(&byte, 1, 1, out);
		byte = hit->off | 0x80U; fwrite(&byte, 1, 1, out);
		rid = hit->rid1;
	}
	byte = (rid >>  0) & 0xFFU; fwrite(&byte, 1, 1, out);
	byte = (rid >>  8) & 0xFFU; fwrite(&byte, 1, 1, out);
	byte = (rid >> 16) & 0xFFU; fwrite(&byte, 1, 1, out);
	byte = (rid >> 24) & 0xFFU; fwrite(&byte, 1, 1, out);
}

static inline int read_simphit(SimpleHit *hit, FILE *in){
	uint8_t byte;
	if(fread(&byte, 1, 1, in) == 0) return 0;
	if(byte == 0) return 0;
	hit->dir1 = (byte >> 1) & 0x01;
	hit->dir2 = (byte >> 0) & 0x01;
	hit->n_mm = (byte >> 2) & 0x0F;
	hit->rid = 0;
	if(fread(&byte, 1, 1, in) == 0) return 0; hit->offset = byte & 0x7F; hit->sign = byte >> 7;
	if(fread(&byte, 1, 1, in) == 0) return 0; hit->rid |= ((uint32_t)byte) <<  0;
	if(fread(&byte, 1, 1, in) == 0) return 0; hit->rid |= ((uint32_t)byte) <<  8;
	if(fread(&byte, 1, 1, in) == 0) return 0; hit->rid |= ((uint32_t)byte) << 16;
	if(fread(&byte, 1, 1, in) == 0) return 0; hit->rid |= ((uint32_t)byte) << 24;
	return 1;
}

static inline uint32_t shuffle_hits(BitVec *flags, hitv *hits, uint32_t m){
	uint32_t i, j, n;
	double r;
	n = hits->size;
	if(m >= n){ ones_bitvec(flags); return n; }
	zeros_bitvec(flags);
	r = ((double)m) / n;
	i = 0; j = 0;
	while(j < m){
		if(get_bitvec(flags, i) == 0 && drand48() <= r){
			one_bitvec(flags, i); j ++;
		}
		if(i == n -1) i = 0;
		else i ++;
	}
	return j;
}

thread_beg_def(mall);
SR_SeqDB *sdb;
uint32_t rid_beg, rid_end;
ATOptions *opt;
LGraph *g;
LibInserts *libs;
int task;
FILE *cnsf, *msaf, *alnf, *dotf, *log;
char *prefix;
thread_end_def(mall);

thread_beg_func(mall);
SR_SeqDB *sdb;
SR_AlnAux *aux1, *aux2;
SR_AlnHit *hit, SP;
LGraph *g;
LibInserts *libs;
hitv *hits1, *hits2;
SimpleHit H;
BitVec *shuffle[2], *ps_state;
char seq1[1024], seq2[1024], chs[512];
FILE *cnsf, *msaf, *alnf, *dotf, *log, *skef;
FILE **aln_parts;
FILE **cns_parts;
char *cns_buf, *msa_buf, *aln_buf, *dot_buf;
size_t cns_size, msa_size, aln_size, dot_size;
uint64_t rd_off1, rd_off2;
uint32_t i, j, rd_len1, rd_len2, t_idx, n_cpu, n_full, l, r, ll, rr, n, m;
int lib_id, min_ins, max_ins, off;
uint8_t byte;
time_t t1, t2;
sdb = mall->sdb;
g = mall->g;
t_idx = mall->t_idx;
n_cpu = mall->n_cpu;
n_full = (mall->opt->n_full + mall->opt->n_cpu - 1) / mall->opt->n_cpu;
aux1 = sr_init_aux(1, mall->opt->min_ol[0], mall->opt->max_mm[0], mall->opt->min_sm[0], mall->opt->allow_gap[0], 0, mall->opt->max_hits[0] / sdb->n_part);
aux2 = sr_init_aux(1, mall->opt->min_ol[0], mall->opt->max_mm[0], mall->opt->min_sm[0], mall->opt->allow_gap[0], 0, mall->opt->max_hits[0] / sdb->n_part);
sr_fit_aux2sdb(aux1, sdb);
sr_fit_aux2sdb(aux2, sdb);
hits1 = init_hitv(1024); hits2 = init_hitv(1024);
shuffle[0] = init_bitvec(1024); shuffle[1] = init_bitvec(1024);
ps_state = init_bitvec(1024);
cns_buf = NULL; msa_buf = NULL; aln_buf = NULL; dot_buf = NULL;
cns_size = 0; msa_size = 0; aln_size = 0; dot_size = 0;
cnsf = open_memstream(&cns_buf, &cns_size);
msaf = open_memstream(&msa_buf, &msa_size);
alnf = open_memstream(&aln_buf, &aln_size);
dotf = open_memstream(&dot_buf, &dot_size);
log  = mall->log;
libs = mall->libs;
skef = NULL;
memset(&SP, 0, sizeof(SR_AlnHit));
if(sdb->n_part < 2){ aln_parts = NULL; cns_parts = NULL; } else {
	aln_parts = malloc(sizeof(FILE*) * (sdb->n_part));
	cns_parts = malloc(sizeof(FILE*) * (sdb->n_part));
}
thread_beg_loop(mall);
if(mall->task == 1){
	for(i=t_idx;i<sdb->n_idx;i+=n_cpu){
		fprintf(log, "[THREAD%03u] indexing %u/%u. \n", t_idx, i, sdb->n_idx); fflush(log);
		sr_index_sdb(sdb, i);
		fprintf(log, "[THREAD%03u] indexed %u/%u. \n", t_idx, i, sdb->n_idx); fflush(log);
	}
} else if(mall->task == 2){
	t1 = time(NULL);
	n = m = 0;
	sprintf(chs, "%s.aln.p%03u.t%03u", mall->prefix, sdb->part_idx, t_idx);
	if((aln_parts[sdb->part_idx] = fopen(chs, "w+")) == NULL){
		fprintf(stdout, " -- Cannot create %s in %s -- %s:%d --\n", chs, __FUNCTION__, __FILE__, __LINE__); fflush(stdout); abort();
	}
	for(i=mall->rid_beg+t_idx;i<mall->rid_end;i+=n_cpu){
		m ++;
		if((m % 100000) == 0){
			t2 = time(NULL); fprintf(log, "[THREAD%03u] aligned %u / %u, %u sec. \n", t_idx, n, m, (unsigned)(t2 - t1)); fflush(log);
		}
		sr_clear_aux(aux1); sr_align_sdb(sdb, 2 * i + 0, aux1);
		sr_clear_aux(aux2); sr_align_sdb(sdb, 2 * i + 1, aux2);
		l = count_sr_hitv(aux1->hits);
		r = count_sr_hitv(aux2->hits);
		if(l + r) n ++;
		for(j=0;j<l;j++){ hit = ref_sr_hitv(aux1->hits, j); write_sr_hit2simp(hit, 2 * i + 0, aln_parts[sdb->part_idx]); }
		byte = 0; fwrite(&byte, 1, 1, aln_parts[sdb->part_idx]);
		for(j=0;j<r;j++){ hit = ref_sr_hitv(aux2->hits, j); write_sr_hit2simp(hit, 2 * i + 1, aln_parts[sdb->part_idx]); }
		byte = 0; fwrite(&byte, 1, 1, aln_parts[sdb->part_idx]);
	}
} else if(mall->task == 3){
	t1 = time(NULL);
	n = m = 0;
	if(sdb->n_part > 1){
		sprintf(chs, "%s.cns.p%03u.t%03u", mall->prefix, sdb->part_idx, t_idx);
		if((cns_parts[sdb->part_idx] = fopen(chs, "w+")) == NULL){
			fprintf(stdout, " -- Cannot create %s in %s -- %s:%d --\n", chs, __FUNCTION__, __FILE__, __LINE__); fflush(stdout); abort();
		}
		sprintf(chs, "%s.cns.ske.t%03u", mall->prefix, t_idx);
		if((skef = fopen(chs, "w+")) == NULL){
			fprintf(stdout, " -- Cannot create %s in %s -- %s:%d --\n", chs, __FUNCTION__, __FILE__, __LINE__); fflush(stdout); abort();
		}
	}
	for(i=0;i+1<sdb->n_part;i++){ fseek(aln_parts[i], 0, SEEK_SET); }
	for(i=mall->rid_beg+t_idx;i<mall->rid_end;i+=n_cpu){
		m ++;
		if((m % 1000) == 0){
			t2 = time(NULL); fprintf(log, "[THREAD%03u] assembled %u / %u, %u sec. \n", t_idx, n, m, (unsigned)(t2 - t1)); fflush(log);
		}
		sr_clear_aux(aux1); sr_align_sdb(sdb, 2 * i + 0, aux1);
		sr_clear_aux(aux2); sr_align_sdb(sdb, 2 * i + 1, aux2);
		l = count_sr_hitv(aux1->hits);
		r = count_sr_hitv(aux2->hits);
		clear_hitv(hits1);
		for(j=0;j+1<sdb->n_part;j++){
			while(1){ if(read_simphit(&H, aln_parts[j])) push_hitv(hits1, H); else break; }
		}
		for(j=0;j<l;j++) sr_hit2simp(ref_sr_hitv(aux1->hits, j), next_ref_hitv(hits1), 2 * i);
		clear_hitv(hits2);
		for(j=0;j+1<sdb->n_part;j++){
			while(1){ if(read_simphit(&H, aln_parts[j])) push_hitv(hits2, H); else break; }
		}
		for(j=0;j<r;j++) sr_hit2simp(ref_sr_hitv(aux2->hits, j), next_ref_hitv(hits2), 2 * i + 1);
		l = count_hitv(hits1); encap_bitvec(shuffle[0], l);
		r = count_hitv(hits2); encap_bitvec(shuffle[1], r);
		if(l + r > mall->opt->limit){
			if(l < mall->opt->limit / 2){
				ll = l; ones_bitvec(shuffle[0]);
				rr = shuffle_hits(shuffle[1], hits2, mall->opt->limit - l);
			} else if(r < mall->opt->limit / 2){
				ll = shuffle_hits(shuffle[0], hits1, mall->opt->limit - r);
				rr = r; ones_bitvec(shuffle[1]);
			} else {
				ll = shuffle_hits(shuffle[0], hits1, mall->opt->limit / 2);
				rr = shuffle_hits(shuffle[1], hits2, mall->opt->limit / 2);
			}
		} else {
			ll = l; rr = r;
			ones_bitvec(shuffle[0]); ones_bitvec(shuffle[1]);
		}
		reset_lgraph(g);
		rd_off1 = sr_rdseq_offset(sdb, 2 * i); rd_len1 = sr_rdseq_length(sdb, 2 * i);
		rd_off2 = sr_rdseq_offset(sdb, 2 * i + 1); rd_len2 = sr_rdseq_length(sdb, 2 * i + 1);
		bits2seq(seq1, ref_u64list(sdb->rd_seqs, 0), rd_off1, rd_len1);
		bits2revseq(seq2, ref_u64list(sdb->rd_seqs, 0), rd_off2, rd_len2);
		lib_id = get_read_lib(libs, 2 * i);
		min_ins = get_u32list(libs->min_ins, lib_id); max_ins = get_u32list(libs->max_ins, lib_id);
		push_lgraph(g, 2 * i + 0, seq1, rd_len1, 0, 0, 0, 0); push_lgraph(g, 2 * i + 1, seq2, rd_len2, 1, 1, 0, 0);
		l = count_hitv(hits1); r = count_hitv(hits2);
		if(m < n_full) fprintf(alnf, "A\t%u\t%u\t%u\t%s\t%s\n", i, ll, rr, seq1, seq2);
		for(j=0;j<l+r;j++){
			if(j < l){ H = get_hitv(hits1, j); if(get_bitvec(shuffle[0], j) == 0) continue; }
			else { H = get_hitv(hits2, j - l); if(get_bitvec(shuffle[1], j - l) == 0) continue; }
			lib_id = get_read_lib(libs, H.rid);
			min_ins = get_u32list(libs->min_ins, lib_id);
			max_ins = get_u32list(libs->max_ins, lib_id);
			rd_off1 = sr_rdseq_offset(sdb, H.rid); rd_len1 = sr_rdseq_length(sdb, H.rid);
			rd_off2 = sr_rdseq_offset(sdb, H.rid ^ 0x1U); rd_len2 = sr_rdseq_length(sdb, H.rid ^ 0x1U);
			off = (H.sign ^ H.dir1)? - ((int)H.offset) : (int)H.offset;
			if(j < l){
				bits2seq(seq1, sdb->rd_seqs->buffer, rd_off1, rd_len1);
				bits2revseq(seq2, sdb->rd_seqs->buffer, rd_off2, rd_len2);
				push_lgraph(g,        H.rid, seq1, rd_len1,  H.dir1, 0, off, off);
				push_lgraph(g, H.rid ^ 0x1U, seq2, rd_len2, !H.dir1, 0, min_ins + off - rd_len2, max_ins + off - rd_len2);
			} else {
				bits2revseq(seq1, sdb->rd_seqs->buffer, rd_off1, rd_len1);
				bits2seq(seq2, sdb->rd_seqs->buffer, rd_off2, rd_len2);
				push_lgraph(g,        H.rid, seq1, rd_len1, !H.dir1, 1, off, off);
				push_lgraph(g, H.rid ^ 0x1U, seq2, rd_len2,  H.dir1, 1, min_ins + off - rd_len2, max_ins + off - rd_len2);
			}
			if(m < n_full) fprintf(alnf, "%c\t%u\t%d\t%d\t%d\t%u\t%s\t%s\n", "LR"[j<l? 0 : 1], H.rid, off, min_ins, max_ins, H.n_mm, seq1, seq2);
		}
		if(g->opt->flags[1] & 0x01U) align2_lgraph(g); else align_lgraph(g);
		if(m < n_full){ simplify_lgraph(g); print_dot_lgraph(g, dotf); }
		if(layout_lgraph(g)){
			one2bitvec(ps_state);
			skeleton_lgraph(g);
			if(mall->opt->max_hits[2]) align_skeleton_lgraph(g, sdb);
			if(mall->opt->max_hits[2] && sdb->n_part > 1){
				dump_sr_hitv(g->sp_hits, cns_parts[sdb->part_idx]);
				fwrite(&SP, sizeof(SR_AlnHit), 1, cns_parts[sdb->part_idx]);
				fwrite(&g->cns_id, sizeof(uint32_t), 1, skef);
				fwrite(&g->cns_len, sizeof(uint32_t), 1, skef);
				fwrite(&g->n_l, sizeof(uint32_t), 1, skef);
				fwrite(&g->n_r, sizeof(uint32_t), 1, skef);
				fwrite(&g->n_layout, sizeof(uint32_t), 1, skef);
				fwrite(g->skeleton->string, 1, g->skeleton->size, skef);
			} else {
				consensus_lgraph(g);
				output_lgraph(g, cnsf, (m < n_full)? msaf : NULL);
				n ++;
			}
		} else zero2bitvec(ps_state);
		if((m & 0xFF) == 0){
			thread_beg_syn(mall);
			fflush(cnsf); fwrite(cns_buf, ftell(cnsf), 1, mall->cnsf); fseek(cnsf, 0, SEEK_SET);
			fflush(msaf); fwrite(msa_buf, ftell(msaf), 1, mall->msaf); fseek(msaf, 0, SEEK_SET);
			fflush(alnf); fwrite(aln_buf, ftell(alnf), 1, mall->alnf); fseek(alnf, 0, SEEK_SET);
			fflush(dotf); fwrite(dot_buf, ftell(dotf), 1, mall->dotf); fseek(dotf, 0, SEEK_SET);
			thread_end_syn(mall);
		}
	}
	t2 = time(NULL);
	fprintf(log, "[THREAD%03u] %u / %u, %u sec. \n", t_idx, n, m, (unsigned)(t2 - t1)); fflush(log);
	if(1){
		thread_beg_syn(mall);
		fflush(cnsf); fwrite(cns_buf, ftell(cnsf), 1, mall->cnsf); fseek(cnsf, 0, SEEK_SET);
		fflush(msaf); fwrite(msa_buf, ftell(msaf), 1, mall->msaf); fseek(msaf, 0, SEEK_SET);
		fflush(alnf); fwrite(aln_buf, ftell(alnf), 1, mall->alnf); fseek(alnf, 0, SEEK_SET);
		fflush(dotf); fwrite(dot_buf, ftell(dotf), 1, mall->dotf); fseek(dotf, 0, SEEK_SET);
		thread_end_syn(mall);
	}
	if(sdb->n_part > 1){
		fflush(skef); fflush(cns_parts[sdb->part_idx]);
		for(i=0;i+1<sdb->n_part;i++){
			fclose(aln_parts[i]);
			sprintf(chs, "%s.aln.p%03u.t%03u", mall->prefix, i, t_idx);
			unlink(chs);
		}
	}
} else if(mall->task == 4){
	t1 = time(NULL);
	sprintf(chs, "%s.cns.p%03u.t%03u", mall->prefix, sdb->part_idx, t_idx);
	if((cns_parts[sdb->part_idx] = fopen(chs, "w+")) == NULL){
		fprintf(stdout, " -- Cannot create %s in %s -- %s:%d --\n", chs, __FUNCTION__, __FILE__, __LINE__); fflush(stdout); abort();
	}
	m = n = 0;
	fseek(skef, 0, SEEK_SET);
	for(i=mall->rid_beg+t_idx;i<mall->rid_end;i+=n_cpu){
		m ++;
		if((m % 1000) == 0){
			t2 = time(NULL); fprintf(log, "[THREAD%03u] aligning (long) %u, %u sec. \n", t_idx, m, (unsigned)(t2 - t1)); fflush(log);
		}
		if(get_bitvec(ps_state, m) == 0) continue;
		n ++;
		reset_lgraph(g);
		fread(&g->cns_id, sizeof(uint32_t), 1, skef);
		fread(&g->cns_len, sizeof(uint32_t), 1, skef);
		fread(&g->n_l, sizeof(uint32_t), 1, skef);
		fread(&g->n_r, sizeof(uint32_t), 1, skef);
		fread(&g->n_layout, sizeof(uint32_t), 1, skef);
		encap_string(g->skeleton, g->cns_len);
		fread(g->skeleton->string, 1, g->cns_len, skef);
		g->skeleton->string[g->cns_len] = 0;
		g->skeleton->size = g->cns_len;
		align_skeleton_lgraph(g, sdb);
		dump_sr_hitv(g->sp_hits, cns_parts[sdb->part_idx]);
		fwrite(&SP, sizeof(SR_AlnHit), 1, cns_parts[sdb->part_idx]);
	}
} else if(mall->task == 5){
	t1 = time(NULL);
	fseek(skef, 0, SEEK_SET);
	n = m = 0;
	for(i=mall->rid_beg+t_idx;i<mall->rid_end;i+=n_cpu){
		m ++;
		if((m % 1000) == 0){
			t2 = time(NULL); fprintf(log, "[THREAD%03u] consensus %u / %u, %u sec. \n", t_idx, n, m, (unsigned)(t2 - t1)); fflush(log);
		}
		if(get_bitvec(ps_state, m) == 0) continue;
		n ++;
		reset_lgraph(g);
		fread(&g->cns_id, sizeof(uint32_t), 1, skef);
		fread(&g->cns_len, sizeof(uint32_t), 1, skef);
		fread(&g->n_l, sizeof(uint32_t), 1, skef);
		fread(&g->n_r, sizeof(uint32_t), 1, skef);
		fread(&g->n_layout, sizeof(uint32_t), 1, skef);
		encap_string(g->skeleton, g->cns_len);
		fread(g->skeleton->string, 1, g->cns_len, skef);
		g->skeleton->string[g->cns_len] = 0;
		g->skeleton->size = g->cns_len;
		align_skeleton_lgraph(g, sdb);
		for(j=0;j+1<sdb->n_part;j++){
			l = (j + sdb->n_part - 1) % sdb->n_part;
			while(fread(&SP, sizeof(SR_AlnHit), 1, cns_parts[l]) == 1){ if(SP.n_ol == 0) break; push_sr_hitv(g->sp_hits, SP); }
		}
		consensus_lgraph(g);
		output_lgraph(g, cnsf, NULL);
		if((n & 0xFF) == 0){
			thread_beg_syn(mall);
			fflush(cnsf); fwrite(cns_buf, ftell(cnsf), 1, mall->cnsf); fseek(cnsf, 0, SEEK_SET);
			fflush(msaf); fwrite(msa_buf, ftell(msaf), 1, mall->msaf); fseek(msaf, 0, SEEK_SET);
			fflush(alnf); fwrite(aln_buf, ftell(alnf), 1, mall->alnf); fseek(alnf, 0, SEEK_SET);
			fflush(dotf); fwrite(dot_buf, ftell(dotf), 1, mall->dotf); fseek(dotf, 0, SEEK_SET);
			thread_end_syn(mall);
		}
	}
	if(1){
		thread_beg_syn(mall);
		fflush(cnsf); fwrite(cns_buf, ftell(cnsf), 1, mall->cnsf); fseek(cnsf, 0, SEEK_SET);
		fflush(msaf); fwrite(msa_buf, ftell(msaf), 1, mall->msaf); fseek(msaf, 0, SEEK_SET);
		fflush(alnf); fwrite(aln_buf, ftell(alnf), 1, mall->alnf); fseek(alnf, 0, SEEK_SET);
		fflush(dotf); fwrite(dot_buf, ftell(dotf), 1, mall->dotf); fseek(dotf, 0, SEEK_SET);
		thread_end_syn(mall);
	}
	sprintf(chs, "%s.cns.ske.t%03u", mall->prefix, t_idx);
	unlink(chs);
	for(j=0;j+1<sdb->n_part;j++){
		l = (j + sdb->n_part - 1) % sdb->n_part;
		fclose(cns_parts[l]);
		sprintf(chs, "%s.cns.p%03u.t%03u", mall->prefix, l, t_idx);
		unlink(chs);
	}
} else {
	microsleep(1);
}
thread_end_loop(mall);
sr_free_aux(aux1);
sr_free_aux(aux2);
free_hitv(hits1);
free_hitv(hits2);
free_bitvec(shuffle[0]);
free_bitvec(shuffle[1]);
free_bitvec(ps_state);
fclose(cnsf); fclose(msaf);
if(alnf) fclose(alnf);
if(cns_buf) free(cns_buf);
if(msa_buf) free(msa_buf);
if(aln_buf) free(aln_buf);
if(sdb->n_part > 1){ free(aln_parts); free(cns_parts); }
thread_end_func(mall);

int debug_assembling(ATOptions opt, char *prefix){
	FileReader *fr;
	LGraph *g;
	FILE *cnsf, *msaf;
	uint32_t id, n_lr, rd_len1, rd_len2, m, p;
	int n, off, max_ins, min_ins;
	char *seq1, *seq2;
	time_t t1, t2;
	char cmd[256];
	if((fr = fopen_filereader(opt.load_aln)) == NULL){
		fprintf(stderr, " -- Cannot open '%s' in %s -- %s:%d --\n", opt.load_aln, __FUNCTION__, __FILE__, __LINE__);
		fflush(stderr);
		abort();
	}
	g = init_lgraph(&opt);
	sprintf(cmd, "%s.fasta", prefix);
	if((cnsf = fopen(cmd, "w")) == NULL){
		fprintf(stderr, " -- Cannot open %s in %s -- %s:%d --\n", cmd, __FUNCTION__, __FILE__, __LINE__);
		fflush(stderr);
		return 1;
	}
	sprintf(cmd, "gzip -c -1 > %s.msa.gz", prefix);
	if((msaf = popen(cmd, "w")) == NULL){
		fprintf(stderr, " -- Cannot open '%s' in %s -- %s:%d --\n", cmd, __FUNCTION__, __FILE__, __LINE__);
		fflush(stderr);
		return 1;
	}
	//print_options(&opt, stdout);
	m = p = 0;
	t1 = time(NULL);
	while(1){
		if((n = fread_table(fr)) == -1) break;
		m ++;
		if((m % 100) == 0){
			t2 = time(NULL);
			fprintf(stderr, "[%s] %u / %u clusters, %u sec. \n", __FUNCTION__, p, m, (unsigned)(t2 - t1)); fflush(stderr);
		}
		reset_lgraph(g);
		id = atoll(get_col_str(fr, 1));
		n_lr = atoi(get_col_str(fr, 2)) + atoi(get_col_str(fr, 3));
		push_lgraph(g, 2 * id + 0, get_col_str(fr, 4), get_col_len(fr, 4), 0, 0, 0, 0);
		push_lgraph(g, 2 * id + 1, get_col_str(fr, 5), get_col_len(fr, 5), 1, 1, 0, 0);
		while(fread_table(fr) != -1){
			if(get_col_str(fr, 0)[0] == 'A'){ froll_back(fr); break; }
			id = atoll(get_col_str(fr, 1));
			off = atoi(get_col_str(fr, 2));
			min_ins = atoi(get_col_str(fr, 3));
			max_ins = atoi(get_col_str(fr, 4));
			seq1 = get_col_str(fr, 6);
			seq2 = get_col_str(fr, 7);
			rd_len1 = get_col_len(fr, 6);
			rd_len2 = get_col_len(fr, 7);
			push_lgraph(g,        id, seq1, rd_len1, 0, (get_col_str(fr, 0)[0] == 'R'), off, off);
			push_lgraph(g, id ^ 0x1U, seq2, rd_len2, 0, (get_col_str(fr, 0)[0] == 'R'), min_ins + off - rd_len2, max_ins + off - rd_len2);
		}
		if(g->opt->flags[1] & 0x01U) align2_lgraph(g); else align_lgraph(g);
		if(layout_lgraph(g)){
			p ++;
			skeleton_lgraph(g);
			consensus_lgraph(g);
			output_lgraph(g, cnsf, msaf);
			fflush(cnsf); fflush(msaf);
		}
	}
	free_lgraph(g);
	fclose(cnsf);
	pclose(msaf);
	fclose_filereader(fr);
	return 0;
}

int main_all(int argc, char **argv){
	ATOptions opt;
	SR_SeqDB *sdb;
	sfv *sfs;
	SeqFile *sf1, *sf2;
	LibInserts *libs;
	FileReader *s1, *s2;
	Sequence *seq1, *seq2;
	FILE *log, *cnsf, *msaf, *alnf, *dotf;
	char *prefix, cmd[256], *date;
	uint32_t i, rid, rrid, n_vis, p;
	int ii, c, vis;
	time_t tm;
	thread_preprocess(mall);
	if((c = parse_options(&opt, 0, argc, argv)) == -1) return usage_all();
	if(opt.load_aln){
		if(c + 1 < argc) return usage_all();
		return debug_assembling(opt, argv[c]);
	}
	if(c == argc || (argc - c) % 2 != 1) return usage_all();
	prefix = argv[c];
	sfs = init_sfv(4);
	for(ii=c+1;ii<argc;ii++){
		sf1 = next_ref_sfv(sfs);
		parse_seqfile(sf1, argv[ii], opt.var_size);
	}
	qsort(ref_sfv(sfs, 0), count_sfv(sfs), sizeof(SeqFile), cmp_seqfile);
	if(opt.min_lib == 0) opt.min_lib = ref_sfv(sfs, 0)->ins_size;
	for(i=0;2*i<count_sfv(sfs);i++){
		if(strcasecmp(ref_sfv(sfs, 2 * i)->prefix, ref_sfv(sfs, 2 * i + 1)->prefix)
			|| ref_sfv(sfs, 2 * i)->ins_size != ref_sfv(sfs, 2 * i + 1)->ins_size
			|| ref_sfv(sfs, 2 * i)->pair_idx >= ref_sfv(sfs, 2 * i + 1)->pair_idx){
			fprintf(stderr, " -- Wrong pair \"%s\" and \"%s\" in %s -- %s:%d --\n", ref_sfv(sfs, 2 * i)->name, ref_sfv(sfs, 2 * i + 1)->name, __FUNCTION__, __FILE__, __LINE__);
			abort();
		}
	}
	libs = guess_lib_inserts(sfs);
	sdb = sr_init_sdb(opt.kmer_size[0], opt.n_seed[0], opt.rd_len, opt.low_cpx);
	sr_set_n_part(sdb, opt.n_part);
	sprintf(cmd, "%s.info", prefix);
	if((log = fopen(cmd, "w")) == NULL){
		fprintf(stderr, " -- Cannot open '%s' in %s -- %s:%d --\n", cmd, __FUNCTION__, __FILE__, __LINE__); fflush(stderr);
		abort();
	}
	sprintf(cmd, "%s.fasta", prefix);
	if((cnsf = fopen(cmd, "w")) == NULL){
		fprintf(stderr, " -- Cannot open %s in %s -- %s:%d --\n", cmd, __FUNCTION__, __FILE__, __LINE__); fflush(stderr); return 1;
	}
	sprintf(cmd, "gzip -1 -c >%s.msa.gz", prefix);
	if((msaf = popen(cmd, "w")) == NULL){
		fprintf(stderr, " -- Cannot open %s in %s -- %s:%d --\n", cmd, __FUNCTION__, __FILE__, __LINE__); fflush(stderr); return 1;
	}
	sprintf(cmd, "gzip -1 -c >%s.aln.gz", prefix);
	if((alnf = popen(cmd, "w")) == NULL){
		fprintf(stderr, " -- Cannot open %s in %s -- %s:%d --\n", cmd, __FUNCTION__, __FILE__, __LINE__); fflush(stderr); return 1;
	}
	sprintf(cmd, "gzip -1 -c >%s.dot.gz", prefix);
	if((dotf = popen(cmd, "w")) == NULL){
		fprintf(stderr, " -- Cannot open %s in %s -- %s:%d --\n", cmd, __FUNCTION__, __FILE__, __LINE__); fflush(stderr); return 1;
	}
	print_options(&opt, log);
	rid = 0;
	tm = time(NULL); date = asctime(localtime(&tm));
	fprintf(log, "%s\n", date); fflush(log);
	for(i=0;i<libs->n_lib;i++){
		fprintf(log, "Lib%u: %u - %u bp\n", i, get_u32list(libs->min_ins, i), get_u32list(libs->max_ins, i));
	}
	fflush(log);
	n_vis = 0;
	for(i=0;2*i<count_sfv(sfs);i++){
		set_u32list(libs->lib_offs, i, rid * 2);
		seq1 = seq2 = NULL;
		sf1 = ref_sfv(sfs, 2 * i);
		sf2 = ref_sfv(sfs, 2 * i + 1);
		vis = (sf1->ins_size >= (int)opt.min_lib);
		if((s1 = fopen_filereader(sf1->name)) == NULL){
			fprintf(stderr, " -- Cannot open %s in %s -- %s:%d --\n", sf1->name, __FUNCTION__, __FILE__, __LINE__); abort();
		}
		if((s2 = fopen_filereader(sf2->name)) == NULL){
			fprintf(stderr, " -- Cannot open %s in %s -- %s:%d --\n", sf2->name, __FUNCTION__, __FILE__, __LINE__); abort();
		}
		rrid = rid;
		while(1){
			if(!((sf1->is_fq)? fread_fastq_adv(&seq1, s1, FASTQ_FLAG_NO_NAME | FASTQ_FLAG_NO_QUAL) : fread_fasta_adv(&seq1, s1, FASTA_FLAG_NO_NAME))) break;
			if(!((sf2->is_fq)? fread_fastq_adv(&seq2, s2, FASTQ_FLAG_NO_NAME | FASTQ_FLAG_NO_QUAL) : fread_fasta_adv(&seq2, s2, FASTA_FLAG_NO_NAME))) break;
			if(seq1->seq.size > (int)opt.rd_clip && seq2->seq.size > (int)opt.rd_clip){
				sr_push_sdb(sdb, seq1->seq.string + opt.rd_clip, seq1->seq.size - opt.rd_clip);
				sr_push_sdb(sdb, seq2->seq.string + opt.rd_clip, seq2->seq.size - opt.rd_clip);
			}
			rid ++;
		}
		if(seq1) free_sequence(seq1);
		if(seq2) free_sequence(seq2);
		fclose_filereader(s1);
		fclose_filereader(s2);
		fprintf(log, "%s\t%d\t%d\t%d\t%d\t%s\n", sf1->prefix, sf1->ins_size, rid - rrid, rrid, rid, vis? "yes" : "no"); fflush(log);
		if(vis) n_vis = rid;
	}
	tm = time(NULL); date = asctime(localtime(&tm));
	fprintf(log, "%s\n", date);
	fflush(log);
	sr_ready_sdb(sdb);
	if(opt.inc_ar == 0) sr_set_n_ar(sdb, 2 * n_vis);
	thread_beg_init(mall, (int)opt.n_cpu);
	mall->opt = &opt;
	mall->sdb = sdb;
	mall->g = init_lgraph(&opt);
	mall->libs = libs;
	mall->rid_beg = 0;
	mall->rid_end = n_vis;
	mall->log  = log;
	mall->cnsf = cnsf;
	mall->msaf = msaf;
	mall->alnf = alnf;
	mall->dotf = dotf;
	mall->task    = 0;
	mall->prefix  = prefix;
	thread_end_init(mall);
	if(opt.n_part > 1){
		for(p=0;p<opt.n_part;p++){
			sr_set_sr_part(sdb, p);
			fprintf(log, "Indexing [%u/%u]\n", p, opt.n_part); fflush(log);
			thread_beg_iter(mall);
			mall->task = 1; // index
			thread_wake(mall);
			thread_end_iter(mall);
			thread_waitfor_all_idle(mall);
			tm = time(NULL); date = asctime(localtime(&tm));
			fprintf(log, "%s\n", date);
			if(p + 1 == opt.n_part){
				fprintf(log, "Local assembling\n");
			} else {
				fprintf(log, "Writting temporary alignments\n");
			}
			fflush(log);
			thread_beg_iter(mall);
			mall->task = (p + 1 < opt.n_part)? 2 : 3; // align and asm
			thread_wake(mall);
			thread_end_iter(mall);
			thread_waitfor_all_idle(mall);
		}
		for(p=0;opt.max_hits[2]&&p+1<opt.n_part;p++){
			sr_set_sr_part(sdb, p);
			fprintf(log, "Indexing [%u/%u]\n", p, opt.n_part); fflush(log);
			thread_beg_iter(mall);
			mall->task = 1; // index
			thread_wake(mall);
			thread_end_iter(mall);
			thread_waitfor_all_idle(mall);
			tm = time(NULL); date = asctime(localtime(&tm));
			fprintf(log, "%s\n", date);
			if(p + 2 == opt.n_part){
				fprintf(log, "Consensus");
			} else {
				fprintf(log, "Aligning long sequence for consensus\n");
			}
			fflush(log);
			thread_beg_iter(mall);
			mall->task = (p + 2 < opt.n_part)? 4 : 5;
			thread_wake(mall);
			thread_end_iter(mall);
			thread_waitfor_all_idle(mall);
		}
	} else {
		fprintf(log, "Indexing\n"); fflush(log);
		thread_beg_iter(mall);
		mall->task = 1; // index
		thread_wake(mall);
		thread_end_iter(mall);
		thread_waitfor_all_idle(mall);
		tm = time(NULL); date = asctime(localtime(&tm));
		fprintf(log, "%s\n", date);
		fprintf(log, "Local assembling\n"); fflush(log);
		thread_beg_iter(mall);
		mall->task = 3; // align and asm
		thread_wake(mall);
		thread_end_iter(mall);
		thread_waitfor_all_idle(mall);
	}
	thread_beg_close(mall);
	free_lgraph(mall->g);
	thread_end_close(mall);
	sr_free_sdb(sdb);
	free_lib_inserts(libs);
	for(i=0;i<count_sfv(sfs);i++){ free(ref_sfv(sfs, i)->name); free(ref_sfv(sfs, i)->prefix); }
	free_sfv(sfs);
	tm = time(NULL); date = asctime(localtime(&tm));
	fprintf(log, "%s\n", date); fflush(log);
	fclose(log);
	fclose(cnsf);
	pclose(msaf);
	if(alnf) pclose(alnf);
	return 0;
}

