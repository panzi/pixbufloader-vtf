/*    pixbufloader-vtf - load Valve Texture Format files in Gtk+ applications
 *    Copyright (C) 2014  Mathias Panzenböck
 *
 *    This library is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU Lesser General Public
 *    License as published by the Free Software Foundation; either
 *    version 2.1 of the License, or (at your option) any later version.
 *
 *    This library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public
 *    License along with this library; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <exception>
#include <sstream>

#include <gdk-pixbuf/gdk-pixbuf-io.h>
#include <gdk-pixbuf/gdk-pixbuf-simple-anim.h>
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

static std::string vtf_flag_names(vlUInt flags) {
	std::stringstream buf;
	bool notfirst = false;

    if (flags & TEXTUREFLAGS_POINTSAMPLE) {
		notfirst = true;
		buf << "PointSample";
    }
    if (flags & TEXTUREFLAGS_TRILINEAR) {
        if (notfirst) buf << ", ";
		notfirst = true;
		buf << "Trilinear";
    }
    if (flags & TEXTUREFLAGS_CLAMPS) {
        if (notfirst) buf << ", ";
		notfirst = true;
		buf << "ClampS";
    }
    if (flags & TEXTUREFLAGS_CLAMPT) {
        if (notfirst) buf << ", ";
		notfirst = true;
		buf << "ClampT";
    }
    if (flags & TEXTUREFLAGS_ANISOTROPIC) {
        if (notfirst) buf << ", ";
		notfirst = true;
		buf << "Anisotropic";
    }
    if (flags & TEXTUREFLAGS_HINT_DXT5) {
        if (notfirst) buf << ", ";
		notfirst = true;
		buf << "Hint DXT5";
    }
    if (flags & TEXTUREFLAGS_SRGB) {
        if (notfirst) buf << ", ";
		notfirst = true;
		buf << "SRGB";
    }
    if (flags & TEXTUREFLAGS_DEPRECATED_NOCOMPRESS) {
        if (notfirst) buf << ", ";
		notfirst = true;
		buf << "Deprecated NoCompress";
    }
    if (flags & TEXTUREFLAGS_NORMAL) {
        if (notfirst) buf << ", ";
		notfirst = true;
		buf << "Normal";
    }
    if (flags & TEXTUREFLAGS_NOMIP) {
        if (notfirst) buf << ", ";
		notfirst = true;
		buf << "NoMip";
    }
    if (flags & TEXTUREFLAGS_NOLOD) {
        if (notfirst) buf << ", ";
		notfirst = true;
		buf << "NoLOD";
    }
    if (flags & TEXTUREFLAGS_MINMIP) {
        if (notfirst) buf << ", ";
		notfirst = true;
		buf << "MinMip";
    }
    if (flags & TEXTUREFLAGS_PROCEDURAL) {
        if (notfirst) buf << ", ";
		notfirst = true;
		buf << "Procedural";
    }
    if (flags & TEXTUREFLAGS_ONEBITALPHA) {
        if (notfirst) buf << ", ";
		notfirst = true;
		buf << "OneBitAlpha";
    }
    if (flags & TEXTUREFLAGS_EIGHTBITALPHA) {
        if (notfirst) buf << ", ";
		notfirst = true;
		buf << "EightBitAlpha";
    }
    if (flags & TEXTUREFLAGS_ENVMAP) {
        if (notfirst) buf << ", ";
		notfirst = true;
		buf << "EnvMap";
    }
    if (flags & TEXTUREFLAGS_RENDERTARGET) {
        if (notfirst) buf << ", ";
		notfirst = true;
		buf << "RenderTarget";
    }
    if (flags & TEXTUREFLAGS_DEPTHRENDERTARGET) {
        if (notfirst) buf << ", ";
		notfirst = true;
		buf << "DepthRenderTarget";
    }
    if (flags & TEXTUREFLAGS_NODEBUGOVERRIDE) {
        if (notfirst) buf << ", ";
		notfirst = true;
		buf << "NoDebugOverride";
    }
    if (flags & TEXTUREFLAGS_SINGLECOPY) {
        if (notfirst) buf << ", ";
		notfirst = true;
		buf << "SingleCopy";
    }
    if (flags & TEXTUREFLAGS_UNUSED0) {
        if (notfirst) buf << ", ";
		notfirst = true;
		buf << "Unused0";
    }
    if (flags & TEXTUREFLAGS_DEPRECATED_ONEOVERMIPLEVELINALPHA) {
        if (notfirst) buf << ", ";
		notfirst = true;
		buf << "Deprecated OneOverMipLevelInAlpha";
    }
    if (flags & TEXTUREFLAGS_UNUSED1) {
        if (notfirst) buf << ", ";
		notfirst = true;
		buf << "Unused1";
    }
    if (flags & TEXTUREFLAGS_DEPRECATED_PREMULTCOLORBYONEOVERMIPLEVEL) {
        if (notfirst) buf << ", ";
		notfirst = true;
		buf << "Deprecated PremultColorByOneOverMipLevel";
    }
    if (flags & TEXTUREFLAGS_UNUSED2) {
        if (notfirst) buf << ", ";
		notfirst = true;
		buf << "Unused2";
    }
    if (flags & TEXTUREFLAGS_DEPRECATED_NORMALTODUDV) {
        if (notfirst) buf << ", ";
		notfirst = true;
		buf << "Deprecated NormalTODUDV";
    }
    if (flags & TEXTUREFLAGS_UNUSED3) {
        if (notfirst) buf << ", ";
		notfirst = true;
		buf << "Unused3";
    }
    if (flags & TEXTUREFLAGS_DEPRECATED_ALPHATESTMIPGENERATION) {
        if (notfirst) buf << ", ";
		notfirst = true;
		buf << "Deprecated AlphaTestMipGeneration";
    }
    if (flags & TEXTUREFLAGS_NODEPTHBUFFER) {
        if (notfirst) buf << ", ";
		notfirst = true;
		buf << "NoDepthBuffer";
    }
    if (flags & TEXTUREFLAGS_UNUSED4) {
        if (notfirst) buf << ", ";
		notfirst = true;
		buf << "Unused4";
    }
    if (flags & TEXTUREFLAGS_DEPRECATED_NICEFILTERED) {
        if (notfirst) buf << ", ";
		notfirst = true;
		buf << "Deprecated NiceFiltered";
    }
    if (flags & TEXTUREFLAGS_CLAMPU) {
        if (notfirst) buf << ", ";
		notfirst = true;
		buf << "ClampU";
    }
    if (flags & TEXTUREFLAGS_VERTEXTEXTURE) {
        if (notfirst) buf << ", ";
		notfirst = true;
		buf << "VertexTexture";
    }
    if (flags & TEXTUREFLAGS_SSBUMP) {
        if (notfirst) buf << ", ";
		notfirst = true;
		buf << "SSBump";
    }
    if (flags & TEXTUREFLAGS_UNUSED5) {
        if (notfirst) buf << ", ";
		notfirst = true;
		buf << "Unused5";
    }
    if (flags & TEXTUREFLAGS_DEPRECATED_UNFILTERABLE_OK) {
        if (notfirst) buf << ", ";
		notfirst = true;
		buf << "Deprecated Unfilterable Ok";
    }
    if (flags & TEXTUREFLAGS_BORDER) {
        if (notfirst) buf << ", ";
		notfirst = true;
		buf << "Border";
    }
    if (flags & TEXTUREFLAGS_DEPRECATED_SPECVAR_RED) {
        if (notfirst) buf << ", ";
		notfirst = true;
		buf << "Deprecated SpecVar Red";
    }
    if (flags & TEXTUREFLAGS_DEPRECATED_SPECVAR_ALPHA) {
        if (notfirst) buf << ", ";
		notfirst = true;
		buf << "Deprecated SpeCVar Alpha";
    }
    if (flags & TEXTUREFLAGS_LAST) {
        if (notfirst) buf << ", ";
		buf << "Last";
    }

    return buf.str();
}

template<typename number_type>
static std::string to_str(number_type number) {
	std::stringstream buf;
	buf << number;
	return buf.str();
}

static void vtf_image_add_options(const CVTFFile &vtf, GdkPixbuf *pixbuf) {
	SVTFImageFormatInfo formatInfo = VTFLib::CVTFFile::GetImageFormatInfo(vtf.GetFormat());
	std::stringstream buf;
	vlSingle rX = 0, rY = 0, rZ = 0;

	vtf.GetReflectivity(rX, rY, rZ);

	buf << vtf.GetMajorVersion() << '.' << vtf.GetMinorVersion();
	std::string version = buf.str().c_str();

	buf.str(std::string());
	buf.clear();
	buf << rX << ", " << rY << ", " << rZ;
	std::string reflectivity = buf.str();

	gdk_pixbuf_set_option(pixbuf, "Version", version.c_str());
	gdk_pixbuf_set_option(pixbuf, "Format", formatInfo.lpName);
	gdk_pixbuf_set_option(pixbuf, "Depth", to_str(vtf.GetDepth()).c_str());
	gdk_pixbuf_set_option(pixbuf, "Bumpmap Scale", to_str(vtf.GetBumpmapScale()).c_str());
	gdk_pixbuf_set_option(pixbuf, "Reflectivity", reflectivity.c_str());
	gdk_pixbuf_set_option(pixbuf, "Faces", to_str(vtf.GetFaceCount()).c_str());
	gdk_pixbuf_set_option(pixbuf, "Mipmaps", to_str(vtf.GetMipmapCount()).c_str());
	gdk_pixbuf_set_option(pixbuf, "Frames", to_str(vtf.GetFrameCount()).c_str());
	gdk_pixbuf_set_option(pixbuf, "Start Frame", to_str(vtf.GetStartFrame()).c_str());
	gdk_pixbuf_set_option(pixbuf, "Flags", vtf_flag_names(vtf.GetFlags()).c_str());
	gdk_pixbuf_set_option(pixbuf, "Bits Per Pixel", to_str(formatInfo.uiBitsPerPixel).c_str());
	gdk_pixbuf_set_option(pixbuf, "Alpha Channel", formatInfo.uiAlphaBitsPerPixel > 0 ? "True" : "False");
	gdk_pixbuf_set_option(pixbuf, "Compressed", formatInfo.bIsCompressed ? "True" : "False");

	if (vtf.GetHasThumbnail()) {
		SVTFImageFormatInfo thumbFormatInfo = VTFLib::CVTFFile::GetImageFormatInfo(vtf.GetThumbnailFormat());

		buf.str(std::string());
		buf.clear();

		buf << vtf.GetThumbnailWidth() << 'x' << vtf.GetThumbnailHeight();

		gdk_pixbuf_set_option(pixbuf, "Thumbnail Format", thumbFormatInfo.lpName);
		gdk_pixbuf_set_option(pixbuf, "Thumbnail Size", buf.str().c_str());
		gdk_pixbuf_set_option(pixbuf, "Thumbnail Bits Per Pixel", to_str(thumbFormatInfo.uiBitsPerPixel).c_str());
		gdk_pixbuf_set_option(pixbuf, "Thumbnail Alpha Channel", thumbFormatInfo.uiAlphaBitsPerPixel > 0 ? "True" : "False");
		gdk_pixbuf_set_option(pixbuf, "Thumbnail Compressed", thumbFormatInfo.bIsCompressed ? "True" : "False");
	}
}

static gboolean vtf_image_load_from_memory(
	const guchar        *buffer,
	guint                size,
	GdkPixbuf          **pixbuf_ptr,
	GdkPixbufAnimation **animation_ptr,
	GError             **error)
{
	GdkPixbuf           *pixbuf    = NULL;
	GdkPixbufSimpleAnim *animation = NULL;

	try {
		CVTFFile vtf;
		if (!vtf.Load(buffer, size)) {
			g_set_error(
				error,
				GDK_PIXBUF_ERROR,
				GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
				"%s", LastError.Get());
			return FALSE;
		}

		const vlUInt width  = vtf.GetWidth();
		const vlUInt height = vtf.GetHeight();
		const vlUInt frames = vtf.GetFrameCount();

		if (animation_ptr && (frames != 1 || !pixbuf_ptr)) {
			animation = gdk_pixbuf_simple_anim_new(width, height, 4.0);

			if (animation == NULL) {
				g_set_error(
					error,
					GDK_PIXBUF_ERROR,
					GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
					"Could not allocate GdkPixbufSimpleAnim object");
				return FALSE;
			}

			gdk_pixbuf_simple_anim_set_loop(animation, TRUE);

			for (vlUInt i = vtf.GetStartFrame(); i < frames; ++ i) {
				pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, width, height);

				if (pixbuf == NULL) {
					g_object_unref(animation);
					g_set_error(
						error,
						GDK_PIXBUF_ERROR,
						GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
						"Could not allocate GdkPixbuf object");
					return FALSE;
				}
				
				const vlByte* frame = vtf.GetData(i, 0, 0, 0);
				guchar* pixels = gdk_pixbuf_get_pixels(pixbuf);

				if (!CVTFFile::ConvertToRGBA8888(frame, pixels, width, height, vtf.GetFormat())) {
					g_object_unref(pixbuf);
					g_object_unref(animation);
					g_set_error(
						error,
						GDK_PIXBUF_ERROR,
						GDK_PIXBUF_ERROR_FAILED,
						"Image data conversion failed");
					return FALSE;
				}

				gdk_pixbuf_simple_anim_add_frame(animation, pixbuf);
			}
		}
		else if (pixbuf_ptr) {
			pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, width, height);

			if (pixbuf == NULL) {
				g_set_error(
					error,
					GDK_PIXBUF_ERROR,
					GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
					"Could not allocate GdkPixbuf object");
				return FALSE;
			}

			const vlByte* frame = vtf.GetData(vtf.GetFrameCount() - 1, 0, 0, 0);
			guchar* pixels = gdk_pixbuf_get_pixels(pixbuf);

			if (!CVTFFile::ConvertToRGBA8888(frame, pixels, width, height, vtf.GetFormat())) {
				g_object_unref(pixbuf);
				g_set_error(
					error,
					GDK_PIXBUF_ERROR,
					GDK_PIXBUF_ERROR_FAILED,
					"Image data conversion failed");
				return FALSE;
			}
		}

		if (pixbuf) {
			vtf_image_add_options(vtf, pixbuf);
		}

		if (pixbuf_ptr) {
			*pixbuf_ptr = pixbuf;
		}

		if (animation_ptr) {
			*animation_ptr = GDK_PIXBUF_ANIMATION(animation);
		}

		return TRUE;
	}
	catch (std::exception& ex) {
		if (animation) {
			g_object_unref(animation);
		}
		else if (pixbuf) {
			g_object_unref(pixbuf);
		}

		g_set_error(
			error,
			GDK_PIXBUF_ERROR,
			GDK_PIXBUF_ERROR_FAILED,
			"%s", ex.what());
		return FALSE;
	}
	catch (...) {
		if (animation) {
			g_object_unref(animation);
		}
		else if (pixbuf) {
			g_object_unref(pixbuf);
		}

		g_set_error(
			error,
			GDK_PIXBUF_ERROR,
			GDK_PIXBUF_ERROR_FAILED,
			"Unhandled C++ exception");
		return FALSE;
	}
}

static GdkPixbuf* vtf_image_load(
	FILE    *fp,
	GError **error)
{
	fseeko(fp, 0, SEEK_END);
	off_t size = ftello(fp);

	if (size < 0) {
		g_set_error(
			error,
			GDK_PIXBUF_ERROR,
			GDK_PIXBUF_ERROR_FAILED,
			"%s", strerror(errno));
		return NULL;
	}

	fseeko(fp, 0, SEEK_SET);
	guchar *buffer = (guchar*)g_malloc(size);

	if (buffer == NULL) {
		g_set_error(
			error,
			GDK_PIXBUF_ERROR,
			GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
			"Could not allocate buffer");
		return NULL;
	}

	if (fread(buffer, size, 1, fp) != 1) {
		g_free(buffer);
		g_set_error(
			error,
			GDK_PIXBUF_ERROR,
			GDK_PIXBUF_ERROR_FAILED,
			"%s", strerror(errno));
		return NULL;
	}

	GdkPixbuf* pixbuf = NULL;
	vtf_image_load_from_memory(buffer, size, &pixbuf, NULL, error);
	g_free(buffer);
	return pixbuf;
}

static GdkPixbufAnimation* vtf_image_load_animation(
	FILE    *fp,
	GError **error)
{
	fseeko(fp, 0, SEEK_END);
	off_t size = ftello(fp);

	if (size < 0) {
		g_set_error(
			error,
			GDK_PIXBUF_ERROR,
			GDK_PIXBUF_ERROR_FAILED,
			"%s", strerror(errno));
		return NULL;
	}

	fseeko(fp, 0, SEEK_SET);
	guchar *buffer = (guchar*)g_malloc(size);

	if (buffer == NULL) {
		g_set_error(
			error,
			GDK_PIXBUF_ERROR,
			GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
			"Could not allocate buffer");
		return NULL;
	}

	if (fread(buffer, size, 1, fp) != 1) {
		g_free(buffer);
		g_set_error(
			error,
			GDK_PIXBUF_ERROR,
			GDK_PIXBUF_ERROR_FAILED,
			"%s", strerror(errno));
		return NULL;
	}

	GdkPixbufAnimation* animation = NULL;
	vtf_image_load_from_memory(buffer, size, NULL, &animation, error);
	g_free(buffer);
	return animation;
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
	GdkPixbuf* pixbuf = NULL;
	GdkPixbufAnimation* animation = NULL;

	if (vtf_image_load_from_memory(context->buffer, context->buffer_used, &pixbuf, &animation, error)) {
		context->prepared_func(pixbuf, animation, context->user_data);
	}

	g_free(context->buffer);
	g_free(context);

	return pixbuf != NULL;
}

extern "C" {

G_MODULE_EXPORT void fill_vtable(GdkPixbufModule* module)
{
	module->load           = vtf_image_load;
	module->load_animation = vtf_image_load_animation;
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
