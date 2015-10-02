/*************************************************************************/ /*
 VSPM

 Copyright (C) 2015 Renesas Electronics Corporation

 License        Dual MIT/GPLv2

 The contents of this file are subject to the MIT license as set out below.

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 Alternatively, the contents of this file may be used under the terms of
 the GNU General Public License Version 2 ("GPL") in which case the provisions
 of GPL are applicable instead of those above.

 If you wish to allow use of your version of this file only under the terms of
 GPL, and not to allow others to use your version of this file under the terms
 of the MIT license, indicate your decision by deleting the provisions above
 and replace them with the notice and other provisions required by GPL as set
 out in the file called "GPL-COPYING" included in this distribution. If you do
 not delete the provisions above, a recipient may use your version of this file
 under the terms of either the MIT license or GPL.

 This License is also included in this distribution in the file called
 "MIT-COPYING".

 EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
 PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
 COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


 GPLv2:
 If you wish to use this file under the terms of GPL, following terms are
 effective.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; version 2 of the License.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/ /*************************************************************************/

#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>

#include "vspm_public.h"
#include "vspm_if.h"
#include "vspm_if_local.h"

static int set_vsp_src_clut_par(
	struct vspm_entry_vsp_in_clut *clut, struct vsp_dl_t *src)
{
	dma_addr_t hard_addr;
	void *virt_addr;

	/* copy vsp_dl_t parameter */
	if (copy_from_user(
			&clut->clut,
			(void __user *)src,
			sizeof(struct vsp_dl_t))) {
		EPRINT("failed to copy of vsp_dl_t\n");
		return -EFAULT;
	}

	if ((clut->clut.virt_addr != NULL) &&
		(clut->clut.tbl_num > 0) &&
		(clut->clut.tbl_num <= 256)) {
		/* allocate memory */
		virt_addr = dma_alloc_coherent(
			dev,
			clut->clut.tbl_num * 8,
			&hard_addr,
			GFP_KERNEL | GFP_DMA);
		if (virt_addr == NULL) {
			EPRINT("failed to allocate memory\n");
			return -EFAULT;
		}

		/* copy color table */
		if (copy_from_user(
				virt_addr,
				(void __user *)clut->clut.virt_addr,
				clut->clut.tbl_num * 8)) {
			EPRINT("failed to copy of color table\n");
			dma_free_coherent(
				dev,
				clut->clut.tbl_num * 8,
				virt_addr,
				hard_addr);
			return -EFAULT;
		}

		/* set parameter */
		clut->clut.hard_addr = (void *)hard_addr;
		clut->clut.virt_addr = virt_addr;

		clut->hard_addr = hard_addr;
		clut->virt_addr = virt_addr;
	}

	return 0;
}


static int set_vsp_src_alpha_par(
	struct vspm_entry_vsp_in_alpha *alpha, struct vsp_alpha_unit_t *src)
{
	/* copy vsp_alpha_unit_t parameter */
	if (copy_from_user(
			&alpha->alpha,
			(void __user *)src,
			sizeof(struct vsp_alpha_unit_t))) {
		EPRINT("failed to copy of vsp_alpha_unit_t\n");
		return -EFAULT;
	}

	/* copy vsp_irop_unit_t paramerter */
	if (alpha->alpha.irop) {
		if (copy_from_user(
				&alpha->irop,
				(void __user *)alpha->alpha.irop,
				sizeof(struct vsp_irop_unit_t))) {
			EPRINT("failed to copy of vsp_irop_unit_t\n");
			return -EFAULT;
		}
		alpha->alpha.irop = &alpha->irop;
	}

	/* copy vsp_ckey_unit_t paramerter */
	if (alpha->alpha.ckey) {
		if (copy_from_user(
				&alpha->ckey,
				(void __user *)alpha->alpha.ckey,
				sizeof(struct vsp_ckey_unit_t))) {
			EPRINT("failed to copy of vsp_ckey_unit_t\n");
			return -EFAULT;
		}
		alpha->alpha.ckey = &alpha->ckey;
	}

	/* copy vsp_mult_unit_t paramerter */
	if (alpha->alpha.mult) {
		if (copy_from_user(
				&alpha->mult,
				(void __user *)alpha->alpha.mult,
				sizeof(struct vsp_mult_unit_t))) {
			EPRINT("failed to copy of vsp_mult_unit_t\n");
			return -EFAULT;
		}
		alpha->alpha.mult = &alpha->mult;
	}

	return 0;
}

static int set_vsp_src_par(struct vspm_entry_vsp_in *in, struct vsp_src_t *src)
{
	int ercd;

	/* copy vsp_src_t parameter */
	if (copy_from_user(
			&in->in,
			(void __user *)src,
			sizeof(struct vsp_src_t))) {
		EPRINT("failed to copy of vsp_src_t\n");
		return -EFAULT;
	}

	/* copy vsp_dl_t parameter */
	if (in->in.clut) {
		ercd = set_vsp_src_clut_par(&in->clut, in->in.clut);
		if (ercd)
			return ercd;
		in->in.clut = &in->clut.clut;
	}

	/* copy vsp_alpha_unit_t parameter */
	if (in->in.alpha) {
		ercd = set_vsp_src_alpha_par(&in->alpha, in->in.alpha);
		if (ercd)
			return ercd;
		in->in.alpha = &in->alpha.alpha;
	}

	return 0;
}


static int set_vsp_dst_par(
	struct vspm_entry_vsp_out *out, struct vsp_dst_t *src)
{
	/* copy vsp_dst_t parameter */
	if (copy_from_user(
			&out->out,
			(void __user *)src,
			sizeof(struct vsp_dst_t))) {
		EPRINT("failed to copy of vsp_dst_t\n");
		return -EFAULT;
	}

	/* copy fcp_info_t parameter */
	if (out->out.fcp) {
		if (copy_from_user(
				&out->fcp,
				(void __user *)out->out.fcp,
				sizeof(struct fcp_info_t))) {
			EPRINT("failed to copy of fcp_info_t\n");
			return -EFAULT;
		}
		out->out.fcp = &out->fcp;
	}

	return 0;
}


static int set_vsp_bru_par(
	struct vspm_entry_vsp_bru *bru, struct vsp_bru_t *src)
{
	struct vsp_bld_ctrl_t * *(src_blend[5]);
	int i;

	/* copy vsp_bru_t parameter */
	if (copy_from_user(
			&bru->bru,
			(void __user *)src,
			sizeof(struct vsp_bru_t))) {
		EPRINT("failed to copy of vsp_bru_t\n");
		return -EFAULT;
	}

	/* copy vsp_bld_dither_t parameter */
	for (i = 0; i < 5; i++) {
		if (bru->bru.dither_unit[i]) {
			if (copy_from_user(
					&bru->dither_unit[i],
					(void __user *)bru->bru.dither_unit[i],
					sizeof(struct vsp_bld_dither_t))) {
				EPRINT("failed to copy of vsp_bld_dither_t\n");
				return -EFAULT;
			}
			bru->bru.dither_unit[i] = &bru->dither_unit[i];
		}
	}

	/* copy vsp_bld_vir_t parameter */
	if (bru->bru.blend_virtual) {
		if (copy_from_user(
				&bru->blend_virtual,
				(void __user *)bru->bru.blend_virtual,
				sizeof(struct vsp_bld_vir_t))) {
			EPRINT("failed to copy of vsp_bld_vir_t\n");
			return -EFAULT;
		}
		bru->bru.blend_virtual = &bru->blend_virtual;
	}

	src_blend[0] = &bru->bru.blend_unit_a;
	src_blend[1] = &bru->bru.blend_unit_b;
	src_blend[2] = &bru->bru.blend_unit_c;
	src_blend[3] = &bru->bru.blend_unit_d;
	src_blend[4] = &bru->bru.blend_unit_e;

	/* copy vsp_bld_ctrl_t parameter */
	for (i = 0; i < 5; i++) {
		if (*src_blend[i]) {
			if (copy_from_user(
					&bru->blend_unit[i],
					(void __user *)*src_blend[i],
					sizeof(struct vsp_bld_ctrl_t))) {
				EPRINT(
					"failed to copy of vsp_bld_ctrl_t\n");
				return -EFAULT;
			}
			*src_blend[i] = &bru->blend_unit[i];
		}
	}

	/* copy vsp_bld_rop_t parameter */
	if (bru->bru.rop_unit) {
		if (copy_from_user(
				&bru->rop_unit,
				(void __user *)bru->bru.rop_unit,
				sizeof(struct vsp_bld_rop_t))) {
			EPRINT("failed to copy of vsp_bld_rop_t\n");
			return -EFAULT;
		}
		bru->bru.rop_unit = &bru->rop_unit;
	}

	return 0;
}

static int set_vsp_hgo_par(
	struct vspm_entry_vsp_hgo *hgo, struct vsp_hgo_t *src)
{
	dma_addr_t hard_addr;
	void *virt_addr;
	unsigned long tmp_addr;

	/* copy vsp_hgo_t parameter */
	if (copy_from_user(
			&hgo->hgo,
			(void __user *)src,
			sizeof(struct vsp_hgo_t))) {
		EPRINT("failed to copy of vsp_hgo_t\n");
		return -EFAULT;
	}
	hgo->user_addr = hgo->hgo.virt_addr;

	/* allocate memory */
	virt_addr = dma_alloc_coherent(
		dev, 1088, &hard_addr, GFP_KERNEL | GFP_DMA);
	if (virt_addr == NULL) {
		EPRINT("failed to allocate memory\n");
		return -EFAULT;
	}

	/* set parameter */
	tmp_addr = (unsigned long)(hard_addr + 255) >> 8;
	hgo->hgo.hard_addr = (void *)(tmp_addr << 8);
	tmp_addr = (unsigned long)(virt_addr + 255) >> 8;
	hgo->hgo.virt_addr = (void *)(tmp_addr << 8);

	hgo->hard_addr = hard_addr;
	hgo->virt_addr = virt_addr;

	return 0;
}

static int set_vsp_hgt_par(
	struct vspm_entry_vsp_hgt *hgt, struct vsp_hgt_t *src)
{
	dma_addr_t hard_addr;
	void *virt_addr;
	unsigned long tmp_addr;

	/* copy vsp_hgt_t parameter */
	if (copy_from_user(
			&hgt->hgt,
			(void __user *)src,
			sizeof(struct vsp_hgt_t))) {
		EPRINT("failed to copy of vsp_hgt_t\n");
		return -EFAULT;
	}
	hgt->user_addr = hgt->hgt.virt_addr;

	/* allocate memory */
	virt_addr = dma_alloc_coherent(
		dev, 800, &hard_addr, GFP_KERNEL | GFP_DMA);
	if (virt_addr == NULL) {
		EPRINT("failed to allocate memory\n");
		return -EFAULT;
	}

	/* set parameter */
	tmp_addr = (unsigned long)(hard_addr + 255) >> 8;
	hgt->hgt.hard_addr = (void *)(tmp_addr << 8);
	tmp_addr = (unsigned long)(virt_addr + 255) >> 8;
	hgt->hgt.virt_addr = (void *)(tmp_addr << 8);

	hgt->hard_addr = hard_addr;
	hgt->virt_addr = virt_addr;

	return 0;
}

static int set_vsp_ctrl_par(
	struct vspm_entry_vsp_ctrl *ctrl, struct vsp_ctrl_t *src)
{
	int ercd;

	/* copy vsp_ctrl_t parameter */
	if (copy_from_user(
			&ctrl->ctrl,
			(void __user *)src,
			sizeof(struct vsp_ctrl_t))) {
		EPRINT("failed to copy of vsp_ctrl_t\n");
		return -EFAULT;
	}

	/* copy vsp_sru_t parameter */
	if (ctrl->ctrl.sru) {
		if (copy_from_user(
				&ctrl->sru,
				(void __user *)ctrl->ctrl.sru,
				sizeof(struct vsp_sru_t))) {
			EPRINT("failed to copy of vsp_sru_t\n");
			return -EFAULT;
		}
		ctrl->ctrl.sru = &ctrl->sru;
	}

	/* copy vsp_uds_t parameter */
	if (ctrl->ctrl.uds) {
		if (copy_from_user(
				&ctrl->uds,
				(void __user *)ctrl->ctrl.uds,
				sizeof(struct vsp_uds_t))) {
			EPRINT("failed to copy of vsp_uds_t\n");
			return -EFAULT;
		}
		ctrl->ctrl.uds = &ctrl->uds;
	}

	/* copy vsp_lut_t parameter */
	if (ctrl->ctrl.lut) {
		if (copy_from_user(
				&ctrl->lut,
				(void __user *)ctrl->ctrl.lut,
				sizeof(struct vsp_lut_t))) {
			EPRINT("failed to copy of vsp_lut_t\n");
			return -EFAULT;
		}
		ctrl->ctrl.lut = &ctrl->lut;
	}

	/* copy vsp_clu_t parameter */
	if (ctrl->ctrl.clu) {
		if (copy_from_user(
				&ctrl->clu,
				(void __user *)ctrl->ctrl.clu,
				sizeof(struct vsp_clu_t))) {
			EPRINT("failed to copy of vsp_clu_t\n");
			return -EFAULT;
		}
		ctrl->ctrl.clu = &ctrl->clu;
	}

	/* copy vsp_hst_t parameter */
	if (ctrl->ctrl.hst) {
		if (copy_from_user(
				&ctrl->hst,
				(void __user *)ctrl->ctrl.hst,
				sizeof(struct vsp_hst_t))) {
			EPRINT("failed to copy of vsp_hst_t\n");
			return -EFAULT;
		}
		ctrl->ctrl.hst = &ctrl->hst;
	}

	/* copy vsp_hsi_t parameter */
	if (ctrl->ctrl.hsi) {
		if (copy_from_user(
				&ctrl->hsi,
				(void __user *)ctrl->ctrl.hsi,
				sizeof(struct vsp_hsi_t))) {
			EPRINT("failed to copy of vsp_hsi_t\n");
			return -EFAULT;
		}
		ctrl->ctrl.hsi = &ctrl->hsi;
	}

	/* copy vsp_bru_t parameter */
	if (ctrl->ctrl.bru) {
		ercd = set_vsp_bru_par(&ctrl->bru, ctrl->ctrl.bru);
		if (ercd)
			return ercd;
		ctrl->ctrl.bru = &ctrl->bru.bru;
	}

	/* copy vsp_hgo_t parameter */
	if (ctrl->ctrl.hgo) {
		ercd = set_vsp_hgo_par(&ctrl->hgo, ctrl->ctrl.hgo);
		if (ercd)
			return ercd;
		ctrl->ctrl.hgo = &ctrl->hgo.hgo;
	}

	/* copy vsp_hgt_t parameter */
	if (ctrl->ctrl.hgt) {
		ercd = set_vsp_hgt_par(&ctrl->hgt, ctrl->ctrl.hgt);
		if (ercd)
			return ercd;
		ctrl->ctrl.hgt = &ctrl->hgt.hgt;
	}

	/* copy vsp_shp_t parameter */
	if (ctrl->ctrl.shp) {
		if (copy_from_user(
				&ctrl->shp,
				(void __user *)ctrl->ctrl.shp,
				sizeof(struct vsp_shp_t))) {
			EPRINT("failed to copy of vsp_shp_t\n");
			return -EFAULT;
		}
		ctrl->ctrl.shp = &ctrl->shp;
	}

	/* copy vsp_drc_t parameter */
	if (ctrl->ctrl.drc) {
		if (copy_from_user(
				&ctrl->drc,
				(void __user *)ctrl->ctrl.drc,
				sizeof(struct vsp_drc_t))) {
			EPRINT("failed to copy of vsp_drc_t\n");
			return -EFAULT;
		}
		ctrl->ctrl.drc = &ctrl->drc;
	}

	return 0;
}


int free_vsp_par(struct vspm_entry_vsp *vsp)
{
	struct vspm_entry_vsp_in *in = &vsp->in[0];
	struct vspm_entry_vsp_ctrl *ctrl = &vsp->ctrl;

	int i;

	if (vsp->dl.virt_addr != NULL) {
		dma_free_coherent(
			dev,
			vsp->par.dl_par.tbl_num * 8,
			vsp->dl.virt_addr,
			vsp->dl.hard_addr);
		vsp->dl.virt_addr = NULL;
		vsp->dl.hard_addr = 0;
	}

	for (i = 0; i < 5; i++) {
		if (in->clut.virt_addr != NULL) {
			dma_free_coherent(
				dev,
				in->clut.clut.tbl_num * 8,
				in->clut.virt_addr,
				in->clut.hard_addr);
			in->clut.virt_addr = NULL;
			in->clut.hard_addr = 0;
		}
		in++;
	}

	if (ctrl->hgo.virt_addr != NULL) {
		dma_free_coherent(
			dev,
			1088,
			ctrl->hgo.virt_addr,
			ctrl->hgo.hard_addr);
		ctrl->hgo.virt_addr = NULL;
		ctrl->hgo.hard_addr = 0;
	}

	if (ctrl->hgt.virt_addr != NULL) {
		dma_free_coherent(
			dev,
			800,
			ctrl->hgt.virt_addr,
			ctrl->hgt.hard_addr);
		ctrl->hgt.virt_addr = NULL;
		ctrl->hgt.hard_addr = 0;
	}

	return 0;
}


int set_vsp_par(
	struct vspm_if_entry_data_t *entry, struct vsp_start_t *vsp_par)
{
	struct vspm_entry_vsp *vsp = &entry->ip_par.vsp;

	struct vsp_dl_t *dl_par = &vsp->par.dl_par;
	dma_addr_t hard_addr;
	void *virt_addr;

	int ercd = 0;

	int i;

	/* copy vsp_start_t parameter */
	if (copy_from_user(
			&vsp->par,
			(void __user *)vsp_par,
			sizeof(struct vsp_start_t))) {
		EPRINT("failed to copy of vsp_start_t\n");
		return -EFAULT;
	}

	/* allocate memory for display list */
	if ((dl_par->tbl_num > 0) && (dl_par->tbl_num < 16384)) {
		/* allocate memory */
		virt_addr = dma_alloc_coherent(
			dev,
			dl_par->tbl_num * 8,
			&hard_addr,
			GFP_KERNEL | GFP_DMA);
		if (virt_addr == NULL) {
			EPRINT("failed to allocate memory\n");
			return -EFAULT;
		}

		/* set parameter */
		dl_par->hard_addr = (void *)hard_addr;
		dl_par->virt_addr = virt_addr;

		vsp->dl.hard_addr = hard_addr;
		vsp->dl.virt_addr = virt_addr;
	}

	/* copy vsp_src_t parameter */
	for (i = 0; i < 5; i++) {
		if (vsp->par.src_par[i]) {
			ercd = set_vsp_src_par(
				&vsp->in[i], vsp->par.src_par[i]);
			if (ercd)
				goto err_exit;
			vsp->par.src_par[i] = &vsp->in[i].in;
		}
	}

	/* copy vsp_dst_t parameter */
	if (vsp->par.dst_par) {
		ercd = set_vsp_dst_par(&vsp->out, vsp->par.dst_par);
		if (ercd)
			goto err_exit;
		vsp->par.dst_par = &vsp->out.out;
	}

	/* copy vsp_ctrl_t parameter */
	if (vsp->par.ctrl_par) {
		ercd = set_vsp_ctrl_par(&vsp->ctrl, vsp->par.ctrl_par);
		if (ercd)
			goto err_exit;
		vsp->par.ctrl_par = &vsp->ctrl.ctrl;
	}

	return 0;

err_exit:
	free_vsp_par(vsp);
	return ercd;
}

void set_cb_rsp_vsp(
	struct vspm_if_cb_data_t *cb_data,
	struct vspm_if_entry_data_t *entry_data)
{
	struct vspm_entry_vsp_hgo *hgo =
		&entry_data->ip_par.vsp.ctrl.hgo;
	struct vspm_entry_vsp_hgt *hgt =
		&entry_data->ip_par.vsp.ctrl.hgt;

	void *data;

	if (entry_data->ip_par.vsp.par.use_module & VSP_HGO_USE) {
		/* copy response of HGO */
		if (hgo->hgo.virt_addr != NULL) {
			/* allocate memory */
			data = kzalloc(1088, GFP_ATOMIC);
			if (data != NULL) {
				/* copy */
				memcpy(data, hgo->hgo.virt_addr, 1088);

				/* set */
				cb_data->vsp_hgo.user_addr = hgo->user_addr;
				cb_data->vsp_hgo.knel_addr = data;
			} else {
				APRINT("failed to allocate memory of HGO\n");
			}
		}
	}

	if (entry_data->ip_par.vsp.par.use_module & VSP_HGT_USE) {
		/* copy response of HGT */
		if (hgt->hgt.virt_addr != NULL) {
			/* allocate memory */
			data = kzalloc(800, GFP_ATOMIC);
			if (data != NULL) {
				/* copy */
				memcpy(data, hgt->hgt.virt_addr, 800);

				/* set */
				cb_data->vsp_hgt.user_addr = hgt->user_addr;
				cb_data->vsp_hgt.knel_addr = data;
			} else {
				APRINT("failed to allocate memory of HGT\n");
			}
		}
	}
}

static int set_fdp_ref_par(
	struct vspm_entry_fdp_ref *ref, struct fdp_refbuf_t *src)
{
	struct fdp_imgbuf_t * *(src_refbuf[3]);
	int i;

	/* copy fdp_refbuf_t parameter */
	if (copy_from_user(
			&ref->ref_buf,
			(void __user *)src,
			sizeof(struct fdp_refbuf_t))) {
		EPRINT("failed to copy of fdp_refbuf_t\n");
		return -EFAULT;
	}

	src_refbuf[0] = &ref->ref_buf.next_buf;
	src_refbuf[1] = &ref->ref_buf.cur_buf;
	src_refbuf[2] = &ref->ref_buf.prev_buf;

	/* copy fdp_imgbuf_t parameter */
	for (i = 0; i < 3; i++) {
		if (*src_refbuf[i]) {
			if (copy_from_user(
					&ref->ref[i],
					(void __user *)*src_refbuf[i],
					sizeof(struct fdp_imgbuf_t))) {
				EPRINT("failed to copy of fdp_imgbuf_t\n");
				return -EFAULT;
			}
			*src_refbuf[i] = &ref->ref[i];
		}
	}

	return 0;
}


static int set_fdp_fproc_par(
	struct vspm_entry_fdp_fproc *fproc, struct fdp_fproc_t *src)
{
	int ercd;

	/* copy fdp_fproc_t parameter */
	if (copy_from_user(
			&fproc->fproc,
			(void __user *)src,
			sizeof(struct fdp_fproc_t))) {
		EPRINT("failed to copy of fdp_fproc_t\n");
		return -EFAULT;
	}

	/* copy fdp_seq_t parameter */
	if (fproc->fproc.seq_par) {
		if (copy_from_user(
				&fproc->seq,
				(void __user *)fproc->fproc.seq_par,
				sizeof(struct fdp_seq_t))) {
			EPRINT("failed to copy of fdp_seq_t\n");
			return -EFAULT;
		}
		fproc->fproc.seq_par = &fproc->seq;
	}

	/* copy fdp_pic_t parameter */
	if (fproc->fproc.in_pic) {
		if (copy_from_user(
				&fproc->in_pic,
				(void __user *)fproc->fproc.in_pic,
				sizeof(struct fdp_pic_t))) {
			EPRINT("failed to copy of fdp_pic_t\n");
			return -EFAULT;
		}
		fproc->fproc.in_pic = &fproc->in_pic;
	}

	/* copy fdp_imgbuf_t parameter */
	if (fproc->fproc.out_buf) {
		if (copy_from_user(
				&fproc->out_buf,
				(void __user *)fproc->fproc.out_buf,
				sizeof(struct fdp_imgbuf_t))) {
			EPRINT("failed to copy of fdp_imgbuf_t\n");
			return -EFAULT;
		}
		fproc->fproc.out_buf = &fproc->out_buf;
	}

	/* copy fdp_refbuf_t parameter */
	if (fproc->fproc.ref_buf) {
		ercd = set_fdp_ref_par(&fproc->ref, fproc->fproc.ref_buf);
		if (ercd)
			return ercd;
		fproc->fproc.ref_buf = &fproc->ref.ref_buf;
	}

	/* copy fcp_info_t parameter */
	if (fproc->fproc.fcp_par) {
		if (copy_from_user(
				&fproc->fcp,
				(void __user *)fproc->fproc.fcp_par,
				sizeof(struct fcp_info_t))) {
			EPRINT("failed to copy of fcp_info_t\n");
			return -EFAULT;
		}
		fproc->fproc.fcp_par = &fproc->fcp;
	}

	return 0;
}


int set_fdp_par(
	struct vspm_if_entry_data_t *entry, struct fdp_start_t *fdp_par)
{
	struct vspm_entry_fdp *fdp = &entry->ip_par.fdp;
	int ercd;

	/* copy fdp_start_t parameter */
	if (copy_from_user(
			&fdp->par,
			(void __user *)fdp_par,
			sizeof(struct fdp_start_t))) {
		EPRINT("failed to copy of fdp_start_t\n");
		return -EFAULT;
	};

	/* copy fdp_fproc_t parameter */
	if (fdp->par.fproc_par) {
		ercd = set_fdp_fproc_par(&fdp->fproc, fdp->par.fproc_par);
		if (ercd)
			return ercd;
		fdp->par.fproc_par = &fdp->fproc.fproc;
	}

	return 0;
}

static int set_compat_vsp_src_clut_par(
	struct vspm_entry_vsp_in_clut *clut, unsigned int *src)
{
	struct compat_vsp_dl_t compat_dl_par;
	dma_addr_t hard_addr;
	void *virt_addr;

	/* copy */
	if (copy_from_user(
			&compat_dl_par,
			(void __user *)src,
			sizeof(struct compat_vsp_dl_t))) {
		EPRINT("failed to copy of vsp_dl_t\n");
		return -EFAULT;
	}

	if ((compat_dl_par.virt_addr != NULL) &&
		(compat_dl_par.tbl_num > 0) &&
		(compat_dl_par.tbl_num <= 256)) {
		/* allocate memory */
		virt_addr = dma_alloc_coherent(
			dev,
			compat_dl_par.tbl_num * 8,
			&hard_addr,
			GFP_KERNEL | GFP_DMA);
		if (virt_addr == NULL) {
			EPRINT("failed to allocate memory\n");
			return -EFAULT;
		}

		/* copy color table */
		if (copy_from_user(
				virt_addr,
				(void __user *)compat_dl_par.virt_addr,
				compat_dl_par.tbl_num * 8)) {
			EPRINT("failed to copy color table\n");
			dma_free_coherent(
				dev,
				compat_dl_par.tbl_num * 8,
				virt_addr,
				hard_addr);
			return -EFAULT;
		}

		/* set parameter */
		clut->clut.hard_addr = (void *)hard_addr;
		clut->clut.virt_addr = virt_addr;
		clut->clut.tbl_num = compat_dl_par.tbl_num;

		clut->hard_addr = hard_addr;
		clut->virt_addr = virt_addr;
	}

	return 0;
}

static int set_compat_vsp_irop_par(
	struct vsp_irop_unit_t *irop, unsigned int *src)
{
	struct compat_vsp_irop_unit_t compat_irop;

	/* copy */
	if (copy_from_user(
			&compat_irop,
			(void __user *)src,
			sizeof(struct compat_vsp_irop_unit_t))) {
		EPRINT("failed to copy of vsp_irop_unit_t\n");
		return -EFAULT;
	}

	/* set */
	irop->op_mode = compat_irop.op_mode;
	irop->ref_sel = compat_irop.ref_sel;
	irop->bit_sel = compat_irop.bit_sel;
	irop->comp_color = (unsigned long)compat_irop.comp_color;
	irop->irop_color0 = (unsigned long)compat_irop.irop_color0;
	irop->irop_color1 = (unsigned long)compat_irop.irop_color1;

	return 0;
}

static int set_compat_vsp_ckey_par(
	struct vsp_ckey_unit_t *ckey, unsigned int *src)
{
	struct compat_vsp_ckey_unit_t compat_ckey;

	/* copy */
	if (copy_from_user(
			&compat_ckey,
			(void __user *)src,
			sizeof(struct compat_vsp_ckey_unit_t))) {
		EPRINT("failed to copy of vsp_ckey_unit_t\n");
		return -EFAULT;
	}

	/* set */
	ckey->mode = compat_ckey.mode;
	ckey->color1 = (unsigned long)compat_ckey.color1;
	ckey->color2 = (unsigned long)compat_ckey.color2;

	return 0;
}

static int set_compat_vsp_src_alpha_par(
	struct vspm_entry_vsp_in_alpha *alpha, unsigned int *src)
{
	struct compat_vsp_alpha_unit_t compat_alpha;
	int ercd;

	/* copy vsp_alpha_unit_t parameter */
	if (copy_from_user(
			&compat_alpha,
			(void __user *)src,
			sizeof(struct compat_vsp_alpha_unit_t))) {
		EPRINT("failed to copy of vsp_alpha_unit_t\n");
		return -EFAULT;
	}

	alpha->alpha.addr_a = (void *)compat_alpha.addr_a;
	alpha->alpha.stride_a = compat_alpha.stride_a;
	alpha->alpha.swap = compat_alpha.swap;
	alpha->alpha.asel = compat_alpha.asel;
	alpha->alpha.aext = compat_alpha.aext;
	alpha->alpha.anum0 = compat_alpha.anum0;
	alpha->alpha.anum1 = compat_alpha.anum1;
	alpha->alpha.afix = compat_alpha.afix;

	/* copy vsp_irop_unit_t paramerter */
	if (compat_alpha.irop) {
		ercd = set_compat_vsp_irop_par(&alpha->irop, compat_alpha.irop);
		if (ercd)
			return ercd;
		alpha->alpha.irop = &alpha->irop;
	}

	/* copy vsp_ckey_unit_t paramerter */
	if (compat_alpha.ckey) {
		ercd = set_compat_vsp_ckey_par(&alpha->ckey, compat_alpha.ckey);
		if (ercd)
			return ercd;
		alpha->alpha.ckey = &alpha->ckey;
	}

	/* copy vsp_mult_unit_t paramerter */
	if (compat_alpha.mult) {
		if (copy_from_user(
				&alpha->mult,
				(void __user *)compat_alpha.mult,
				sizeof(struct vsp_mult_unit_t))) {
			EPRINT("failed to copy of vsp_mult_unit_t\n");
			return -EFAULT;
		}
		alpha->alpha.mult = &alpha->mult;
	}

	return 0;
}

static int set_compat_vsp_src_par(
	struct vspm_entry_vsp_in *in, unsigned int *src)
{
	struct compat_vsp_src_t compat_vsp_src;
	int ercd;

	/* copy vsp_src_t parameter */
	if (copy_from_user(
			&compat_vsp_src,
			(void __user *)src,
			sizeof(struct compat_vsp_src_t))) {
		EPRINT("failed to copy of vsp_src_t\n");
		return -EFAULT;
	}

	in->in.addr = (void *)compat_vsp_src.addr;
	in->in.addr_c0 = (void *)compat_vsp_src.addr_c0;
	in->in.addr_c1 = (void *)compat_vsp_src.addr_c1;
	in->in.stride = compat_vsp_src.stride;
	in->in.stride_c = compat_vsp_src.stride_c;
	in->in.width = compat_vsp_src.width;
	in->in.height = compat_vsp_src.height;
	in->in.width_ex = compat_vsp_src.width_ex;
	in->in.height_ex = compat_vsp_src.height_ex;
	in->in.x_offset = compat_vsp_src.x_offset;
	in->in.y_offset = compat_vsp_src.y_offset;
	in->in.format = compat_vsp_src.format;
	in->in.swap = compat_vsp_src.swap;
	in->in.x_position = compat_vsp_src.x_position;
	in->in.y_position = compat_vsp_src.y_position;
	in->in.pwd = compat_vsp_src.pwd;
	in->in.cipm = compat_vsp_src.cipm;
	in->in.cext = compat_vsp_src.cext;
	in->in.csc = compat_vsp_src.csc;
	in->in.iturbt = compat_vsp_src.iturbt;
	in->in.clrcng = compat_vsp_src.clrcng;
	in->in.vir = compat_vsp_src.vir;
	in->in.vircolor = (unsigned long)compat_vsp_src.vircolor;
	in->in.connect = (unsigned long)compat_vsp_src.connect;

	/* copy vsp_dl_t parameter */
	if (compat_vsp_src.clut) {
		ercd = set_compat_vsp_src_clut_par(
			&in->clut, compat_vsp_src.clut);
		if (ercd)
			return ercd;
		in->in.clut = &in->clut.clut;
	}

	/* copy vsp_alpha_unit_t parameter */
	if (compat_vsp_src.alpha) {
		ercd = set_compat_vsp_src_alpha_par(
			&in->alpha, compat_vsp_src.alpha);
		if (ercd)
			return ercd;
		in->in.alpha = &in->alpha.alpha;
	}

	return 0;
}

static int set_compat_fcp_par(struct fcp_info_t *fcp_info, unsigned int *src)
{
	struct compat_fcp_info_t compat_fcp_info;

	/* copy */
	if (copy_from_user(
			&compat_fcp_info,
			(void __user *)src,
			sizeof(struct compat_fcp_info_t))) {
		EPRINT("failed to copy of fcp_info_t\n");
		return -EFAULT;
	}

	/* set */
	fcp_info->fcnl = compat_fcp_info.fcnl;
	fcp_info->tlen = compat_fcp_info.tlen;
	fcp_info->pos_y = compat_fcp_info.pos_y;
	fcp_info->pos_c = compat_fcp_info.pos_c;
	fcp_info->stride_div16 = compat_fcp_info.stride_div16;
	fcp_info->ba_anc_prev_y = (void *)compat_fcp_info.ba_anc_prev_y;
	fcp_info->ba_anc_cur_y = (void *)compat_fcp_info.ba_anc_cur_y;
	fcp_info->ba_anc_next_y = (void *)compat_fcp_info.ba_anc_next_y;
	fcp_info->ba_anc_cur_c = (void *)compat_fcp_info.ba_anc_cur_c;
	fcp_info->ba_ref_prev_y = (void *)compat_fcp_info.ba_ref_prev_y;
	fcp_info->ba_ref_cur_y = (void *)compat_fcp_info.ba_ref_cur_y;
	fcp_info->ba_ref_next_y = (void *)compat_fcp_info.ba_ref_next_y;
	fcp_info->ba_ref_cur_c = (void *)compat_fcp_info.ba_ref_cur_c;

	return 0;
}

static int set_compat_vsp_dst_par(
	struct vspm_entry_vsp_out *out, unsigned int *src)
{
	struct compat_vsp_dst_t compat_vsp_dst;
	int ercd;

	/* copy vsp_dst_t parameter */
	if (copy_from_user(
			&compat_vsp_dst,
			(void __user *)src,
			sizeof(struct compat_vsp_dst_t))) {
		EPRINT("failed to copy of vsp_dst_t\n");
		return -EFAULT;
	}

	out->out.addr = (void *)compat_vsp_dst.addr;
	out->out.addr_c0 = (void *)compat_vsp_dst.addr_c0;
	out->out.addr_c1 = (void *)compat_vsp_dst.addr_c1;
	out->out.stride = compat_vsp_dst.stride;
	out->out.stride_c = compat_vsp_dst.stride_c;
	out->out.width = compat_vsp_dst.width;
	out->out.height = compat_vsp_dst.height;
	out->out.x_offset = compat_vsp_dst.x_offset;
	out->out.y_offset = compat_vsp_dst.y_offset;
	out->out.format = compat_vsp_dst.format;
	out->out.swap = compat_vsp_dst.swap;
	out->out.pxa = compat_vsp_dst.pxa;
	out->out.pad = compat_vsp_dst.pad;
	out->out.x_coffset = compat_vsp_dst.x_coffset;
	out->out.y_coffset = compat_vsp_dst.y_coffset;
	out->out.csc = compat_vsp_dst.csc;
	out->out.iturbt = compat_vsp_dst.iturbt;
	out->out.clrcng = compat_vsp_dst.clrcng;
	out->out.cbrm = compat_vsp_dst.cbrm;
	out->out.abrm = compat_vsp_dst.abrm;
	out->out.athres = compat_vsp_dst.athres;
	out->out.clmd = compat_vsp_dst.clmd;
	out->out.dith = compat_vsp_dst.dith;
	out->out.rotation = compat_vsp_dst.rotation;

	/* copy fcp_info_t parameter */
	if (compat_vsp_dst.fcp) {
		ercd = set_compat_fcp_par(&out->fcp, compat_vsp_dst.fcp);
		if (ercd)
			return ercd;
		out->out.fcp = &out->fcp;
	}

	return 0;
}

static int set_compat_vsp_sru_par(struct vsp_sru_t *sru, unsigned int *src)
{
	struct compat_vsp_sru_t compat_sru;

	/* copy */
	if (copy_from_user(
			&compat_sru,
			(void __user *)src,
			sizeof(struct compat_vsp_sru_t))) {
		EPRINT("failed to copy of vsp_sru_t\n");
		return -EFAULT;
	}

	/* set */
	sru->mode = compat_sru.mode;
	sru->param = compat_sru.param;
	sru->enscl = compat_sru.enscl;
	sru->fxa = compat_sru.fxa;
	sru->connect = (unsigned long)compat_sru.connect;

	return 0;
}

static int set_compat_vsp_uds_par(struct vsp_uds_t *uds, unsigned int *src)
{
	struct compat_vsp_uds_t compat_uds;

	/* copy */
	if (copy_from_user(
			&compat_uds,
			(void __user *)src,
			sizeof(struct compat_vsp_uds_t))) {
		EPRINT("failed to copy of vsp_uds_t\n");
		return -EFAULT;
	}

	/* set */
	uds->amd = compat_uds.amd;
	uds->clip = compat_uds.clip;
	uds->alpha = compat_uds.alpha;
	uds->complement = compat_uds.complement;
	uds->athres0 = compat_uds.athres0;
	uds->athres1 = compat_uds.athres1;
	uds->anum0 = compat_uds.anum0;
	uds->anum1 = compat_uds.anum1;
	uds->anum2 = compat_uds.anum2;
	uds->x_ratio = compat_uds.x_ratio;
	uds->y_ratio = compat_uds.y_ratio;
	uds->connect = (unsigned long)compat_uds.connect;

	return 0;
}


static int set_compat_vsp_lut_par(struct vsp_lut_t *lut, unsigned int *src)
{
	struct compat_vsp_lut_t compat_lut;

	/* copy */
	if (copy_from_user(
			&compat_lut,
			(void __user *)src,
			sizeof(struct compat_vsp_lut_t))) {
		EPRINT("failed to copy of vsp_lut_t\n");
		return -EFAULT;
	}

	/* set */
	lut->lut.hard_addr = (void *)compat_lut.lut.hard_addr;
	lut->lut.virt_addr = (void *)compat_lut.lut.virt_addr;
	lut->lut.tbl_num = compat_lut.lut.tbl_num;
	lut->fxa = compat_lut.fxa;
	lut->connect = (unsigned long)compat_lut.connect;

	return 0;
}

static int set_compat_vsp_clu_par(struct vsp_clu_t *clu, unsigned int *src)
{
	struct compat_vsp_clu_t compat_clu;

	/* copy */
	if (copy_from_user(
			&compat_clu,
			(void __user *)src,
			sizeof(struct compat_vsp_clu_t))) {
		EPRINT("failed to copy of vsp_clu_t\n");
		return -EFAULT;
	}

	/* set */
	clu->mode = compat_clu.mode;
	clu->clu.hard_addr = (void *)compat_clu.clu.hard_addr;
	clu->clu.virt_addr = (void *)compat_clu.clu.virt_addr;
	clu->clu.tbl_num = compat_clu.clu.tbl_num;
	clu->fxa = compat_clu.fxa;
	clu->connect = (unsigned long)compat_clu.connect;

	return 0;
}


static int set_compat_vsp_hst_par(struct vsp_hst_t *hst, unsigned int *src)
{
	struct compat_vsp_hst_t compat_hst;

	/* copy */
	if (copy_from_user(
			&compat_hst,
			(void __user *)src,
			sizeof(struct compat_vsp_hst_t))) {
		EPRINT("failed to copy of vsp_hst_t\n");
		return -EFAULT;
	}

	/* set */
	hst->fxa = compat_hst.fxa;
	hst->connect = (unsigned long)compat_hst.connect;

	return 0;
}

static int set_compat_vsp_hsi_par(struct vsp_hsi_t *hsi, unsigned int *src)
{
	struct compat_vsp_hsi_t compat_hsi;

	/* copy */
	if (copy_from_user(
			&compat_hsi,
			(void __user *)src,
			sizeof(struct compat_vsp_hsi_t))) {
		EPRINT("failed to copy of vsp_hsi_t\n");
		return -EFAULT;
	}

	/* set */
	hsi->fxa = compat_hsi.fxa;
	hsi->connect = (unsigned long)compat_hsi.connect;

	return 0;
}

static int set_compat_vsp_bru_vir_par(
	struct vsp_bld_vir_t *vir, unsigned int *src)
{
	struct compat_vsp_bld_vir_t compat_vir;

	/* copy */
	if (copy_from_user(
			&compat_vir,
			(void __user *)src,
			sizeof(struct compat_vsp_bld_vir_t))) {
		EPRINT("failed to copy of vsp_bld_vir_t\n");
		return -EFAULT;
	}

	/* set */
	vir->width = compat_vir.width;
	vir->height = compat_vir.height;
	vir->x_position = compat_vir.x_position;
	vir->y_position = compat_vir.y_position;
	vir->pwd = compat_vir.pwd;
	vir->color = (unsigned long)compat_vir.color;

	return 0;
}

static int set_compat_vsp_bru_par(
	struct vspm_entry_vsp_bru *bru, unsigned int *src)
{
	struct vsp_bld_ctrl_t * *(src_blend[5]);
	struct compat_vsp_bru_t compat_bru;
	int ercd;
	int i;

	/* copy vsp_bru_t parameter */
	if (copy_from_user(
			&compat_bru,
			(void __user *)src,
			sizeof(struct compat_vsp_bru_t))) {
		EPRINT("failed to copy of vsp_bru_t\n");
		return -EFAULT;
	}

	bru->bru.lay_order = (unsigned long)compat_bru.lay_order;
	bru->bru.adiv = compat_bru.adiv;
	bru->bru.connect = (unsigned long)compat_bru.connect;

	/* copy vsp_bld_dither_t parameter */
	for (i = 0; i < 5; i++) {
		if (compat_bru.dither_unit[i]) {
			if (copy_from_user(
					&bru->dither_unit[i],
					(void __user *)
						compat_bru.dither_unit[i],
					sizeof(struct vsp_bld_dither_t))) {
				EPRINT("failed to copy of vsp_bld_dither_t\n");
				return -EFAULT;
			}
			bru->bru.dither_unit[i] = &bru->dither_unit[i];
		}
	}

	/* copy vsp_bld_vir_t parameter */
	if (compat_bru.blend_virtual) {
		ercd = set_compat_vsp_bru_vir_par(
			&bru->blend_virtual, compat_bru.blend_virtual);
		if (ercd)
			return ercd;
		bru->bru.blend_virtual = &bru->blend_virtual;
	}

	/* copy vsp_bld_ctrl_t parameter */
	src_blend[0] = &bru->bru.blend_unit_a;
	src_blend[1] = &bru->bru.blend_unit_b;
	src_blend[2] = &bru->bru.blend_unit_c;
	src_blend[3] = &bru->bru.blend_unit_d;
	src_blend[4] = &bru->bru.blend_unit_e;

	for (i = 0; i < 5; i++) {
		if (compat_bru.blend_unit[i]) {
			if (copy_from_user(
					&bru->blend_unit[i],
					(void __user *)compat_bru.blend_unit[i],
					sizeof(struct vsp_bld_ctrl_t))) {
				EPRINT("failed to copy of vsp_bld_ctrl_t\n");
				return -EFAULT;
			}
			*src_blend[i] = &bru->blend_unit[i];
		}
	}

	/* copy vsp_bld_rop_t parameter */
	if (compat_bru.rop_unit) {
		if (copy_from_user(
				&bru->rop_unit,
				(void __user *)compat_bru.rop_unit,
				sizeof(struct vsp_bld_rop_t))) {
			EPRINT("failed to copy of vsp_bld_rop_t\n");
			return -EFAULT;
		}
		bru->bru.rop_unit = &bru->rop_unit;
	}

	return 0;
}

static int set_compat_vsp_hgo_par(
	struct vspm_entry_vsp_hgo *hgo, unsigned int *src)
{
	struct compat_vsp_hgo_t compat_hgo;
	dma_addr_t hard_addr;
	void *virt_addr;
	unsigned long tmp_addr;

	/* copy */
	if (copy_from_user(
			&compat_hgo,
			(void __user *)src,
			sizeof(struct compat_vsp_hgo_t))) {
		EPRINT("failed to copy of vsp_hgo_t\n");
		return -EFAULT;
	}

	/* allocate memory */
	virt_addr = dma_alloc_coherent(
		dev, 1088, &hard_addr, GFP_KERNEL | GFP_DMA);
	if (virt_addr == NULL) {
		EPRINT("failed to allocate memory\n");
		return -EFAULT;
	}

	/* set */
	tmp_addr = (unsigned long)(hard_addr + 255) >> 8;
	hgo->hgo.hard_addr = (void *)(tmp_addr << 8);
	tmp_addr = (unsigned long)(virt_addr + 255) >> 8;
	hgo->hgo.virt_addr = (void *)(tmp_addr << 8);
	hgo->hgo.width = compat_hgo.width;
	hgo->hgo.height = compat_hgo.height;
	hgo->hgo.x_offset = compat_hgo.x_offset;
	hgo->hgo.y_offset = compat_hgo.y_offset;
	hgo->hgo.binary_mode = compat_hgo.binary_mode;
	hgo->hgo.maxrgb_mode = compat_hgo.maxrgb_mode;
	hgo->hgo.step_mode = compat_hgo.step_mode;
	hgo->hgo.x_skip = compat_hgo.x_skip;
	hgo->hgo.y_skip = compat_hgo.y_skip;
	hgo->hgo.sampling = (unsigned long)compat_hgo.sampling;

	return 0;
}

static int set_compat_vsp_hgt_par(
	struct vspm_entry_vsp_hgt *hgt, unsigned int *src)
{
	struct compat_vsp_hgt_t compat_hgt;
	dma_addr_t hard_addr;
	void *virt_addr;
	unsigned long tmp_addr;

	int i;

	/* copy */
	if (copy_from_user(
			&compat_hgt,
			(void __user *)src,
			sizeof(struct compat_vsp_hgt_t))) {
		EPRINT("failed to copy of vsp_hgt_t\n");
		return -EFAULT;
	}

	/* allocate memory */
	virt_addr = dma_alloc_coherent(
		dev, 800, &hard_addr, GFP_KERNEL | GFP_DMA);
	if (virt_addr == NULL) {
		EPRINT("failed to allocate memory\n");
		return -EFAULT;
	}

	/* set */
	tmp_addr = (unsigned long)(hard_addr + 255) >> 8;
	hgt->hgt.hard_addr = (void *)(tmp_addr << 8);
	tmp_addr = (unsigned long)(virt_addr + 255) >> 8;
	hgt->hgt.virt_addr = (void *)(tmp_addr << 8);
	hgt->hgt.width = compat_hgt.width;
	hgt->hgt.height = compat_hgt.height;
	hgt->hgt.x_offset = compat_hgt.x_offset;
	hgt->hgt.y_offset = compat_hgt.y_offset;
	hgt->hgt.x_skip = compat_hgt.x_skip;
	hgt->hgt.y_skip = compat_hgt.y_skip;
	for (i = 0; i < 6; i++) {
		hgt->hgt.area[i].lower = compat_hgt.area[i].lower;
		hgt->hgt.area[i].upper = compat_hgt.area[i].upper;
	}
	hgt->hgt.sampling = (unsigned long)compat_hgt.sampling;

	return 0;
}

static int set_compat_vsp_shp_par(struct vsp_shp_t *shp, unsigned int *src)
{
	struct compat_vsp_shp_t compat_shp;

	/* copy */
	if (copy_from_user(
			&compat_shp,
			(void __user *)src,
			sizeof(struct compat_vsp_shp_t))) {
		EPRINT("failed to copy of vsp_shp_t\n");
		return -EFAULT;
	}

	/* set */
	shp->mode = compat_shp.mode;
	shp->gain0 = compat_shp.gain0;
	shp->limit0 = compat_shp.limit0;
	shp->gain10 = compat_shp.gain10;
	shp->limit10 = compat_shp.limit10;
	shp->gain11 = compat_shp.gain11;
	shp->limit11 = compat_shp.limit11;
	shp->gain20 = compat_shp.gain20;
	shp->limit20 = compat_shp.limit20;
	shp->gain21 = compat_shp.gain21;
	shp->limit21 = compat_shp.limit21;
	shp->fxa = compat_shp.fxa;
	shp->connect = (unsigned long)compat_shp.connect;

	return 0;
}

static int set_compat_vsp_drc_par(struct vsp_drc_t *drc, unsigned int *src)
{
	struct compat_vsp_drc_t compat_drc;

	/* copy */
	if (copy_from_user(
			&compat_drc,
			(void __user *)src,
			sizeof(struct compat_vsp_drc_t))) {
		EPRINT("failed to copy of vsp_drc_t\n");
		return -EFAULT;
	}

	/* set */
	drc->drc.hard_addr = (void *)compat_drc.drc.hard_addr;
	drc->drc.virt_addr = (void *)compat_drc.drc.virt_addr;
	drc->drc.tbl_num = compat_drc.drc.tbl_num;
	drc->fxa = compat_drc.fxa;
	drc->connect = (unsigned long)compat_drc.connect;

	return 0;
}


static int set_compat_vsp_ctrl_par(
	struct vspm_entry_vsp_ctrl *ctrl, unsigned int *src)
{
	struct compat_vsp_ctrl_t compat_vsp_ctrl;
	int ercd;

	/* copy vsp_ctrl_t parameter */
	if (copy_from_user(
			&compat_vsp_ctrl,
			(void __user *)src,
			sizeof(struct compat_vsp_ctrl_t))) {
		EPRINT("failed to copy of vsp_ctrl_t\n");
		return -EFAULT;
	}

	/* copy vsp_sru_t parameter */
	if (compat_vsp_ctrl.sru) {
		ercd = set_compat_vsp_sru_par(&ctrl->sru, compat_vsp_ctrl.sru);
		if (ercd)
			return ercd;
		ctrl->ctrl.sru = &ctrl->sru;
	}

	/* copy vsp_uds_t parameter */
	if (compat_vsp_ctrl.uds) {
		ercd = set_compat_vsp_uds_par(&ctrl->uds, compat_vsp_ctrl.uds);
		if (ercd)
			return ercd;
		ctrl->ctrl.uds = &ctrl->uds;
	}

	/* copy vsp_lut_t parameter */
	if (compat_vsp_ctrl.lut) {
		ercd = set_compat_vsp_lut_par(&ctrl->lut, compat_vsp_ctrl.lut);
		if (ercd)
			return ercd;
		ctrl->ctrl.lut = &ctrl->lut;
	}

	/* copy vsp_clu_t parameter */
	if (compat_vsp_ctrl.clu) {
		ercd = set_compat_vsp_clu_par(&ctrl->clu, compat_vsp_ctrl.clu);
		if (ercd)
			return ercd;
		ctrl->ctrl.clu = &ctrl->clu;
	}

	/* copy vsp_hst_t parameter */
	if (compat_vsp_ctrl.hst) {
		ercd = set_compat_vsp_hst_par(&ctrl->hst, compat_vsp_ctrl.hst);
		if (ercd)
			return ercd;
		ctrl->ctrl.hst = &ctrl->hst;
	}

	/* copy vsp_hsi_t parameter */
	if (compat_vsp_ctrl.hsi) {
		ercd = set_compat_vsp_hsi_par(&ctrl->hsi, compat_vsp_ctrl.hsi);
		if (ercd)
			return ercd;
		ctrl->ctrl.hsi = &ctrl->hsi;
	}

	/* copy vsp_bru_t parameter */
	if (compat_vsp_ctrl.bru) {
		ercd = set_compat_vsp_bru_par(&ctrl->bru, compat_vsp_ctrl.bru);
		if (ercd)
			return ercd;
		ctrl->ctrl.bru = &ctrl->bru.bru;
	}

	/* copy vsp_hgo_t parameter */
	if (compat_vsp_ctrl.hgo) {
		ercd = set_compat_vsp_hgo_par(&ctrl->hgo, compat_vsp_ctrl.hgo);
		if (ercd)
			return ercd;
		ctrl->ctrl.hgo = &ctrl->hgo.hgo;
	}

	/* copy vsp_hgt_t parameter */
	if (compat_vsp_ctrl.hgt) {
		ercd = set_compat_vsp_hgt_par(&ctrl->hgt, compat_vsp_ctrl.hgt);
		if (ercd)
			return ercd;
		ctrl->ctrl.hgt = &ctrl->hgt.hgt;
	}

	/* copy vsp_shp_t parameter */
	if (compat_vsp_ctrl.shp) {
		ercd = set_compat_vsp_shp_par(&ctrl->shp, compat_vsp_ctrl.shp);
		if (ercd)
			return ercd;
		ctrl->ctrl.shp = &ctrl->shp;
	}

	/* copy vsp_drc_t parameter */
	if (compat_vsp_ctrl.drc) {
		ercd = set_compat_vsp_drc_par(&ctrl->drc, compat_vsp_ctrl.drc);
		if (ercd)
			return ercd;
		ctrl->ctrl.drc = &ctrl->drc;
	}

	return 0;
}


int set_compat_vsp_par(
	struct vspm_if_entry_data_t *entry, unsigned int *src)
{
	struct vspm_entry_vsp *vsp = &entry->ip_par.vsp;
	struct compat_vsp_start_t compat_vsp_par;
	dma_addr_t hard_addr;
	void *virt_addr;
	int ercd;

	int i;

	/* copy vsp_start_t parameter */
	if (copy_from_user(
			&compat_vsp_par,
			(void __user *)src,
			sizeof(struct compat_vsp_start_t))) {
		EPRINT("failed to copy of vsp_start_t\n");
		return -EFAULT;
	}

	vsp->par.rpf_num = compat_vsp_par.rpf_num;
	vsp->par.rpf_order = 0;	/* not used */
	vsp->par.use_module = (unsigned long)compat_vsp_par.use_module;

	/* allocate memory for display list */
	if ((compat_vsp_par.dl_par.tbl_num > 0) &&
		(compat_vsp_par.dl_par.tbl_num < 16384)) {
		/* allocate memory */
		virt_addr = dma_alloc_coherent(
			dev,
			vsp->par.dl_par.tbl_num * 8,
			&hard_addr,
			GFP_KERNEL | GFP_DMA);
		if (virt_addr == NULL) {
			EPRINT("failed to allocate memory\n");
			return -EFAULT;
		}

		/* set parameter */
		vsp->par.dl_par.hard_addr = (void *)hard_addr;
		vsp->par.dl_par.virt_addr = virt_addr;
		vsp->par.dl_par.tbl_num = compat_vsp_par.dl_par.tbl_num;

		vsp->dl.hard_addr = hard_addr;
		vsp->dl.virt_addr = virt_addr;
	}

	/* copy vsp_src_t parameter */
	for (i = 0; i < 5; i++) {
		if (compat_vsp_par.src_par[i]) {
			ercd = set_compat_vsp_src_par(
				&vsp->in[i], compat_vsp_par.src_par[i]);
			if (ercd)
				goto err_exit;
			vsp->par.src_par[i] = &vsp->in[i].in;
		}
	}

	/* copy vsp_dst_t parameter */
	if (compat_vsp_par.dst_par) {
		ercd = set_compat_vsp_dst_par(
			&vsp->out, compat_vsp_par.dst_par);
		if (ercd)
			goto err_exit;
		vsp->par.dst_par = &vsp->out.out;
	}

	/* copy vsp_ctrl_t parameter */
	if (compat_vsp_par.ctrl_par) {
		ercd = set_compat_vsp_ctrl_par(
			&vsp->ctrl, compat_vsp_par.ctrl_par);
		if (ercd)
			goto err_exit;
		vsp->par.ctrl_par = &vsp->ctrl.ctrl;
	}

	return 0;

err_exit:
	free_vsp_par(vsp);
	return ercd;
}

static int set_compat_fdp_pic_par(struct fdp_pic_t *in_pic, unsigned int *src)
{
	struct compat_fdp_pic_t compat_fdp_pic;

	/* copy */
	if (copy_from_user(
			&compat_fdp_pic,
			(void __user *)src,
			sizeof(struct compat_fdp_pic_t))) {
		EPRINT("failed to copy of fdp_pic_t\n");
		return -EFAULT;
	}

	/* set */
	in_pic->picid = (unsigned long)compat_fdp_pic.picid;
	in_pic->chroma_format = compat_fdp_pic.chroma_format;
	in_pic->width = compat_fdp_pic.width;
	in_pic->height = compat_fdp_pic.height;
	in_pic->progressive_sequence = compat_fdp_pic.progressive_sequence;
	in_pic->progressive_frame = compat_fdp_pic.progressive_frame;
	in_pic->picture_structure = compat_fdp_pic.picture_structure;
	in_pic->repeat_first_field = compat_fdp_pic.repeat_first_field;
	in_pic->top_field_first = compat_fdp_pic.top_field_first;

	return 0;
}

static int set_compat_fdp_img_par(
	struct fdp_imgbuf_t *imgbuf, unsigned int *src)
{
	struct compat_fdp_imgbuf_t compat_fdp_imgbuf;

	/* copy */
	if (copy_from_user(
			&compat_fdp_imgbuf,
			(void __user *)src,
			sizeof(struct compat_fdp_imgbuf_t))) {
		EPRINT("failed to copy of fdp_imgbuf_t\n");
		return -EFAULT;
	}

	/* set */
	imgbuf->addr = (void *)compat_fdp_imgbuf.addr;
	imgbuf->addr_c0 = (void *)compat_fdp_imgbuf.addr_c0;
	imgbuf->addr_c1 = (void *)compat_fdp_imgbuf.addr_c1;
	imgbuf->stride = compat_fdp_imgbuf.stride;
	imgbuf->stride_c = compat_fdp_imgbuf.stride_c;

	return 0;
}

static int set_compat_fdp_ref_par(
	struct vspm_entry_fdp_ref *ref, unsigned int *src)
{
	struct compat_fdp_refbuf_t compat_fdp_refbuf;
	int ercd;

	/* copy fdp_refbuf_t parameter */
	if (copy_from_user(
			&compat_fdp_refbuf,
			(void __user *)src,
			sizeof(struct compat_fdp_refbuf_t))) {
		EPRINT("failed to copy of fdp_refbuf_t\n");
		return -EFAULT;
	}

	if (compat_fdp_refbuf.next_buf) {
		ercd = set_compat_fdp_img_par(
			&ref->ref[0], compat_fdp_refbuf.next_buf);
		if (ercd)
			return ercd;
		ref->ref_buf.next_buf = &ref->ref[0];
	}

	if (compat_fdp_refbuf.cur_buf) {
		ercd = set_compat_fdp_img_par(
			&ref->ref[1], compat_fdp_refbuf.cur_buf);
		if (ercd)
			return ercd;
		ref->ref_buf.cur_buf = &ref->ref[1];
	}

	if (compat_fdp_refbuf.prev_buf) {
		ercd = set_compat_fdp_img_par(
			&ref->ref[2], compat_fdp_refbuf.prev_buf);
		if (ercd)
			return ercd;
		ref->ref_buf.prev_buf = &ref->ref[2];
	}

	return 0;
}

static int set_compat_fdp_fproc_par(
	struct vspm_entry_fdp_fproc *fproc, unsigned int *src)
{
	struct compat_fdp_fproc_t compat_fdp_fproc;
	int ercd;

	/* copy fdp_fproc_t parameter */
	if (copy_from_user(
			&compat_fdp_fproc,
			(void __user *)src,
			sizeof(struct compat_fdp_fproc_t))) {
		EPRINT("failed to copy of fdp_fproc_t\n");
		return -EFAULT;
	}

	fproc->fproc.last_seq_indicator = compat_fdp_fproc.last_seq_indicator;
	fproc->fproc.current_field = compat_fdp_fproc.current_field;
	fproc->fproc.interpolated_line = compat_fdp_fproc.interpolated_line;
	fproc->fproc.out_format = compat_fdp_fproc.out_format;

	/* copy fdp_seq_t parameter */
	if (compat_fdp_fproc.seq_par) {
		if (copy_from_user(
				&fproc->seq,
				(void __user *)compat_fdp_fproc.seq_par,
				sizeof(struct fdp_seq_t))) {
			EPRINT("failed to copy to fdp_seq_t\n");
			return -EFAULT;
		}
		fproc->fproc.seq_par = &fproc->seq;
	}

	/* copy fdp_pic_t parameter */
	if (compat_fdp_fproc.in_pic) {
		ercd = set_compat_fdp_pic_par(
			&fproc->in_pic, compat_fdp_fproc.in_pic);
		if (ercd)
			return ercd;
		fproc->fproc.in_pic = &fproc->in_pic;
	}

	/* copy fdp_imgbuf_t parameter */
	if (compat_fdp_fproc.out_buf) {
		ercd = set_compat_fdp_img_par(
			&fproc->out_buf, compat_fdp_fproc.out_buf);
		if (ercd)
			return ercd;
		fproc->fproc.out_buf = &fproc->out_buf;
	}

	/* copy fdp_refbuf_t parameter */
	if (compat_fdp_fproc.ref_buf) {
		ercd = set_compat_fdp_ref_par(
			&fproc->ref, compat_fdp_fproc.ref_buf);
		if (ercd)
			return ercd;
		fproc->fproc.ref_buf = &fproc->ref.ref_buf;
	}

	/* copy fcp_info_t parameter */
	if (compat_fdp_fproc.fcp_par) {
		ercd = set_compat_fcp_par(
			&fproc->fcp, compat_fdp_fproc.fcp_par);
		if (ercd)
			return ercd;
		fproc->fproc.fcp_par = &fproc->fcp;
	}

	return 0;
}


int set_compat_fdp_par(
	struct vspm_if_entry_data_t *entry, unsigned int *src)
{
	struct vspm_entry_fdp *fdp = &entry->ip_par.fdp;
	struct compat_fdp_start_t compat_fdp_par;
	int ercd;

	/* copy fdp_start_t parameter */
	if (copy_from_user(
			&compat_fdp_par,
			(void __user *)src,
			sizeof(struct compat_fdp_start_t))) {
		EPRINT("failed to copy of fdp_start_t\n");
		return -EFAULT;
	}

	fdp->par.fdpgo = compat_fdp_par.fdpgo;

	/* copy fdp_fproc_t parameter */
	if (compat_fdp_par.fproc_par) {
		ercd = set_compat_fdp_fproc_par(
			&fdp->fproc, compat_fdp_par.fproc_par);
		if (ercd)
			return ercd;
		fdp->par.fproc_par = &fdp->fproc.fproc;
	}

	return 0;
}
