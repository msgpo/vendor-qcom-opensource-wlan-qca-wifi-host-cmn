/*
* Copyright (c) 2017 The Linux Foundation. All rights reserved.
*
* Permission to use, copy, modify, and/or distribute this software for
* any purpose with or without fee is hereby granted, provided that the
* above copyright notice and this permission notice appear in all
* copies.
*
* THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
* WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
* AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
* DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
* PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
* TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
* PERFORMANCE OF THIS SOFTWARE.
*/
/**
 * DOC: define utility API related to the pmo component
 * called by other components
 */

#include "wlan_pmo_obj_mgmt_api.h"
#include "wlan_pmo_tgt_api.h"
#include "wlan_pmo_static_config.h"

QDF_STATUS pmo_init(void)
{
	QDF_STATUS status;
	struct wlan_pmo_ctx *pmo_ctx;

	PMO_ENTER();
	if (pmo_allocate_ctx() != QDF_STATUS_SUCCESS) {
		pmo_err("unable to allocate psoc ctx");
		status = QDF_STATUS_E_FAULT;
		goto out;
	}

	pmo_ctx = pmo_get_context();
	if (!pmo_ctx) {
		pmo_err("unable to get pmo ctx");
		status = QDF_STATUS_E_INVAL;
		goto out;
	}

	status = wlan_objmgr_register_psoc_create_handler(
			WLAN_UMAC_COMP_PMO,
			pmo_psoc_object_created_notification,
			(void *)pmo_ctx);
	if (status != QDF_STATUS_SUCCESS) {
		pmo_err("unable to register psoc create handle");
		goto out;
	}

	status = wlan_objmgr_register_psoc_destroy_handler(
			WLAN_UMAC_COMP_PMO,
			 pmo_psoc_object_destroyed_notification,
			(void *)pmo_ctx);
	if (status != QDF_STATUS_SUCCESS) {
		pmo_err("unable to register psoc create handle");
		goto out;
	}

	status = wlan_objmgr_register_vdev_create_handler(
			WLAN_UMAC_COMP_PMO,
			pmo_vdev_object_created_notification,
			(void *)pmo_ctx);
	if (status != QDF_STATUS_SUCCESS) {
		pmo_err("unable to register vdev create handle");
		goto out;
	}

	status = wlan_objmgr_register_vdev_destroy_handler(
			WLAN_UMAC_COMP_PMO,
			pmo_vdev_object_destroyed_notification,
			(void *)pmo_ctx);
	if (status != QDF_STATUS_SUCCESS)
		pmo_err("unable to register vdev create handle");
out:
	PMO_EXIT();

	return status;
}

QDF_STATUS pmo_deinit(void)
{
	QDF_STATUS status;
	struct wlan_pmo_ctx *pmo_ctx;

	PMO_ENTER();
	pmo_ctx = pmo_get_context();
	if (!pmo_ctx) {
		pmo_err("unable to get pmo ctx");
		status =  QDF_STATUS_E_FAILURE;
		goto out;
	}

	status = wlan_objmgr_unregister_psoc_create_handler(
			WLAN_UMAC_COMP_PMO,
			pmo_psoc_object_created_notification,
			(void *)pmo_ctx);
	if (status != QDF_STATUS_SUCCESS) {
		pmo_err("unable to unregister psoc create handle");
		goto out;
	}

	status = wlan_objmgr_unregister_psoc_destroy_handler(
			WLAN_UMAC_COMP_PMO,
			 pmo_psoc_object_destroyed_notification,
			(void *)pmo_ctx);
	if (status != QDF_STATUS_SUCCESS) {
		pmo_err("unable to unregister psoc create handle");
		goto out;
	}

	status = wlan_objmgr_unregister_vdev_create_handler(
			WLAN_UMAC_COMP_PMO,
			pmo_vdev_object_created_notification,
			(void *)pmo_ctx);
	if (status != QDF_STATUS_SUCCESS) {
		pmo_err("unable to unregister vdev create handle");
		goto out;
	}

	status = wlan_objmgr_unregister_vdev_destroy_handler(
			WLAN_UMAC_COMP_PMO,
			pmo_vdev_object_destroyed_notification,
			(void *)pmo_ctx);
	if (status != QDF_STATUS_SUCCESS) {
		pmo_err("unable to unregister vdev create handle");
		goto out;
	}

out:
	pmo_free_ctx();
	PMO_EXIT();

	return status;
}

QDF_STATUS pmo_psoc_object_created_notification(
		struct wlan_objmgr_psoc *psoc, void *arg)
{
	struct pmo_psoc_priv_obj *psoc_ctx = NULL;
	QDF_STATUS status;
	struct wlan_pmo_ctx *pmo_ctx;

	PMO_ENTER();
	pmo_ctx = pmo_get_context();
	if (!pmo_ctx) {
		QDF_ASSERT(0);
		pmo_err("unable to get pmo ctx");
		status = QDF_STATUS_E_FAILURE;
		goto out;
	}

	psoc_ctx = qdf_mem_malloc(sizeof(*pmo_ctx));
	if (psoc_ctx == NULL) {
		pmo_err("Failed to allocate pmo_psoc");
		status = QDF_STATUS_E_NOMEM;
		goto out;
	}

	status = wlan_objmgr_psoc_component_obj_attach(psoc,
			WLAN_UMAC_COMP_PMO,
			psoc_ctx,
			QDF_STATUS_SUCCESS);
	if (status != QDF_STATUS_SUCCESS) {
		pmo_err("Failed to attach psoc_ctx with psoc");
		qdf_mem_free(psoc_ctx);
		status = QDF_STATUS_E_FAILURE;
		goto out;
	}
	qdf_spinlock_create(&psoc_ctx->lock);
out:
	PMO_EXIT();

	return status;
}

QDF_STATUS pmo_psoc_object_destroyed_notification(
		struct wlan_objmgr_psoc *psoc, void *arg)
{
	struct pmo_psoc_priv_obj *psoc_ctx = NULL;
	QDF_STATUS status;

	PMO_ENTER();
	psoc_ctx = pmo_get_psoc_priv_ctx(psoc);
	if (!psoc_ctx) {
		pmo_err("psoc_ctx is NULL");
		status = QDF_STATUS_E_FAILURE;
		goto out;
	}

	status = wlan_objmgr_psoc_component_obj_detach(psoc,
			WLAN_UMAC_COMP_PMO,
			psoc_ctx);
	if (status != QDF_STATUS_SUCCESS) {
		pmo_err("Failed to detach psoc_ctx from psoc");
		status = QDF_STATUS_E_FAILURE;
		goto out;
	}

	qdf_spinlock_destroy(&psoc_ctx->lock);
	qdf_mem_free(psoc_ctx);
out:
	PMO_EXIT();

	return status;
}

QDF_STATUS pmo_vdev_object_created_notification(
		struct wlan_objmgr_vdev *vdev, void *arg)
{
	struct pmo_psoc_priv_obj *psoc_ctx = NULL;
	struct wlan_objmgr_psoc *psoc;
	struct pmo_vdev_priv_obj *vdev_ctx;
	QDF_STATUS status;

	PMO_ENTER();
	psoc = wlan_vdev_get_psoc(vdev);
	if (psoc == NULL) {
		pmo_err("psoc is NULL");
		status = QDF_STATUS_E_NULL_VALUE;
		goto out;
	}

	psoc_ctx = pmo_get_psoc_priv_ctx(psoc);
	if (!psoc_ctx) {
		pmo_err("psoc_ctx is NULL");
		status = QDF_STATUS_E_NULL_VALUE;
		goto out;
	}

	vdev_ctx = qdf_mem_malloc(sizeof(*vdev_ctx));
	if (vdev_ctx == NULL) {
		pmo_err("Failed to allocate vdev_ctx");
		status = QDF_STATUS_E_NOMEM;
		goto out;
	}

	status = wlan_objmgr_vdev_component_obj_attach(vdev,
			 WLAN_UMAC_COMP_PMO,
			(void *)vdev_ctx, QDF_STATUS_SUCCESS);
	if (status != QDF_STATUS_SUCCESS) {
		pmo_err("Failed to attach vdev_ctx with vdev");
		qdf_mem_free(vdev_ctx);
		goto out;
	}

	qdf_spinlock_create(&vdev_ctx->pmo_vdev_lock);
	vdev_ctx->ptrn_match_enable =
		psoc_ctx->psoc_cfg.ptrn_match_enable_all_vdev;
	vdev_ctx->pmo_psoc_ctx = psoc_ctx;
	qdf_atomic_init(&vdev_ctx->gtk_err_enable);

	/* Register static configuration with firmware */
	pmo_register_wow_wakeup_events(vdev);
	pmo_register_action_frame_patterns(vdev);
	/* Register default wow patterns with firmware */
	pmo_register_wow_default_patterns(vdev);
out:
	PMO_EXIT();

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS pmo_vdev_object_destroyed_notification(
		struct wlan_objmgr_vdev *vdev, void *arg)
{
	struct pmo_vdev_priv_obj *vdev_ctx = NULL;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	PMO_ENTER();
	vdev_ctx = pmo_get_vdev_priv_ctx(vdev);
	if (!vdev_ctx) {
		pmo_err("vdev_ctx is NULL");
		status = QDF_STATUS_E_INVAL;
		goto out;
	}

	status = wlan_objmgr_vdev_component_obj_detach(vdev,
			 WLAN_UMAC_COMP_PMO,
			(void *)vdev_ctx);
	if (status != QDF_STATUS_SUCCESS)
		pmo_err("Failed to detach vdev_ctx with vdev");

	qdf_spinlock_destroy(&vdev_ctx->pmo_vdev_lock);
	qdf_mem_free(vdev_ctx);
out:
	PMO_EXIT();

	return status;
}

QDF_STATUS pmo_register_suspend_handler(
		enum wlan_umac_comp_id id,
		pmo_psoc_suspend_handler handler,
		void *arg)
{
	struct wlan_pmo_ctx *pmo_ctx;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	PMO_ENTER();
	pmo_ctx = pmo_get_context();
	if (!pmo_ctx) {
		QDF_ASSERT(0);
		pmo_err("unable to get pmo ctx");
		status = QDF_STATUS_E_FAILURE;
		goto out;
	}

	if (id > WLAN_UMAC_MAX_COMPONENTS || id < 0) {
		pmo_err("component id: %d is %s then valid components id",
			id, id < 0 ? "Less" : "More");
		status = QDF_STATUS_E_FAILURE;
		goto out;
	}

	qdf_spin_lock(&pmo_ctx->lock);
	pmo_ctx->pmo_suspend_handler[id] = handler;
	pmo_ctx->pmo_suspend_handler_arg[id] = handler;
	qdf_spin_unlock(&pmo_ctx->lock);
out:
	PMO_EXIT();

	return status;
}

QDF_STATUS pmo_register_resume_handler(
		enum wlan_umac_comp_id id,
		pmo_psoc_resume_handler handler,
		void *arg)
{
	struct wlan_pmo_ctx *pmo_ctx;
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	PMO_ENTER();
	pmo_ctx = pmo_get_context();
	if (!pmo_ctx) {
		pmo_err("unable to get pmo ctx");
		status = QDF_STATUS_E_FAILURE;
		goto out;
	}

	if (id > WLAN_UMAC_MAX_COMPONENTS || id < 0) {
		pmo_err("component id: %d is %s then valid components id",
			id, id < 0 ? "Less" : "More");
		status = QDF_STATUS_E_FAILURE;
		goto out;
	}

	qdf_spin_lock(&pmo_ctx->lock);
	pmo_ctx->pmo_resume_handler[id] = handler;
	pmo_ctx->pmo_resume_handler_arg[id] = handler;
	qdf_spin_unlock(&pmo_ctx->lock);
out:
	PMO_EXIT();

	return status;
}

QDF_STATUS pmo_suspend_psoc(struct wlan_objmgr_psoc *psoc,
		bool is_runtime_suspend)
{
	return QDF_STATUS_SUCCESS;
}

QDF_STATUS pmo_resume_psoc(struct wlan_objmgr_psoc *psoc,
		bool is_runtime_resume)
{
	return QDF_STATUS_SUCCESS;
}

