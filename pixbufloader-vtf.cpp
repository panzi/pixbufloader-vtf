#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <gdk-pixbuf/gdk-pixbuf-io.h>
#include <glib/gstdio.h>

#include <VTFLib.h>
#include <VTFFile.h>

using namespace VTFLib;

typedef struct {
	GdkPixbufModuleSizeFunc     size_func;
	GdkPixbufModulePreparedFunc prepared_func; 
	GdkPixbufModuleUpdatedFunc  updated_func;
	gpointer                    user_data;

	guchar *buffer;
	guint   buffer_size;
	guint   buffer_used;
} VtfContext;

static GdkPixbufModulePattern vtf_signature[] = {
	{ "VTF\0", "   z", 100 },
	{ NULL, NULL, 0 }
};

static gchar* vtf_mime_types[] = {
	"image/x-vtf",
	NULL
};

static gchar* vtf_extensions[] = {
	"vtf",
	NULL
};

static GdkPixbuf* vtf_image_load_memory(
	const guchar *buffer,
	guint         size,
	GError      **error)
{
	try {
		CVTFFile vtf;
		if (vtf.Load(buffer, size)) {
			const vlUInt width  = vtf.GetWidth();
			const vlUInt height = vtf.GetHeight();
			GdkPixbuf* pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, width, height);

	        if (pixbuf != NULL) {
				const vlByte* frame = vtf.GetData(vtf.GetFrameCount() - 1, 0, 0, 0);
				guchar* pixels = gdk_pixbuf_get_pixels(pixbuf);

				if (CVTFFile::ConvertToRGBA8888(frame, pixels, width, height, vtf.GetFormat())) {
					return pixbuf;
				}
				else {
					g_object_unref(pixbuf);
					g_set_error(
						error,
						GDK_PIXBUF_ERROR,
						GDK_PIXBUF_ERROR_FAILED,
						"Image data conversion failed");
					return NULL;
				}
			}
			else {
				g_set_error(
					error,
					GDK_PIXBUF_ERROR,
					GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
					"Could not allocate pixbuf object");
				return NULL;
			}
		}
		else {
			g_set_error(
				error,
				GDK_PIXBUF_ERROR,
				GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
				LastError.Get());
			return NULL;
		}
	}
	catch (...) {
		g_set_error(
			error,
			GDK_PIXBUF_ERROR,
			GDK_PIXBUF_ERROR_FAILED,
			"Unhandled C++ exception");
		return NULL;
	}
}

static GdkPixbuf* vtf_image_load(
	FILE    *fp,
	GError **error)
{
	fseeko(fp, 0, SEEK_END);
	off_t size = ftello(fp);

	if (size >= 0) {
		fseeko(fp, 0, SEEK_SET);
		guchar *buffer = (guchar*)g_malloc(size);

		if (buffer != NULL) {
			if (fread(buffer, size, 1, fp) == 1) {
				GdkPixbuf* pixbuf = vtf_image_load_memory(buffer, size, error);
				g_free(buffer);
				return pixbuf;
			}
			else {
				g_free(buffer);
				g_set_error(
					error,
					GDK_PIXBUF_ERROR,
					GDK_PIXBUF_ERROR_FAILED,
					strerror(errno));
				return NULL;
			}
		}
		else {
			g_set_error(
				error,
				GDK_PIXBUF_ERROR,
				GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
				"Could not allocate pixbuf object");
			return NULL;
		}
	}
	else {
		g_set_error(
			error,
			GDK_PIXBUF_ERROR,
			GDK_PIXBUF_ERROR_FAILED,
			strerror(errno));
		return NULL;
	}
}

static gpointer vtf_image_begin_load(
	GdkPixbufModuleSizeFunc     size_func,
	GdkPixbufModulePreparedFunc prepared_func,
	GdkPixbufModuleUpdatedFunc  updated_func,
	gpointer                    user_data,
	GError                    **error)
{
	VtfContext* context = (VtfContext*)g_malloc(sizeof(VtfContext));
	
	if (context == NULL) {
		g_set_error(
			error,
			GDK_PIXBUF_ERROR,
			GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
			"Not enough memory");
		return NULL;
	}

	context->size_func     = size_func;
	context->prepared_func = prepared_func;
	context->updated_func  = updated_func;
	context->user_data     = user_data;

	context->buffer_size = BUFSIZ;
	context->buffer = (guchar*)g_malloc(context->buffer_size);
	context->buffer_used = 0;

	if (context->buffer == NULL) {
	    g_free(context);
		g_set_error(
			error,
			GDK_PIXBUF_ERROR,
			GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
			"Not enough memory");
		return NULL;
	}

	return context;
}

static gboolean vtf_image_load_increment(
	gpointer      context_ptr,
	const guchar *data,
	guint         size,
	GError      **error)
{
	VtfContext* context = (VtfContext*) context_ptr;
	const guint used = context->buffer_used + size;
	guint buffer_size = context->buffer_size;

	if (used > buffer_size) {
		while (used > buffer_size) {
			buffer_size *= 2;
		}

		guchar *buffer = (guchar*)g_realloc(context->buffer, buffer_size);

		if (buffer == NULL) {
			g_free(context->buffer);
			g_free(context);

			g_set_error(
				error,
				GDK_PIXBUF_ERROR,
				GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
				"Not enough memory");
			return FALSE;
		}

		context->buffer = buffer;
		context->buffer_size = buffer_size;
	}

	memcpy(context->buffer + context->buffer_used, data, size);
	context->buffer_used = used;

	return TRUE;
}

static gboolean vtf_image_stop_load(
	gpointer context_ptr,
	GError **error)
{
	VtfContext* context = (VtfContext*) context_ptr;
	GdkPixbuf* pixbuf = vtf_image_load_memory(context->buffer, context->buffer_used, error);

	if (pixbuf != NULL) {
		context->prepared_func(pixbuf, NULL, context->user_data);
	}

	g_free(context->buffer);
	g_free(context);

	return pixbuf != NULL;
}

extern "C" {

G_MODULE_EXPORT void fill_vtable(GdkPixbufModule* module)
{
	module->load           = vtf_image_load;
	module->begin_load     = vtf_image_begin_load;
	module->load_increment = vtf_image_load_increment;
	module->stop_load      = vtf_image_stop_load;
}

G_MODULE_EXPORT void fill_info(GdkPixbufFormat *info)
{
	info->name = "vtf";
	info->signature   = vtf_signature;
	info->description = "Valve Texture format";
	info->mime_types  = vtf_mime_types;
	info->extensions  = vtf_extensions;
	info->flags   = 0; // not threadsafe because of global LastError object
	info->license = "LGPL";
}

}
